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
  certs/tb371fc-oem-and-wifi-trust.pem \
  certs/tb371fc-live-1010-module-signer.pem \
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
# Module signature enforcement requires the actual 2026-10-10 vendor signer
# to be present in the built-in trust bundle (all certs have identical subjects).
if ! python3 - "$root/certs/tb371fc-oem-and-wifi-trust.pem" "$root/certs/tb371fc-live-1010-module-signer.pem" <<'CERT_AUDIT'
import base64,hashlib,pathlib,re,sys
pat = re.compile(rb"-----BEGIN CERTIFICATE-----\s*(.*?)\s*-----END CERTIFICATE-----",re.S)
def hashes(path):
    out=[]
    for payload in pat.findall(pathlib.Path(path).read_bytes()):
        data=base64.b64decode(re.sub(rb"\s+", b"", payload), validate=True)
        out.append(hashlib.sha256(data).hexdigest())
    return out
expected="fc596fd06129729a28f76403996d8c3ead421660a411169cfd5cfb5ca0e4db45"
known=hashes(sys.argv[2]); bundle=hashes(sys.argv[1]);
if known!=[expected] or expected not in bundle:
    print("ERROR: verified 1010 vendor module signer certificate missing/changed",file=sys.stderr)
    sys.exit(1)
print(f"PASS: verified 1010 module signer certificate is trusted ({len(bundle)} certificates).")
CERT_AUDIT
then status=1; fi
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
