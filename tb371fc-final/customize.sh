#!/system/bin/sh
if [ "$(getprop ro.product.model)" != TB371FC ]; then abort "TB371FC required"; fi
if [ "$(getprop ro.soc.model)" != SM8250 ]; then abort "SM8250 required"; fi
if [ "$(cat /sys/class/kgsl/kgsl-3d0/gpu_model 2>/dev/null)" != Adreno650v3 ]; then abort "Adreno650v3 required"; fi
set_perm_recursive "$MODPATH/system" 0 0 0755 0644
set_perm_recursive "$MODPATH/system/vendor/lib" 0 0 0755 0644 u:object_r:same_process_hal_file:s0
set_perm_recursive "$MODPATH/system/vendor/lib64" 0 0 0755 0644 u:object_r:same_process_hal_file:s0
if [ -f "$MODPATH/system/vendor/firmware/a650_sqe.fw" ]; then
  set_perm "$MODPATH/system/vendor/firmware/a650_sqe.fw" 0 0 0644 u:object_r:vendor_firmware_file:s0
fi
ui_print "TB371FC Adreno v676.68 installed; Turnip 64-bit Vulkan preserved."
