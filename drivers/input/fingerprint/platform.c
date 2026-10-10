/*
 * platform indepent driver interface
 *
 * Coypritht (c) 2017 Goodix
 */
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/timer.h>
#include <linux/err.h>

#include "gf_spi.h"

#if defined(USE_SPI_BUS)
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>
#elif defined(USE_PLATFORM_BUS)
#include <linux/platform_device.h>
#endif

int gf_parse_dts(struct gf_dev *gf_dev)
{
	int rc = 0;
	struct device *dev = &gf_dev->spi->dev;
	struct device_node *np = dev->of_node;

	gf_dev->reset_gpio = of_get_named_gpio(np, "fp-gpio-reset", 0);
	if (gf_dev->reset_gpio < 0) {
		pr_err("failed to get reset gpio!\n");
		return gf_dev->reset_gpio;
	}
	rc = gpio_request(gf_dev->reset_gpio, "goodix_reset");
	if (rc) {
		pr_err("failed to request reset gpio, rc = %d\n", rc);
		return rc;
	}

	gf_dev->irq_gpio = of_get_named_gpio(np, "fp-gpio-irq", 0);
	if (gf_dev->irq_gpio < 0) {
		pr_err("failed to get irq gpio!\n");
		rc = gf_dev->irq_gpio;
		goto err_irq_gpio;
	}
	rc = gpio_request(gf_dev->irq_gpio, "goodix_irq");
	if (rc) {
		pr_err("failed to request irq gpio, rc = %d\n", rc);
		goto err_irq_gpio;
	}

	gf_dev->vdd_supply = regulator_get(dev, "goodix_vdd");
	if (IS_ERR(gf_dev->vdd_supply)) {
		rc = PTR_ERR(gf_dev->vdd_supply);
		gf_dev->vdd_supply = NULL;
		pr_err("failed to get goodix_vdd, rc = %d\n", rc);
		goto err_vdd;
	}

	rc = gpio_direction_output(gf_dev->reset_gpio, 0);
	if (rc) {
		pr_err("failed to set reset gpio output, rc = %d\n", rc);
		goto err_regulator;
	}
	rc = gpio_direction_input(gf_dev->irq_gpio);
	if (rc) {
		pr_err("failed to set irq gpio input, rc = %d\n", rc);
		goto err_regulator;
	}
	return 0;

err_regulator:
	regulator_put(gf_dev->vdd_supply);
	gf_dev->vdd_supply = NULL;
err_vdd:
	gpio_free(gf_dev->irq_gpio);
	gf_dev->irq_gpio = -EINVAL;
err_irq_gpio:
	gpio_free(gf_dev->reset_gpio);
	gf_dev->reset_gpio = -EINVAL;
	return rc;
}

void gf_cleanup(struct gf_dev *gf_dev)
{
	pr_info("[info] %s\n", __func__);

	if (gpio_is_valid(gf_dev->irq_gpio)) {
		gpio_free(gf_dev->irq_gpio);
		gf_dev->irq_gpio = -EINVAL;
		pr_info("remove irq_gpio success\n");
	}
	if (gpio_is_valid(gf_dev->reset_gpio)) {
		gpio_free(gf_dev->reset_gpio);
		gf_dev->reset_gpio = -EINVAL;
		pr_info("remove reset_gpio success\n");
	}
}

int gf_power_on(struct gf_dev *gf_dev)
{
	int rc;

	pr_info("%s\n", __func__);
	if (gf_dev->power_enabled)
		return 0;
	if (!gf_dev->vdd_supply || !gpio_is_valid(gf_dev->reset_gpio))
		return -ENODEV;

	gpio_set_value(gf_dev->reset_gpio, 0);
	mdelay(40);

	rc = regulator_set_voltage(gf_dev->vdd_supply, 3000000, 3000000);
	if (rc)
		pr_warn("goodix_vdd voltage request not supported, rc = %d; using regulator constraints\n", rc);
	rc = regulator_enable(gf_dev->vdd_supply);
	if (rc) {
		pr_err("regulator enable failed, rc = %d\n", rc);
		return rc;
	}

	gf_dev->power_enabled = true;
	pr_info("regulator_enable=%d\n", rc);
	gpio_set_value(gf_dev->reset_gpio, 1);
	mdelay(10);
	return rc;
}

int gf_power_off(struct gf_dev *gf_dev)
{
	int rc;

	pr_info("%s\n", __func__);
	if (!gf_dev->power_enabled)
		return 0;
	if (!gf_dev->vdd_supply)
		return -ENODEV;

	if (gpio_is_valid(gf_dev->reset_gpio))
		gpio_set_value(gf_dev->reset_gpio, 0);
	rc = regulator_disable(gf_dev->vdd_supply);
	if (rc) {
		pr_err("regulator disable failed, rc = %d\n", rc);
		return rc;
	}
	gf_dev->power_enabled = false;
	pr_info("regulator disable\n");
	mdelay(1);
	return 0;
}

int gf_hw_reset(struct gf_dev *gf_dev, unsigned int delay_ms)
{
	if (gf_dev == NULL || !gf_dev->power_enabled ||
		!gpio_is_valid(gf_dev->reset_gpio)) {
		pr_info("Input buff is NULL.\n");
		return -ENODEV;
	}
	if (gpio_direction_output(gf_dev->reset_gpio, 1))
		return -EIO;
	gpio_set_value(gf_dev->reset_gpio, 0);
	mdelay(3);
	gpio_set_value(gf_dev->reset_gpio, 1);
	mdelay(delay_ms);
	return 0;
}

int gf_irq_num(struct gf_dev *gf_dev)
{
	if (gf_dev == NULL) {
		pr_info("Input buff is NULL.\n");
		return -1;
	} else {
		return gpio_to_irq(gf_dev->irq_gpio);
	}
}
