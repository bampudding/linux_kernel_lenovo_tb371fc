#!/bin/sh
# Run once after installing the APatch module and before rebooting.
# APatch 11224 moves system/vendor to vendor and replaces system/vendor with
# a symlink. Mountify 204 only copies real directories under module/system,
# so it cannot overlay the vendor HAL until the link is replaced by a copy.
set -eu

SERIAL="${1:-6ab06f89}"
if [ "$SERIAL" != "6ab06f89" ]; then
  echo "Refusing to operate on a non-TB371FC ADB serial" >&2
  exit 1
fi

MODEL="$(adb -s "$SERIAL" shell getprop ro.product.model | tr -d '\r')"
if [ "$MODEL" != "TB371FC" ]; then
  echo "Refusing to operate on $MODEL" >&2
  exit 1
fi

adb -s "$SERIAL" root >/dev/null
adb -s "$SERIAL" wait-for-device
adb -s "$SERIAL" shell '
  set -eu
  dir=/data/adb/modules/tb371fc-turnip-vulkan-263
  test -f "$dir/vendor/lib64/hw/vulkan.adreno.so"
  if test -L "$dir/system/vendor"; then
    rm "$dir/system/vendor"
  fi
  mkdir -p "$dir/system/vendor"
  cp -a "$dir/vendor/." "$dir/system/vendor/"
  test "$(sha256sum "$dir/system/vendor/lib64/hw/vulkan.adreno.so" | cut -d " " -f1)" = "2d37305bb0e74d6b56a279671630af0722915ae1730b26426b562f9fb4e79593"
  echo "Mountify-compatible module content verified; reboot to activate."
'
