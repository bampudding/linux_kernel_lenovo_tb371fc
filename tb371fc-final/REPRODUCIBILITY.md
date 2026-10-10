# TB371FC 2026-10-10 source integrity and kernel ABI gate

**2026-10-10, TB371FC only.** This document does not apply to Lenovo TB-J606F.

## Fixed release error

The previous GitHub source tree and Git bundle omitted `techpack/` because its top-level `.gitignore` excluded `techpack/`. This directory holds essential Qualcomm display, camera, audio, and video code. The kernel top-level Makefile requires `techpack/` in `drivers-y`; building only from a local worktree falsely masked the omission. The source-fix branch removes that ignore rule and tracks the entire source directory.

The checked-in symlink `arch/arm64/boot/dts/vendor` currently has a **missing external target**. The exact vendor DTS sources have not been recovered and must not be substituted speculatively. As a controlled packaging fallback, `prebuilt/tb371fc-known-good-dtb.bin` is the 1,609,107-byte DTB extracted from the **public** `boot-b-final-vanilla.img` release. Its SHA256 `cb0f50f13fd70b5dcd0b12d330ae09d070ae4d6f20f4873b1a9815883726ee0e` matches the actual working TB371FC boot B DTB. This is a binary *reference*, not restored editable DTS source.

Run `bash tb371fc-final/source-audit.sh` in a fresh checkout; `--strict-dts` additionally requires original DTS source availability and therefore currently fails intentionally.

## Current proven reference

- 2026-10-10 booting kernel banner: `4.19.325-perf+ #6 SMP PREEMPT Sat Oct 10 13:46:31 KST 2026`.
- Running APatch d03 Image, when unpatched using kptools: length 37,429,264 bytes, SHA256 `4c5226d10ebc60761c17d51418dac18c90d02ed7089749f4ca8a5066b80be66c`.
- Embedded kernel config for normal and freshly built images matches SHA256 `dd54df4f9cd987d361d6640283d3327b07d038206088ea9deb05edf4ba94f5d7`.
- Historical original *source build path* recorded in current 2026-10-10 kernel: `/root/kernel-lab/tb371fc/staging-419325-20260927/checkpoint-source-235-gpu-modern`. The **0927 part is an old directory name, not the build date**.
- Restore source path + original build timestamp and built-in CPIO timestamp 13:41:11 KST still did **not** reproduce original Image hash. This establishes that **source content, build environment, or other link inputs differ**; do not declare the discrepancy fixed solely by correcting a path.

## Build and flashing safety

- Build in an isolated `O=` output with Android Clang r353983c, correct cross compiler, and `tb371fc-final/kernel.config`, and preserve exact outputs, `Module.symvers`, and toolchain version.
- `CONFIG_MODULE_SIG_FORCE=y`, `CONFIG_MODULE_SIG_ALL=y`, `CONFIG_MODVERSIONS=y`. A clean build can generate a fresh `certs/signing_key.pem`; an old vendor module signed by a different key may not load unless its public signer is already trusted. For production, align boot kernel, trusted certificates, module CRCs, signed external modules, and vendor partition; never ship a generated one-off signing key.
- Do not use a kernel compiled from an incomplete GitHub snapshot, do not mix rebuilt kernel with unverified vendor/DLKM modules, and do not flash `boot_a` (known-good fallback). Changes to `boot_b` require verified recovery and an explicit real-device boot test.
- SELinux Enforcing is already working on the device with separately verified least-privilege HQKit policy. This source patch must not reset boot arguments to `androidboot.selinux=permissive`.
