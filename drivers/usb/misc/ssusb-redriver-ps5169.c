// SPDX-License-Identifier: GPL-2.0-only
/*
 * Parade PS5169 USB/DisplayPort redriver.
 *
 * Lenovo TB371FC backport: the PS5169 register programming sequence follows
 * Lenovo ZUI PS5169 code, while the event plumbing follows the Qualcomm
 * 4.19 extcon redriver implementation used by this kernel generation.
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/extcon.h>
#include <linux/power_supply.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/regmap.h>
#include <linux/ctype.h>
#include <linux/delay.h>

#define NOTIFIER_PRIORITY 1
#define REDRIVER_REG_MAX  0xff

enum plug_orientation {
	ORIENTATION_NONE,
	ORIENTATION_CC1,
	ORIENTATION_CC2,
};

enum operation_mode {
	OP_MODE_USB,
	OP_MODE_DP,
	OP_MODE_USB_AND_DP,
};

struct chip_id {
	u8 chip_id_1;
	u8 chip_id_2;
	u8 revision_1;
	u8 revision_2;
};

struct ps5169_reg_sequence {
	u8 reg_addr;
	u8 reg_value;
	u8 wait_time;
};

struct ssusb_redriver {
	struct device *dev;
	struct regmap *regmap;
	struct i2c_client *client;

	struct work_struct config_work;
	struct workqueue_struct *redriver_wq;

	struct power_supply *usb_psy;
	bool host_active;
	bool vbus_active;
	bool is_usb3;
	enum plug_orientation typec_orientation;
	enum operation_mode op_mode;

	struct extcon_dev *extcon_usb;
	struct extcon_dev *extcon_dp;
	struct notifier_block vbus_nb;
	struct notifier_block id_nb;
	struct notifier_block dp_nb;
	struct notifier_block panic_nb;

	struct dentry *debug_root;
	struct chip_id id;
};

/*
 * These tables intentionally keep the Lenovo symbol names found in the
 * TB371FC stock kernel.  The values match Lenovo ZUI PS5169 programming.
 */
struct ps5169_reg_sequence ps5169_default_seq[] = {
	{ 0x9d, 0x80, 0 },
	{ 0x00, 0x00, 10 },
	{ 0x9d, 0x00, 0 },
	{ 0x40, 0x80, 0 },
	{ 0x04, 0x44, 0 },
	{ 0xa0, 0x02, 0 },
	{ 0x51, 0x87, 0 },
	{ 0x50, 0x00, 0 },
	{ 0x54, 0x01, 0 },
	{ 0x5d, 0x66, 0 },
	{ 0x52, 0x10, 0 },
	{ 0x55, 0x00, 0 },
	{ 0x56, 0x00, 0 },
	{ 0x57, 0x00, 0 },
	{ 0x58, 0x00, 0 },
	{ 0x59, 0x00, 0 },
	{ 0x5a, 0x00, 0 },
	{ 0x5b, 0x00, 0 },
	{ 0x5e, 0x06, 0 },
	{ 0x5f, 0x00, 0 },
	{ 0x60, 0x00, 0 },
	{ 0x61, 0x03, 0 },
	{ 0x65, 0x40, 0 },
	{ 0x66, 0x00, 0 },
	{ 0x67, 0x03, 0 },
	{ 0x75, 0x0c, 0 },
	{ 0x77, 0x00, 0 },
	{ 0x78, 0x7c, 0 },
};

struct ps5169_reg_sequence ps5169_USB_seq[] = {
	{ 0x40, 0xc0, 0 },
	{ 0x8d, 0x01, 0 },
	{ 0x90, 0x01, 0 },
};

struct ps5169_reg_sequence ps5169_USB_FLIP_seq[] = {
	{ 0x40, 0xd0, 0 },
	{ 0x8d, 0x01, 0 },
	{ 0x90, 0x01, 0 },
};

struct ps5169_reg_sequence ps5169_USB_remove_seq[] = {
	{ 0x40, 0x80, 0 },
	{ 0x8d, 0x00, 0 },
	{ 0x90, 0x00, 0 },
};

struct ps5169_reg_sequence ps5169_DP_seq[] = {
	{ 0x40, 0xa0, 0 },
	{ 0xa0, 0x00, 0 },
	{ 0xa1, 0x04, 0 },
};

struct ps5169_reg_sequence ps5169_DP_FLIP_seq[] = {
	{ 0x40, 0xb0, 0 },
	{ 0xa0, 0x00, 0 },
	{ 0xa1, 0x04, 0 },
};

struct ps5169_reg_sequence ps5169_DP_remove_seq[] = {
	{ 0x40, 0x80, 0 },
	{ 0xa0, 0x02, 0 },
	{ 0xa1, 0x00, 0 },
};

struct ps5169_reg_sequence ps5169_USB_AND_DP_seq[] = {
	{ 0x40, 0xe0, 0 },
	{ 0x8d, 0x01, 0 },
	{ 0x90, 0x01, 0 },
	{ 0xa0, 0x00, 0 },
	{ 0xa1, 0x04, 0 },
};

struct ps5169_reg_sequence ps5169_USB_AND_DP_FLIP_seq[] = {
	{ 0x40, 0xf0, 0 },
	{ 0x8d, 0x01, 0 },
	{ 0x90, 0x01, 0 },
	{ 0xa0, 0x00, 0 },
	{ 0xa1, 0x04, 0 },
};

struct ps5169_reg_sequence ps5169_USB_AND_DP_remove_seq[] = {
	{ 0x40, 0x80, 0 },
	{ 0xa0, 0x02, 0 },
	{ 0x8d, 0x00, 0 },
	{ 0x90, 0x00, 0 },
	{ 0xa1, 0x00, 0 },
};

static bool redriver_exist;

bool get_redriver_exist(void)
{
	return redriver_exist;
}
EXPORT_SYMBOL(get_redriver_exist);

static int redriver_i2c_reg_get(struct ssusb_redriver *redriver,
				u8 reg, u8 *val)
{
	unsigned int tmp;
	int ret;

	ret = regmap_read(redriver->regmap, reg, &tmp);
	if (ret < 0) {
		dev_err(redriver->dev, "reading reg 0x%02x failure\n", reg);
		return ret;
	}

	*val = (u8)tmp;
	return 0;
}

static int redriver_i2c_reg_set(struct ssusb_redriver *redriver,
				u8 reg, u8 val)
{
	int ret;

	ret = regmap_write(redriver->regmap, reg, val);
	if (ret < 0)
		dev_err(redriver->dev, "writing reg 0x%02x failure\n", reg);

	return ret;
}

static int redriver_i2c_reg_table_set(struct ssusb_redriver *redriver,
				      struct ps5169_reg_sequence *table,
				      size_t count)
{
	size_t i;
	int ret = 0;

	for (i = 0; i < count; i++) {
		if (table[i].wait_time) {
			msleep(table[i].wait_time);
			continue;
		}

		ret = redriver_i2c_reg_set(redriver, table[i].reg_addr,
					  table[i].reg_value);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static int redriver_get_chip_id(struct ssusb_redriver *redriver)
{
	int ret;

	memset(&redriver->id, 0, sizeof(redriver->id));

	ret = redriver_i2c_reg_get(redriver, 0xad,
				   &redriver->id.chip_id_1);
	if (ret < 0)
		return ret;
	ret = redriver_i2c_reg_get(redriver, 0xac,
				   &redriver->id.chip_id_2);
	if (ret < 0)
		return ret;
	ret = redriver_i2c_reg_get(redriver, 0xae,
				   &redriver->id.revision_1);
	if (ret < 0)
		return ret;
	ret = redriver_i2c_reg_get(redriver, 0xaf,
				   &redriver->id.revision_2);
	if (ret < 0)
		return ret;

	dev_info(redriver->dev,
		 "get chip id: id_1 = 0x%02x, id_2 = 0x%02x, revision_1 = 0x%02x, revision_2 = 0x%02x\n",
		 redriver->id.chip_id_1, redriver->id.chip_id_2,
		 redriver->id.revision_1, redriver->id.revision_2);
	return 0;
}

static int ssusb_redriver_default_config(struct ssusb_redriver *redriver)
{
	return redriver_i2c_reg_table_set(redriver, ps5169_default_seq,
					 ARRAY_SIZE(ps5169_default_seq));
}

static int ssusb_redriver_mode_set(struct ssusb_redriver *redriver, bool on)
{
	struct ps5169_reg_sequence *table = NULL;
	size_t count = 0;

	switch (redriver->op_mode) {
	case OP_MODE_USB:
		if (!on) {
			table = ps5169_USB_remove_seq;
			count = ARRAY_SIZE(ps5169_USB_remove_seq);
		} else if (redriver->typec_orientation == ORIENTATION_CC2) {
			table = ps5169_USB_FLIP_seq;
			count = ARRAY_SIZE(ps5169_USB_FLIP_seq);
		} else if (redriver->typec_orientation == ORIENTATION_CC1) {
			table = ps5169_USB_seq;
			count = ARRAY_SIZE(ps5169_USB_seq);
		}
		break;
	case OP_MODE_DP:
		if (!on) {
			table = ps5169_DP_remove_seq;
			count = ARRAY_SIZE(ps5169_DP_remove_seq);
		} else if (redriver->typec_orientation == ORIENTATION_CC2) {
			table = ps5169_DP_FLIP_seq;
			count = ARRAY_SIZE(ps5169_DP_FLIP_seq);
		} else if (redriver->typec_orientation == ORIENTATION_CC1) {
			table = ps5169_DP_seq;
			count = ARRAY_SIZE(ps5169_DP_seq);
		}
		break;
	case OP_MODE_USB_AND_DP:
		if (!on) {
			table = ps5169_USB_AND_DP_remove_seq;
			count = ARRAY_SIZE(ps5169_USB_AND_DP_remove_seq);
		} else if (redriver->typec_orientation == ORIENTATION_CC2) {
			table = ps5169_USB_AND_DP_FLIP_seq;
			count = ARRAY_SIZE(ps5169_USB_AND_DP_FLIP_seq);
		} else if (redriver->typec_orientation == ORIENTATION_CC1) {
			table = ps5169_USB_AND_DP_seq;
			count = ARRAY_SIZE(ps5169_USB_AND_DP_seq);
		}
		break;
	}

	if (!table) {
		if (on)
			dev_dbg(redriver->dev,
				"defer mode programming until Type-C orientation is known\n");
		return 0;
	}

	return redriver_i2c_reg_table_set(redriver, table, count);
}

static void ssusb_redriver_gen_dev_set(struct ssusb_redriver *redriver,
				       bool on)
{
	int ret;

	ret = ssusb_redriver_mode_set(redriver, on);
	if (ret < 0)
		dev_err(redriver->dev, "failed to %s PS5169 mode %d: %d\n",
			on ? "enable" : "disable", redriver->op_mode, ret);
}

static void ssusb_redriver_config_work(struct work_struct *work)
{
	struct ssusb_redriver *redriver =
		container_of(work, struct ssusb_redriver, config_work);
	struct extcon_dev *edev = redriver->extcon_usb;
	union extcon_property_value val;
	unsigned int extcon_id = EXTCON_NONE;
	int ret;

	if (redriver->vbus_active)
		extcon_id = EXTCON_USB;
	else if (redriver->host_active)
		extcon_id = EXTCON_USB_HOST;

	if (edev && extcon_id != EXTCON_NONE &&
	    extcon_get_state(edev, extcon_id)) {
		ret = extcon_get_property(edev, extcon_id,
					 EXTCON_PROP_USB_SS, &val);
		if (!ret)
			redriver->is_usb3 = val.intval != 0;
		else
			redriver->is_usb3 = true;

		if (redriver->is_usb3 || redriver->op_mode != OP_MODE_USB) {
			ret = extcon_get_property(edev, extcon_id,
					EXTCON_PROP_USB_TYPEC_POLARITY, &val);
			if (!ret)
				redriver->typec_orientation = val.intval ?
					ORIENTATION_CC2 : ORIENTATION_CC1;
			else if (redriver->op_mode == OP_MODE_USB)
				redriver->typec_orientation = ORIENTATION_NONE;

			ssusb_redriver_gen_dev_set(redriver, true);
		} else {
			ssusb_redriver_gen_dev_set(redriver, false);
		}
	} else if (redriver->op_mode != OP_MODE_USB) {
		ssusb_redriver_gen_dev_set(redriver, true);
	} else {
		redriver->op_mode = OP_MODE_USB;
		redriver->typec_orientation = ORIENTATION_NONE;
		ssusb_redriver_gen_dev_set(redriver, false);
	}
}

static int ssusb_redriver_dp_notifier(struct notifier_block *nb,
				      unsigned long dp_lane, void *ptr)
{
	struct ssusb_redriver *redriver =
		container_of(nb, struct ssusb_redriver, dp_nb);
	enum operation_mode op_mode;

	switch (dp_lane) {
	case 0:
		op_mode = OP_MODE_USB;
		break;
	case 2:
		op_mode = OP_MODE_USB_AND_DP;
		break;
	case 4:
		op_mode = OP_MODE_DP;
		break;
	default:
		return NOTIFY_DONE;
	}

	if (redriver->op_mode == op_mode)
		return NOTIFY_DONE;

	redriver->op_mode = op_mode;
	queue_work(redriver->redriver_wq, &redriver->config_work);
	return NOTIFY_DONE;
}

static int ssusb_redriver_vbus_notifier(struct notifier_block *nb,
					unsigned long event, void *ptr)
{
	struct ssusb_redriver *redriver =
		container_of(nb, struct ssusb_redriver, vbus_nb);

	if (redriver->vbus_active == !!event)
		return NOTIFY_DONE;

	redriver->vbus_active = !!event;
	queue_work(redriver->redriver_wq, &redriver->config_work);
	return NOTIFY_DONE;
}

static int ssusb_redriver_id_notifier(struct notifier_block *nb,
				      unsigned long event, void *ptr)
{
	struct ssusb_redriver *redriver =
		container_of(nb, struct ssusb_redriver, id_nb);
	bool host_active = !!event;

	if (redriver->host_active == host_active)
		return NOTIFY_DONE;

	redriver->host_active = host_active;
	queue_work(redriver->redriver_wq, &redriver->config_work);
	return NOTIFY_DONE;
}

static int ssusb_redriver_extcon_register(struct ssusb_redriver *redriver)
{
	struct device_node *node = redriver->dev->of_node;
	struct extcon_dev *edev;
	int ret;

	if (!of_find_property(node, "extcon", NULL)) {
		dev_err(redriver->dev, "failed to get extcon for redriver\n");
		return -EINVAL;
	}

	edev = extcon_get_edev_by_phandle(redriver->dev, 0);
	if (IS_ERR(edev)) {
		dev_err(redriver->dev, "failed to get phandle for redriver\n");
		return PTR_ERR(edev);
	}

	redriver->extcon_usb = edev;
	redriver->vbus_nb.notifier_call = ssusb_redriver_vbus_notifier;
	redriver->vbus_nb.priority = NOTIFIER_PRIORITY;
	ret = extcon_register_notifier(edev, EXTCON_USB, &redriver->vbus_nb);
	if (ret < 0)
		return ret;

	redriver->id_nb.notifier_call = ssusb_redriver_id_notifier;
	redriver->id_nb.priority = NOTIFIER_PRIORITY;
	ret = extcon_register_notifier(edev, EXTCON_USB_HOST, &redriver->id_nb);
	if (ret < 0)
		goto unregister_vbus;

	if (of_count_phandle_with_args(node, "extcon", NULL) > 1) {
		edev = extcon_get_edev_by_phandle(redriver->dev, 1);
		if (IS_ERR(edev)) {
			ret = PTR_ERR(edev);
			goto unregister_id;
		}

		redriver->extcon_dp = edev;
		redriver->dp_nb.notifier_call = ssusb_redriver_dp_notifier;
		redriver->dp_nb.priority = NOTIFIER_PRIORITY;
		ret = extcon_register_blocking_notifier(edev, EXTCON_DISP_DP,
							&redriver->dp_nb);
		if (ret < 0)
			goto unregister_id;
	}

	if (extcon_get_state(redriver->extcon_usb, EXTCON_USB))
		ssusb_redriver_vbus_notifier(&redriver->vbus_nb, true,
					    redriver->extcon_usb);
	else if (extcon_get_state(redriver->extcon_usb, EXTCON_USB_HOST))
		ssusb_redriver_id_notifier(&redriver->id_nb, true,
					  redriver->extcon_usb);

	return 0;

unregister_id:
	extcon_unregister_notifier(redriver->extcon_usb, EXTCON_USB_HOST,
				   &redriver->id_nb);
unregister_vbus:
	extcon_unregister_notifier(redriver->extcon_usb, EXTCON_USB,
				   &redriver->vbus_nb);
	return ret;
}

static void ssusb_redriver_extcon_unregister(struct ssusb_redriver *redriver)
{
	if (redriver->extcon_dp)
		extcon_unregister_blocking_notifier(redriver->extcon_dp,
						    EXTCON_DISP_DP,
						    &redriver->dp_nb);
	if (redriver->extcon_usb) {
		extcon_unregister_notifier(redriver->extcon_usb, EXTCON_USB_HOST,
					   &redriver->id_nb);
		extcon_unregister_notifier(redriver->extcon_usb, EXTCON_USB,
					   &redriver->vbus_nb);
	}
}

static int ssusb_redriver_panic_notifier(struct notifier_block *nb,
					 unsigned long event, void *ptr)
{
	struct ssusb_redriver *redriver =
		container_of(nb, struct ssusb_redriver, panic_nb);

	pr_err("PS5169: op mode=%d vbus=%d host=%d orientation=%d\n",
	       redriver->op_mode, redriver->vbus_active,
	       redriver->host_active, redriver->typec_orientation);
	return NOTIFY_OK;
}

static const int dump_reg_list[] = {
	0x40, 0xa0, 0xa1, 0x52, 0x5e, 0x5c,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
	0x50, 0x5d, 0x54, 0x51, 0x77, 0x78,
	0xac, 0xad, 0xae, 0xaf,
};

static int dump_register_show(struct seq_file *s, void *unused)
{
	struct ssusb_redriver *redriver = s->private;
	u8 val;
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(dump_reg_list); i++) {
		ret = redriver_i2c_reg_get(redriver, dump_reg_list[i], &val);
		if (ret < 0)
			seq_printf(s, "0x%02x = ERR\n", dump_reg_list[i]);
		else
			seq_printf(s, "0x%02x = 0x%02x\n",
				   dump_reg_list[i], val);
	}
	return 0;
}

static int dump_register_open(struct inode *inode, struct file *file)
{
	return single_open(file, dump_register_show, inode->i_private);
}

static const struct file_operations dump_register_ops = {
	.open = dump_register_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int update_register_open(struct inode *inode, struct file *file)
{
	return single_open(file, NULL, inode->i_private);
}

static ssize_t update_register_write(struct file *file,
				     const char __user *ubuf,
				     size_t count, loff_t *ppos)
{
	struct seq_file *s = file->private_data;
	struct ssusb_redriver *redriver = s->private;
	char buf[40];
	unsigned int reg, val;

	if (count >= sizeof(buf))
		return -EINVAL;
	if (copy_from_user(buf, ubuf, count))
		return -EFAULT;
	buf[count] = 0;

	if (sscanf(buf, "0x%x=0x%x", &reg, &val) != 2)
		return -EINVAL;
	if (reg > REDRIVER_REG_MAX || val > 0xff)
		return -EINVAL;
	if (redriver_i2c_reg_set(redriver, reg, val) < 0)
		return -EIO;

	return count;
}

static const struct file_operations update_register_ops = {
	.open = update_register_open,
	.write = update_register_write,
	.llseek = seq_lseek,
	.release = single_release,
};

static void ssusb_redriver_debugfs_entries(struct ssusb_redriver *redriver)
{
	struct dentry *entry;

	redriver->debug_root = debugfs_create_dir("ps5169_redriver", NULL);
	if (IS_ERR_OR_NULL(redriver->debug_root))
		return;

	entry = debugfs_create_file("dump_register", 0600,
				    redriver->debug_root, redriver,
				    &dump_register_ops);
	if (IS_ERR_OR_NULL(entry))
		dev_warn(redriver->dev, "Could not create dump_register file\n");

	entry = debugfs_create_file("update_register", 0600,
				    redriver->debug_root, redriver,
				    &update_register_ops);
	if (IS_ERR_OR_NULL(entry))
		dev_warn(redriver->dev, "Could not create update_register file\n");
}

static const struct regmap_config redriver_regmap = {
	.name = "ps5169",
	.max_register = REDRIVER_REG_MAX,
	.reg_bits = 8,
	.val_bits = 8,
};

static int redriver_i2c_probe(struct i2c_client *client,
			      const struct i2c_device_id *id)
{
	struct ssusb_redriver *redriver;
	union power_supply_propval pval = { 0 };
	int ret;

	redriver = devm_kzalloc(&client->dev, sizeof(*redriver), GFP_KERNEL);
	if (!redriver)
		return -ENOMEM;

	redriver->dev = &client->dev;
	redriver->client = client;
	redriver->typec_orientation = ORIENTATION_NONE;
	INIT_WORK(&redriver->config_work, ssusb_redriver_config_work);

	redriver->redriver_wq = alloc_ordered_workqueue("redriver_wq",
							WQ_HIGHPRI);
	if (!redriver->redriver_wq)
		return -ENOMEM;

	redriver->regmap = devm_regmap_init_i2c(client, &redriver_regmap);
	if (IS_ERR(redriver->regmap)) {
		ret = PTR_ERR(redriver->regmap);
		goto destroy_wq;
	}

	i2c_set_clientdata(client, redriver);

	ret = redriver_get_chip_id(redriver);
	if (ret < 0)
		dev_warn(&client->dev, "Failed to get chip id: %d, continue\n",
			 ret);
	else
		redriver_exist = true;

	ret = ssusb_redriver_default_config(redriver);
	if (ret < 0)
		goto destroy_wq;

	redriver->host_active = false;
	redriver->op_mode = OP_MODE_USB;

	redriver->usb_psy = power_supply_get_by_name("usb");
	if (!redriver->usb_psy) {
		dev_warn(&client->dev, "Could not get usb power_supply\n");
		pval.intval = -EINVAL;
	} else {
		power_supply_get_property(redriver->usb_psy,
					  POWER_SUPPLY_PROP_PRESENT, &pval);
		if (!pval.intval)
			ssusb_redriver_gen_dev_set(redriver, false);
	}

	ret = ssusb_redriver_extcon_register(redriver);
	if (ret)
		goto put_psy;

	redriver->panic_nb.notifier_call = ssusb_redriver_panic_notifier;
	atomic_notifier_chain_register(&panic_notifier_list,
				       &redriver->panic_nb);

	ssusb_redriver_debugfs_entries(redriver);
	dev_info(&client->dev, "PS5169 USB/DP re-driver probed\n");
	return 0;

put_psy:
	if (redriver->usb_psy)
		power_supply_put(redriver->usb_psy);
destroy_wq:
	redriver_exist = false;
	destroy_workqueue(redriver->redriver_wq);
	return ret;
}

static int redriver_i2c_remove(struct i2c_client *client)
{
	struct ssusb_redriver *redriver = i2c_get_clientdata(client);

	debugfs_remove_recursive(redriver->debug_root);
	atomic_notifier_chain_unregister(&panic_notifier_list,
					 &redriver->panic_nb);
	ssusb_redriver_extcon_unregister(redriver);
	cancel_work_sync(&redriver->config_work);
	if (redriver->usb_psy)
		power_supply_put(redriver->usb_psy);
	destroy_workqueue(redriver->redriver_wq);
	redriver_exist = false;
	return 0;
}

static int __maybe_unused redriver_i2c_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ssusb_redriver *redriver = i2c_get_clientdata(client);

	if ((!redriver->vbus_active && !redriver->host_active &&
	     redriver->op_mode != OP_MODE_DP) ||
	    (redriver->host_active &&
	     redriver->op_mode == OP_MODE_USB_AND_DP))
		ssusb_redriver_gen_dev_set(redriver, false);

	flush_workqueue(redriver->redriver_wq);
	return 0;
}

static int __maybe_unused redriver_i2c_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ssusb_redriver *redriver = i2c_get_clientdata(client);

	if (redriver->host_active &&
	    redriver->op_mode == OP_MODE_USB_AND_DP)
		ssusb_redriver_gen_dev_set(redriver, true);

	flush_workqueue(redriver->redriver_wq);
	return 0;
}

static SIMPLE_DEV_PM_OPS(redriver_i2c_pm, redriver_i2c_suspend,
			 redriver_i2c_resume);

static void redriver_i2c_shutdown(struct i2c_client *client)
{
	struct ssusb_redriver *redriver = i2c_get_clientdata(client);

	ssusb_redriver_gen_dev_set(redriver, false);
}

static const struct of_device_id redriver_match_table[] = {
	{ .compatible = "hq_redriver,ps5169" },
	{ },
};
MODULE_DEVICE_TABLE(of, redriver_match_table);

static const struct i2c_device_id redriver_i2c_id[] = {
	{ "ps5169", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, redriver_i2c_id);

static struct i2c_driver redriver_i2c_driver = {
	.driver = {
		.name = "ssusb redriver hq",
		.of_match_table = redriver_match_table,
		.pm = &redriver_i2c_pm,
	},
	.probe = redriver_i2c_probe,
	.remove = redriver_i2c_remove,
	.shutdown = redriver_i2c_shutdown,
	.id_table = redriver_i2c_id,
};

module_i2c_driver(redriver_i2c_driver);

MODULE_DESCRIPTION("Parade PS5169 USB/DisplayPort redriver");
MODULE_LICENSE("GPL v2");
