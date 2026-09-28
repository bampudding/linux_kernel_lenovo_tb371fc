# AREA: VER — 버전 대응 (소스·부팅·벤더)
> 조회: `grep -rh "ID:TB371FC-T.*-VER" /root/kernel-lab/tb371fc/docs/VER.md`. 전체 표는 HANDOFF 3절.

## [ID:TB371FC-T01-VER-01] 초기 대응 (T01/T08)
- 소스 3곳 동일: Mac `kernels/tb371fc` = `source-k419167-erofs-v54` = `source-k419167-nvt-interlock` (`4.19.167`, `NT36532=y`, `ext_proc be64d833...`, `dsi_panel 461d58e4...` 수정 전).
- 부팅 3종 `4.19.167-perf clang9.0.3`: `unpack 21:23 #1`, `nvt_interlock 21:58 #2` (interlock 문자열 실포함), `v54 13:25` (interlock 전), `a3fix` 별도 계열.
- 벤더 `ZUI_16.0.474 spinel/kona SDK30`, composer `191=227 b968a104... 622K aarch64`.
- 실기기 대응 미검증, `stage-227` 후보로만 취급.

## [ID:TB371FC-T10-VER-01] Image 계열 확정 (T10)
- 현 부팅 interlock 계열 = `source-k419167-nvt-interlock`. `erofs-v54`는 대조용 유지. `staging display-source-*`는 별도 계열 (interlock 없음)이라 혼동 금지.
