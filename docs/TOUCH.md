# AREA: TOUCH — 터치·interlock·resume
> 조회: `grep -rh "ID:TB371FC-T.*-TOUCH" /root/kernel-lab/tb371fc/docs/TOUCH.md`. 전체는 HANDOFF 4.1절.

## [ID:TB371FC-T01-TOUCH-01] 단서 (T01)
- `nt36532/nt36xxx_ext_proc.c /proc/nvt_fw_status`, resume 후 `A0`+`fw_update_stat` 참이면 HAL에 `A3` 보고. 포함 여부·정당성·글리치 관계 검토, 넓게 속이거나 제거 금지.

## [ID:TB371FC-T08-TOUCH-01] interlock 확인 (T08/T10)
- `ext_proc.c:933 show`, `966 if (A0 && bTouchIsAwake && fw_update_stat) → A3 + LOG interlock`. 3개 소스 동일.
- `nt36xxx.c:3270 resume`: FW 다운로드 후 `A3` 최대 1회 재시도 (`INIT 10회` vs `NORMAL 50회×10ms`), 그래도 `bTouchIsAwake=1`.
- notifier: `EARLY POWERDOWN→suspend`, `EVENT UNBLANK→resume` (패널 이후). `pm_resume`은 completion만. `fw_update_stat`는 `fw_update.c:863`에서만 `=1`.
- `staging display-source-167(-reset)` ext_proc에는 interlock 없음. interlock 계열은 `source-k419167-*`+Mac 한정.

## [ID:TB371FC-T10-TOUCH-01] nvt-rek 기각 (T10)
- `display nvt-rek`의 `A3 재시도→REK(A1) 대기` 완화는 역방향이라 미채택. 본 수정은 터치 측 손대지 않음.

## [ID:TB371FC-T18-TOUCH-01] 순서 확정 (T18)
- 활성 `.config`: `CONFIG_DRM_PANEL=y` → `#if` 체인에서 `drm_panel_notifier`만 등록. `fb/early_suspend` 분기는 컴파일 제외 (`CONFIG_FB=y`여도 마찬가지).
- SDE 발사점: `EARLY_EVENT_BLANK` = `sde_kms.c:1019 prepare_commit` (HW kickoff·panel enable 이전), `EVENT_BLANK` = `:1204 complete_commit` (`post_kickoff`·첫 프레임 이후).
- 확정 순서: suspend는 display-off 이전(정상), resume(FW 다운로드→A3)는 display-on+ 첫 kickoff 이후. HAL ON 폴링이 atomic commit 자체를 게이트하면 resume이 영원히 안 돌아 검은 화면 무한 대기. commit이 진행된 뒤 폴링이면 resume이 complete에서 돌아 A3(실측 또는 interlock) 보고 후 HAL 진행 — 이때 패널이 `keep=true`면 리셋 생략 글리치, `keep=false`면 prepare에서 실리셋이라 정상. 본 fix와 일치.
