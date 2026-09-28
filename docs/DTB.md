# AREA: DTB — dtb·dtbo·패널 노드
> 조회: `grep -rh "ID:TB371FC-T.*-DTB" /root/kernel-lab/tb371fc/docs/DTB.md`. 전체는 HANDOFF 3·4절.

## [ID:TB371FC-T01-DTB-01] 전제 (T01)
- 부팅 dtb 다중 연결형, 패널이 dtbo에 있을 수 있어 기본 DTB만으로 단정 금지.

## [ID:TB371FC-T08-DTB-01] 역컴파일 (T08/T10)
- `unpack dtb 1.5M` = `tb-dtb0/1/2` 3연결 (`dtb0 76a458e2...`, `dtb1 2a2ac9cc...`, `dtb2 ed944b0b...`). Fedora `dtc`로 `26096`줄 역컴파일.
- 차이 SoC 리비전만 (`v2.1/v2/v1`, `msm-id`, 전압). 3종 모두 `mdss_dsi0/1` 있으나 패널 노드 없음 → 패널은 `dtbo` 별도 (미확보). 부팅 이미지에 dtbo 없음.
- `Mac tb-dtb0.dtb` → `/tmp/opencode/tb371fc/dtb0.dtb` 복사 분석 (SHA 대조), 원본 무수정.
- 잔여: `recovery_*_dtbo.img`·`Downloads`·`dts/qcom/*overlay*` 대조, `on/off·reset/vddio` 표, 기기 복귀 시 `dtbo` 덤프.

## [ID:TB371FC-T19-DTB-01] 터치·전원 노드 전수 (T19)
- `compatible` 831건 중 `nt3/novatek/touchscreen/panel@/labibb/ktz8866/backlight` 0건 — 3개 DTB 모두 동일.
- SPI 컨트롤러 20개 전수: 자식 노드 0개, `ok`는 `se14(880000)` 1개뿐(자식 없음). 나머지 19개 `disabled`.
- `reset-gpio` 적중 4건은 `tfa98xx@34~37` (오디오 앰프, 터치 아님).
- 결론: 터치 SPI 클라이언트(`reset_gpio`·IRQ)·패널·백라이트·labibb 전원은 전부 DTBO 제공. `keep=false` fix는 DTBO 정의를 매 wake마다 그대로 재생하므로 DTBO 내용과 무관하게 안전. `on/off cmds`·`reset/vddio` 실값은 DTBO 확보 후 대조 (복귀 절차 유지).

## [ID:TB371FC-T20-DTB-01] 소스 측 패널 후보 소진 (T20, negative result)
- `arch/arm64/boot/dts/qcom/`에 spinel/TB371FC 없음 (상류 DTS만, 하류 DTB는 프리빌트). `techpack/display`에 spinel 언급 없음.
- `.config`: `CONFIG_DRM_PANEL=y`만, 인트리 패널 드라이버 전부 `not set` → 패널은 제네릭 DSI 파서+DTBO 기술로만 동작.
- 결론: 소스에서 패널 compatible·on/off cmds를 찾을 길 없음. DTBO 덤프(복귀 시 `dtbo` 파티션)가 유일 경로. 대조표 양식은 PROC 복귀 순서에 이미 포함.

## [ID:TB371FC-T23-DTB-01] DTBO 실측 — 기본 패널 확정 (T23, HDD 순정 dtbo_a)
- 출처: `/root/HDD/user0/tb371fc/backups/device-current-2026-09-20/partitions/dtbo_a.img` (25M, 읽기 전용, 원본 무수정). mkdtbo 헤더+14 overlay, `/tmp/opencode`에 분할·역컴파일.
- 정정: 기본 패널은 sw43404 AMOLED가 아님 (동일 오버레이 내 타 SKU). `qcom,dsi-default-panel` = spinel BOE/Tianma **nt36532 3K video** (dual-DSI `0x5c0×0x730`×2 = 2944×1840, DSC 1.1, burst, 120/144/60/30 dfps).
- BOE 노드 `dsi_panel_spinel_boe_nt36532_dsc_3k_video`: reset `<1/10ms 0/10ms 1/10ms 0/10ms 1/10ms>`, ON은 LP·말미 `11(sleep-out)+29(display-on)`, OFF는 `28+10` HS. dual ctrl/phy.
- **TDDI 일체형**: nt36532 칩이 DSI(표시)+SPI(터치 FW) 공유, 리셋선 공유. `keep=true` 스킵 → 칩 미재기동 → 글리치+터치 A0(→interlock 거짓말 필요). `keep=false` → 리셋 재생 → ON 래치+실 A3. 본 fix의 DTBO 근거 완성.
- 잔여: BOE vs Tianma 실장치 구분 (둘 다 default 후보, 리셋·ON 동형이라 fix 영향 없음), 터치 SPI 노드 위치 (표시-터치 결합상 필수는 아님).
