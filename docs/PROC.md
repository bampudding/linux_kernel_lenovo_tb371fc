# AREA: PROC — 복귀·되돌리기·수집 절차
> 조회: `grep -rh "ID:TB371FC-T.*-PROC" /root/kernel-lab/tb371fc/docs/PROC.md`. 전체는 HANDOFF 8절.

## [ID:TB371FC-T03-PROC-01] 전제 (T03)
- 기기 미연결 → 플래싱 불가. 복귀 전 읽기 조사만.

## [ID:TB371FC-T10-PROC-01] 패널 되돌리기 (T10)
- `source-k419167-nvt-interlock/.../dsi_panel.c` `false→true` 역치환 후 SHA `461d58e4...` 확인 (주석 제외 diff). `erofs-v54` 대조 유지.

## [ID:TB371FC-T12-PROC-01] 기기 복귀 순서 (T12)
1. `Mac:/opt/homebrew/bin/adb -s 6ab06f89`, `slot-b`, `boot_completed`, 현 `boot_b/vendor_b/vbmeta_b` SHA 백업 (`-s` 필수, P11 오플래시 금지).
2. 정상부팅→`off/on` 5회→`dmesg | grep -i "dsi_panel|TB371FC|nvt"`·`logcat | grep -i "SetPowerMode|nvt_fw"`·`/proc/nvt_fw_status` 수집.
3. 실패 시 `last_kmsg/pstore` 후 백업 이미지 원복, `dtbo` 덤프 후 실대조.

## [ID:TB371FC-T22-PROC-01] 패키징 체크리스트 (T22, 실행 없음)
- 조합: 신규 `Image.gz` (`source-k419167-nvt-interlock`+본 fix, 현 O= `.config` 유지, 동일 계열 DLKM) + `unpack_audiofix_boot/ramdisk` + `dtb` (3연결 그대로) + 기존 cmdline (`mkbootimg_args.txt`: header v2, `os 11.0.0/2024-02`, pagesize `0x1000`, `kernel_offset 0x8000`, `ramdisk_offset 0x1000000`, `dtb_offset 0x1f00000`).
- 도구: `Mac tb371fc-flash/mkbootimg.py` + `avbtool.py` (96M `_avb` 형태, sha256 계열 유지). `vendor_b`는 `vendor_tb371fc_erofs_audiofix_ctxmerged.simg` + 신규 DLKM 교체 후 `pack_vendor.sh` AVB sha256 (Mac `kernel-box`).
- 순서: `Image.gz`+`Module.symvers` diff(무변경 확인) → DLKM 서명 확인 → boot `_avb` 생성 → `vbmeta_disabled_verity` 그대로 → `fastboot -s 6ab06f89 flash boot_b/vendor_b/vbmeta_b` (slot-b).
- 금지: config 전환·계열 혼합·SHA-1 hashtree·전체 트리 SCP. `P11(-s HA1E02DA)` 오플래시 금지.
