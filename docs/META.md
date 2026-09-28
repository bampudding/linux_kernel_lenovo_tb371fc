# AREA: META — 핸드오프·정책·ID 체계
> 조회: `grep -rh "ID:TB371FC-T.*-META" /root/kernel-lab/tb371fc/docs/META.md`.

## [ID:TB371FC-T12-META-01] 문서 위치 (T12)
- 원문: `/root/PROMPT_TB371FC.md`, `/root/PROMPT_P11.md`, `/root/kernel-lab/tb371fc/PROMPT_TB371FC.md`, `/root/CLUSTER_BUILD_GUIDE.md`, `/root/distcc-cluster/README.md`, `/root/SSD_ONLY_BUILD_POLICY.md`, `/root/kernel-lab/tb371fc/AGENTS.md`.
- 본 세션: `HANDOFF_TB371FC_DISPLAY_20260929.md`, `RECORDING_POLICY.md`, `TURN_LOG_20260929.md`, `docs/{ENV,VER,TOUCH,PANEL,HAL,DTB,BUILD,PROC,META}.md`, `docs/INDEX.md`, `docs/query.sh`.

## [ID:TB371FC-T13-META-01] ID 체계 (T13~)
- 형식 `[ID:TB371FC-T<NN>-<AREA>-<SEQ>]`. `TNN`으로 턴 전체, `-AREA-`로 영역별 조회. 예: `grep -rh "TB371FC-T10" docs/` (T10 전부), `grep -rh "-HAL-" docs/HAL.md` (HAL 전부).
- 새 턴은 해당 영역 파일에 추가하고 `INDEX.md` 1줄 추가한다. 새 날짜는 `TURN_LOG_YYYYMMDD.md`·`HANDOFF_..._YYYYMMDD.md`를 새로 만들고 기존을 덮어쓰지 않는다.
