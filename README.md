# Lenovo Xiaoxin Pad Pro 12.7 (TB371FC) — final verified build

**Release:** `tb371fc-final-20261010` · **Hardware:** Lenovo TB371FC (Qualcomm SM8250 / Adreno 650 v3) · **Verified:** 2026-10-10

This branch contains the **final kernel source snapshot** used for the TB371FC running build, plus the integration scripts for its graphics stack. Previous experimental releases are superseded. This project is **not** the Lenovo P11 TB-J606F kernel.

## What was verified on-device

| Layer | Final state |
| --- | --- |
| Kernel | `4.19.325-perf+ #6`, APatch KernelPatch `d03`, working on slot B |
| Kernel source origin | **Linux 4.19.235-derived**, with `SUBLEVEL=325` set; **not** a complete 4.19.325 upstream LTS backport |
| Kernel GPU | Qualcomm KGSL/Adreno A650; GPU memory `gpu_mem_total` eBPF tracking fixed; DMA-BUF lifetime and perf-counter bounds fixes |
| OpenGL ES 32/64-bit | Qualcomm **V@0676.68** (2025-02-17) via reversible APatch module |
| Vulkan 64-bit | Mesa **Turnip 26.3.0-devel** (`git-d0e18ae81e`), Vulkan **1.3.363** on A650 |
| Vulkan 32-bit | Qualcomm v676.68, Vulkan **1.1.128** on A650 |
| Vendor | Existing Lenovo VNDK **30** and EROFS vendor; no VNDK31 switch |
| Compatibility | Small **32/64-bit ION-backed `libdmabufheap` ABI bridge** enables the newer Qualcomm GLES stack with the legacy vendor libc++ |
| Display | TB371FC DSC/touch resume fixes preserved; supported modes **120/60/30 Hz**; 144 Hz is *not* validated |

Device testing: two successful restarts, 32/64-bit OpenGL pbuffer render/readback with `(255, 0, 0, 255)`, **20/20** repeated GPU rendering checks, 32-bit Qualcomm and 64-bit Turnip Vulkan device enumeration, **32/32 health checks**, Wi-Fi, audio, SurfaceFlinger and GPU memory eBPF. **Long-duration gaming/thermal/resume stress tests and GPU work-period eBPF accounting remain incomplete.**

## Release assets

The GitHub Release contains a **vanilla boot** image, a read-only snapshot of the working EROFS vendor filesystem, the current vbmeta B image, APatch/Mountify graphics overlay ZIPs and `SHA256SUMS.txt`. The working device's **owner-personalized APatch-patched boot** is preserved privately, **not included in the public Release**, because the kernel patch was paired with the owner's secret key. Repatch the vanilla boot with your *own* APatch key and matching version, or retain your already-booting local kernel image.

> **Do not flash the release images blindly.** Confirm model `TB371FC`, A/B slot state, AVB/vbmeta policies, logical `vendor` partition size, stock firmware compatibility and full backups first. Slot A was deliberately left untouched during validation. The vendor image is an EROFS filesystem extracted from the *currently active logical vendor mapping*, not an entire super-partition image. Fastbootd/dynamic-partition flashing may require device-specific steps. ZIP modules require APatch + Mountify; simply placing binaries under `/vendor` is not supported.

For reversible OpenGL rollback on the verified device:

```sh
adb -s 6ab06f89 shell '/data/adb/apd module disable tb371fc-adreno-ogl-67668'
adb -s 6ab06f89 reboot
```

The private key, user data and personalized owner `boot_b` backup are **not published**. All upstream Qualcomm binaries retain their respective owners' rights. Do not assume VNDK30→31 replacement is compatible: the device and linker were intentionally kept at VNDK30.

Build sources include KGSL/Adreno GPU tracking, EROFS and TB371FC-specific display fixes. Compiler used: Android Clang `r353983c`, arm64. Build outputs and local development logs are not tracked.
