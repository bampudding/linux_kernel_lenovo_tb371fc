#!/bin/bash
set -e

echo '=== TB371FC One-Click Flash Script (EROFS Stack) ==='
echo 'Ensuring fastbootd mode...'
fastboot getvar is-userspace 2>&1 | grep -q 'is-userspace: yes' || {
    echo 'Device is not in fastbootd mode! Booting to fastbootd...'
    fastboot reboot fastboot
    sleep 10
}

echo '1. Flashing boot_b...'
fastboot flash boot_b boot.img

echo '2. Flashing vendor_b...'
fastboot flash vendor_b vendor_final_erofs.simg

echo '3. Flashing vbmeta_b...'
fastboot flash vbmeta_b vbmeta_b_final_perfect.img

echo '4. Erasing userdata & metadata (Wipe)...'
fastboot erase userdata
fastboot erase metadata

echo '=== Flash Complete! Rebooting to system... ==='
fastboot reboot
