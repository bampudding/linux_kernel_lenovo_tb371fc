#!/usr/bin/env bash
# TB371FC source-release integrity gate. Never flashes a device.
set -euo pipefail
root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd -P)"
cd "$root"
status=0
for file in \
  techpack/Kbuild \
  techpack/audio/Makefile \
  techpack/camera/Makefile \
  techpack/display/Makefile \
  techpack/display/msm/dsi/dsi_display.c \
  techpack/video/Makefile \
  tb371fc-final/kernel.config \
  tb371fc-final/prebuilt/tb371fc-known-good-dtb.bin; do
  if [[ ! -s "$file" ]]; then
    printf 'ERROR: missing mandatory build/packaging input: %s\n' "$file" >&2
    status=1
  elif git rev-parse --is-inside-work-tree >/dev/null 2>&1 && ! git ls-files --error-unmatch -- "$file" >/dev/null 2>&1; then
    printf 'ERROR: input exists locally but is not tracked by Git: %s\n' "$file" >&2
    status=1
  fi
done
if [[ -s tb371fc-final/prebuilt/tb371fc-known-good-dtb.bin ]]; then
  expected=cb0f50f13fd70b5dcd0b12d330ae09d070ae4d6f20f4873b1a9815883726ee0e
  actual="$(sha256sum tb371fc-final/prebuilt/tb371fc-known-good-dtb.bin | cut -d' ' -f1)"
  if [[ "$actual" != "$expected" ]]; then
    printf 'ERROR: trusted DTB differs from verified boot image (%s).\n' "$actual" >&2
    status=1
  fi
fi
if [[ -L arch/arm64/boot/dts/vendor && ! -e arch/arm64/boot/dts/vendor ]]; then
  printf 'WARN: vendor DTS symlink is dangling: cannot recreate DTBs from DTS sources.\n'
  printf '      Packaging MUST use the pinned known-good DTB, not a guessed DTB.\n'
  if [[ "${1:-}" == --strict-dts ]]; then status=1; fi
fi
if grep -qx 'CONFIG_MODULE_SIG_FORCE=y' tb371fc-final/kernel.config; then
  printf 'WARN: CONFIG_MODULE_SIG_FORCE=y: all external modules need a matching trusted signer.\n'
  printf '      A fresh build may generate a different signing key. No private keys are shipped.\n'
fi
if [[ "$status" -eq 0 ]]; then
  printf 'PASS: tracked TB371FC source and reference DTB inputs found.\n'
else
  printf 'FAIL: release cannot be accepted as source-complete.\n' >&2
fi
exit "$status"
