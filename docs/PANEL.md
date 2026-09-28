# AREA: PANEL — 패널 전원·리셋·DSI
> 조회: `grep -rh "ID:TB371FC-T.*-PANEL" /root/kernel-lab/tb371fc/docs/PANEL.md`. 전체는 HANDOFF 4.2·5절.

## [ID:TB371FC-T08-PANEL-01] 전원 유지 발견 (T08)
- `dsi_panel.c:466 keep=true`, `468 power_on` retained shortcut (regulator/pinctrl/bias/reset 생략, ON만), `527 power_off` retained.
- 흐름 `prepare(power_on+PRE_ON)→enable(ON)→disable(OFF)→unprepare(POST_OFF+power_off)`.

## [ID:TB371FC-T10-PANEL-01] staging 대조 (T10)
- `display-167 keep=true` vs `reset/nvt-rek keep=false`. `reset`의 retained 분기 내 reset 추가는 `keep=false`에서 dead (무해·혼란). 본 수정은 dead 코드 추가 없이 단일 bool만 변경.

## [ID:TB371FC-T10-PANEL-02] 최소 수정 구현 (T10)
- 대상 `source-k419167-nvt-interlock/.../dsi_panel.c` `465-469` 주석 교체 + `keep true→false`. 전 `461d58e4...` → 후 `89b1856f...`. `erofs-v54` 대조 유지. 터치 interlock 유지.
- 효과: wake cold-boot 동일 시퀀스 반복 → ON 정상 래치, ESD와 동일 OEM 경로. 신규 Image 조합은 HANDOFF 5절에 기록, 미생성.
- 되돌리기: `sed false→true` 후 SHA `461d58e4...` (주석 제외 diff 확인), 상세는 TURN_LOG T10.
