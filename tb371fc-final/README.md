# TB371FC Final Verified Stack — 2026-10-10

## Finalized hardware configuration

**Device:** Lenovo TB371FC (Snapdragon 870/Adreno 650v3), active slot B.

**Kernel:** `4.19.325-perf+ #6` (built from a **Linux 4.19.235-derived** source; `325` is the version string, not a fully backported 4.19.325 LTS security baseline). GPU memory eBPF tracepoint and accounting restored; KGSL DMA-BUF cleanup and A6xx performance-register list bounds checks backported. APatch KernelPatch `d03` has been validated on the owner's boot image.

**Vendor:** real working EROFS logical vendor snapshot with **VNDK30**, not a fictional VNDK31 migration. TB371FC panel DSC and touch fixes preserved. SQE/GMU firmware kept at the stable Lenovo version.

**Graphics:** Qualcomm **OpenGL ES V@0676.68** for 32/64-bit apps (2025-02-17); Qualcomm v676.68 Vulkan for 32-bit (API 1.1.128); **Mesa Turnip 26.3.0-devel** for 64-bit Vulkan (API 1.3.363). An ION-backed `libdmabufheap` shim and matched EGL subdriver make Qualcomm v676.68 work with the legacy VNDK30 vendor libc++. All drivers are applied **systemlessly** by APatch + Mountify.

**Real-world checks:** Android boot and reboots, 32-/64-bit OpenGL pbuffer render/readback, 20 repeated render passes, both Vulkan devices, GPU memory eBPF reports, 32/32 health checks, Wi-Fi/audio/display functional. **Not yet validated:** long-duration gaming load, battery life, 144 Hz panel mode, `GPU work information` per-UID tracepoint and BPF program compatibility.

## Assets and usage

`boot-b-final-vanilla.img` — kernel and original RAMDISK/DTB and hash footer, **not rooted**. To use APatch safely, patch with your own SuperKey and a compatible APK-matched KernelPatch image. The owner's private pre-patched boot is **not publicly distributed**.

`vendor-b-final-erofs.img` — extracted and hashed EROFS filesystem data from the running TB371FC `/vendor` logical volume. **Not** an entire `/super` image; treat flashing as device-specific fastbootd/dynamic partition work. Do not use on P11.

`vbmeta-b-final.img` — exact bytes of current TB371FC vbmeta_b, backup and inspection aid; AVB policy must be reviewed before any reflash.

`TB371FC-Turnip-26.3.0-20261008-r2-APatch.zip` — 64-bit Vulkan Turnip overlay.

`TB371FC-Adreno-v676.68-ION-Compat-APatch.zip` — 32/64-bit OGL v676.68, 32-bit Qualcomm Vulkan, ION C ABI shims; preserves Lenovo SQE and 64-bit Turnip.

`mountify-204-install.zip` — required APatch metamodule. As tested, APatch may turn `system/vendor` into a symlink. For Mountify, convert that link to a **real directory copy inside the module** before reboot, as detailed in `tb371fc-final/finalize_mountify.sh`.

`SHA256SUMS.txt` — hashes of public release assets.

**Do not install this release on other hardware.** Save the current boot image and active vendor/vbmeta first; never overwrite golden slot A merely for a GPU update.

The only supported rollback for the OGL overlay is to disable `tb371fc-adreno-ogl-67668` and reboot. Original Qualcomm OGL will reappear, with Turnip 64-bit Vulkan unchanged.

## 2026-10-10 post-release source and hardware safety correction

The previous GitHub source release omitted all `techpack/` drivers due to a `.gitignore` error. The corrective source branch `fix/tb371fc-complete-build-inputs-20261010` adds that code, a pinned public known-good DTB, the original running kernel module signer **public X.509 certificate**, a clean-clone build check, and a strict vendor module ABI audit. See `tb371fc-final/REPRODUCIBILITY.md`. The new clean clone built both `Image.gz` and a complete `Module.symvers`, with symbol CRCs identical to the 2026-10-10 release. Full vendor DTS source remains missing. No rebuilt kernel has been boot-tested.

**The experimental Adreno v676.68 OpenGL module must remain disabled** on the currently booted device: it was found to crash Qualcomm camera OpenCL (`libCB.so`) and OMX (`libgsl.so`). Both cameras recovered when the module was disabled, while SELinux remained Enforcing. Do not rely on the earlier graphics-only tests to claim camera or media compatibility.
