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

## Independent clean-clone and vendor module CRC results

- Fixed source commit `ae3ad5859` was checked out into an empty working directory. `source-audit.sh` passed, with explicit warnings for the missing editable vendor DTS and forced module signing.
- `make Image.gz` and `make modules` both **completed successfully** with Android Clang r353983c and the pinned `tb371fc-final/kernel.config`.
- The newly generated **complete** `Module.symvers` has 13,647 entries and SHA256 `773533854d010561b25f8cd5ad62ade8f291e3b26c3b19c7cd1572b656214418`. It is **byte-for-byte identical** to the one published with the previously verified 2026-10-10 release.
- `verify_module_abi.py` audits the `__versions` section of each actual vendor `.ko` against this complete symbol table. The installed `/vendor/lib/modules` contains 44 files (all report vermagic `4.19.231-perf+` despite the working kernel being 4.19.325). The tool checked **3,621** imported versioned symbols, with **277** CRC mismatches spread over **7** modules:
  - `qca_cld3_qca6490.ko`: 190 mismatches (including `module_layout`)
  - `rmnet_shs.ko`: 31 mismatches
  - `rmnet_perf.ko`: 27 mismatches
  - `wigig_sensing.ko`: 12 mismatches
  - `tcp_bic.ko`: 6 mismatches
  - `tcp_westwood.ko`: 6 mismatches
  - `tcp_htcp.ko`: 5 mismatches
- The remaining 37 files (including `qca_cld3_qca6390.ko` and audio modules) have **zero** CRC mismatches against the new build. `CONFIG_MODVERSIONS=y` means a differing vermagic *alone* does not prove incompatibility: CRCs are the relevant check.
- The newly built `tcp_bic.ko`, `tcp_westwood.ko`, `tcp_htcp.ko` and `wigig_sensing.ko` are candidates to replace their incompatible old counterparts, but have **not** been installed; their signing keys are tied to this particular fresh build. The three other incompatible vendor modules require a separate matching-source rebuild or exclusion when packaging a new kernel/vendor combination.
- Until all intended-to-load modules pass both CRC and signature checks, do **not** flash the newly compiled kernel or rebuild/replace the current live vendor image. The 7 mismatching legacy files are not evidence that all 7 are loaded; loading status must be checked separately.

Run the compatibility gate against the actual vendor package being considered:

```bash
python3 tb371fc-final/verify_module_abi.py /path/to/full/Module.symvers /path/to/vendor/modules
```

This gate exits nonzero if any module has missing exports or mismatched CRCs; use `--report-only` only for diagnosis.

## Current device graphics compatibility correction

The experimental Adreno v676.68 OpenGL overlay has been **disabled** after repeated camera OpenCL and media OMX crashes. With the stock `libgsl.so` restored, both camera 0 and camera 1 generated hundreds of preview frames and the device remained SELinux Enforcing. Do not describe the optional OpenGL overlay as camera-/media-safe without a new full-stack integration test.

## Root-cause fix: restore missing 2026-10-10 module signing certificate

**Critical additional finding**: all 44 real vendor `.ko` files carry a PKCS#7 signer certificate serial `901B2A1FDFBE332F3F52A9299000EAC92E063C`. Before this correction, the clean-rebuild source's `certs/tb371fc-oem-and-wifi-trust.pem` had only three other public X.509 certificates; a fresh Kbuild generates a *different* temporary signer. With `CONFIG_MODULE_SIG_FORCE=y`, the new kernel would not recognize modules signed by the original #6 kernel, even if CRCs were compatible. This is a credible direct mechanism for kernel-rebuild boot/peripheral failures.

The exact fourth **public** X.509 certificate was recovered from the already published unrooted `boot-b-final-vanilla.img` Image and cross-checked against the owner-private APatch kernel (the unpatched kernels have identical SHA256). Its SHA-256 fingerprint is `FC:59:6F:D0:61:29:72:9A:28:F7:64:03:99:6D:8C:3E:AD:42:16:60:A4:11:16:9C:FD:5C:FB:5C:A0:E4:DB:45`. Using its public certificate to verify the *actual vendor module* PKCS#7 signature with OpenSSL CMS **succeeded** (`CMS Verification successful`). The certificate was added to the tracked `certs/tb371fc-oem-and-wifi-trust.pem` trust bundle, *without disabling* `CONFIG_MODULE_SIG_FORCE` or exposing any private signing key. `source-audit.sh` checks the DER fingerprint in both the standalone certificate and bundled trust file.

This preserves forced module signature verification while accepting the existing vendor module signer. Matching CRCs remain a separate mandatory condition; the seven incompatible modules listed above are still excluded from any safe update until rebuilt or otherwise resolved. A device boot test of a completely new kernel has **not** been performed merely from this certificate correction.

## Safe offline module remediation, NOT deployed

After a clean-clone `make modules_install` into an **isolated `INSTALL_MOD_PATH`**, the four in-tree replacements `tcp_bic.ko`, `tcp_westwood.ko`, `tcp_htcp.ko`, and `wigig_sensing.ko` were correctly signed by the fresh kernel's **own** generated signing certificate. The private signing key remains on the build server and is not copied into Git or the report. A staged 41-module set was constructed from these 4 and the 37 original vendor modules with already matching CRCs. **All 41 PKCS#7/CMS signatures cryptographically verified** against the two public certificates actually embedded in the candidate kernel; **3,026/3,026** imported versioned symbol CRCs also match. The other 3 mismatching and currently unnecessary modules were excluded (`qca_cld3_qca6490.ko`, `rmnet_shs.ko`, `rmnet_perf.ko`).

Offline staging path, **not on device**:

`/root/kernel-lab/tb371fc/vendor-abi-compatible-staging-20261010/`

Contains `MANIFEST.txt` with SHA-256s and explicit nondeployment warning. This does **not** resolve missing external source for the excluded modules, and a boot test with this matching kernel/vendor set is still outstanding.
