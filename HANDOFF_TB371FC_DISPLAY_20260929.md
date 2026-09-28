# TB371FC Display Re-light — Session Record & Handoff (2026-09-29, KST)

> 원칙: 가설과 확인된 증거를 구분한다. 기기가 연결되어 있지 않으므로 플래시·adb 재현·실제 화면 검증을 주장하지 않는다.
> 기존 이미지·소스·로그를 보존하고 비파괴적으로 조사한다. P11 248~250 파이프라인은 재개하지 않는다.

## 0. 원작업 지시 (요약, 누락 없음)
- 대상: Lenovo TB371FC 화면 재점등 문제. Mac + fedora-heart 접근 가능. SSH 별칭·경로 추측 금지, 실제 확인.
- 목표: 부팅 직후 보이나 off→on 시 물리 화면 미표시 + 이전 우회 시 글리치. 둘을 별개로 숨기지 말고 패널 전원/리셋·DSI 재초기화·터치 FW·display HAL 상호작용으로 수정 후보 작성. 최신 벤더 통째 롤백 대신 현재 벤더에 맞는 수정.
- 제약: 기기 미연결 (플래시/adb/화면 검증 주장 금지). P11 4.19.185는 별도 기기 정상 기준. P11 248~250 중단, 재개 금지. 시스템·Incus 변경·대량 빌드·플래시·파티션 변경 불필요. 가설/증거 구분. 소스·벤더·부팅 이미지 대응 버전 먼저 확인.
- 이전 단서: Mac `/Users/heart/kernel-lab`, 커널 후보 `kernels/tb371fc`, 활성 `NT36532=y`/`NT36XXX not set` → `nt36532/` 우선. `nt36xxx_ext_proc.c /proc/nvt_fw_status`에 resume 후 `A0`+`fw_update_stat` 참이면 HAL에 `A3` 보고하는 interlock 존재 여부·정당성·글리치 관계 검토 (넓게 속이거나 제거 금지). 벤더 후보 `vendor-lab/tb371fc/stage-227-bootcheck/vendor-root`의 composer에서 `proc/nvt_fw_status` 확인됨, 실기기 벤더 동일 여부 재확인 필요. 부팅 후보 `/Users/heart/tb371fc-flash/`·`unpack_audiofix_boot/`, 이전 4.19.167 보고이나 대응 직접 검증. 부팅 dtb는 다중 DTB 연결형, 패널이 dtbo에 있을 수 있어 기본 DTB만으로 단정 금지. Android Display ON≠물리 ON (원격 wake 후 실화면 암전 확인 사례). fedora-heart 12GiB, 무제한 병렬 금지.
- 진행: 1) SSH·경로·변경사항 확인 2) 패널 모델·DTBO·off/on·전원/리셋·DSI/touch notifier 추적 3) composer 읽기 시점·대기 상태 (바이너리 읽기 전용) 4) 근거 확보 시 최소 수정+정적·제한 빌드, 완성 이미지 출처·조합 기록 5) 복귀 시 정상부팅→off/on 반복→글리치→로그·복귀 문서화. 먼저 파일·버전 관계+유력 실패지점 보고 후 진행. 종료 시 변경·근거·검증·미검증 구분 보고.
- 후속 지시: TB371FC 위주, adb 연결 체크 후 플래싱 검토. 고사양 작업 분담 질의 → heart 22 + lenovo 8 = 30 확정 (32 과다). distcc가 `/root/HDD` 참조 금지. 작업 개시 승인. 안정화 잔여 질의. 본 기록 지시.

## 1. 환경 (실측)
- 현재 호스트: `fedora-heart` (Debian container, `Linux 7.2.6+deb14`, `Mem 12GiB`, `nproc 24` = 전체 28 중 4 offline, Xeon E5-2697 v3).
- `fedora-lenovo` (`172.30.1.4`, `nproc 8`, i7-8565U, `Mem 19GiB + Swap 7GiB`).
- Mac SSH: `mac-workspace` (`127.0.0.1:40222`, `heart`, key `/root/.ssh/id_ed25519_mac_access`) 성공. `MacBook-Pro-418.local, Darwin 27.2.0 arm64`.
- Mac adb는 PATH에 없고 `/opt/homebrew/bin/adb (37.0.1)` 실체 확인. Fedora adb `37.0.0`.
- ADB 결과: 양쪽 `adb devices -l`·`fastboot devices` 빈 상태. `adb -s 6ab06f89` → `not found`. Fedora `/dev/bus/usb/*/*` 없음 (USB 패스스루 없음). 결론: TB371FC(`6ab06f89`)·P11(`HA1E02DA`) 모두 미연결 → 플래싱 불가.
- 저장소: `SSD_ONLY_BUILD_POLICY.md` 확인, `check-ssd-build-budget TB371FC PASS (65.03/80GiB, 여유 185GiB)`. 활성 `O=`·`M=`·`ccache(~/.cache/ccache)`·staging 모두 SSD. HDD는 아카이브 전용.

## 2. 핸드오프 문서 위치 (원문 유지, 본 기록은 요약)
- `/root/PROMPT_TB371FC.md`, `/root/PROMPT_P11.md`, `/root/kernel-lab/tb371fc/PROMPT_TB371FC.md` (원격 `-j30`, `22+8` 반영済).
  - TB371FC: `6ab06f89`, `slot-b`, `4.19.167`, `/`·`/vendor` erofs, 기준 `boot_tb371fc_k419167_erofs_v54_audiofix_avb.img (6da528f8...)` + `vendor_tb371fc_erofs_audiofix_ctxmerged.simg (476MB)` + `vbmeta_disabled_verity.img (Flags 2)`.
  - P11: `HA1E02DA`, `slot-a`, `4.19.157 + vendor v4` 정상. `4.19.207` 스플래시·kABI 과제. TB371FC dir 절대 금지 (역도 동일).
  - 공통: Mac 1순위 (`-j10`, `/Users/heart/kernel-lab ↔ /workspace`, `kernel-box`, `pack_vendor.sh` AVB sha256), 원격 2순위 (본 세션에서 `-j30` 확정, 산출물 `~25MB`만 SCP).
- `/root/CLUSTER_BUILD_GUIDE.md` ≒ `/root/distcc-cluster/README.md` (`-j30`, `localhost/22 + 172.30.1.4/8` 반영済).
- `/root/SSD_ONLY_BUILD_POLICY.md` (Incus 250GiB, 프로젝트별 80GiB, 여유 25GiB, HDD 아카이브 전용, 빌드 전 budget 실행).
- `/root/kernel-lab/tb371fc/AGENTS.md` (SSD-only, budget 80GiB, HDD 직접·심링크 사용 금지).

## 3. 버전 대응표 (확인)
| 대상 | 값 |
|---|---|
| 소스 3곳 | Mac `kernels/tb371fc` = Fedora `source-k419167-erofs-v54` = `source-k419167-nvt-interlock`. `Makefile 4.19.167`, `NT36532=y`. `ext_proc be64d833...`, `dsi_panel 461d58e4...` (수정 전) 일치 |
| 부팅 커널 | `unpack_audiofix_boot/kernel` gzip `#1 2026-09-27 21:23`, `Image.gz_nvt_interlock` `#2 21:58` (둘 다 `clang 9.0.3 r353983c`), `Image.gz_erofs_v54` `#1 13:25`. `gzip -dc \| strings`로 NVT 이미지에 `TB371FC resume interlock` 실포함 확인. V54는 `TB371FC` 포함이나 interlock 문자열 없음 (interlock 추가 전 빌드). `a3fix`는 interlock 없음 (별도 계열) |
| 벤더 | `stage-227 vendor-root/build.prop`: `TB371FC ZUI_16.0.474, spinel/kona, SDK30`. composer `stage-191`=`stage-227` 동일 `b968a1045c85ed20cced05fe743953b9fb39dcbb30e1719ed3caee461be374e2` (622K, `aarch64` stripped) |
| DTB | `unpack dtb 1.5M` = `tb-dtb0/1/2` 3연결. `dtb0 76a458e2...`, `dtb1 2a2ac9cc...`, `dtb2 ed944b0b...`. 차이는 SoC 리비전(`v2.1/v2/v1`, `msm-id`, 전압 테이블)뿐. 3종 모두 패널 노드 없음 → 패널은 `dtbo` 별도 파티션 (미확보) |
| 실기기 대응 | 미검증 (기기 없음). `stage-227`은 후보일 뿐 실기기 벤더와 동일 단정 금지 |

## 4. 핵심 발견 (증거)
### 4.1 터치 interlock (좁은 우회, 유지)
- `nt36532/nt36xxx_ext_proc.c:933 nvt_fw_status_show`, `966 if (buf[1]==A0 && bTouchIsAwake && fw_update_stat) → seq_printf A3 + NVT_LOG interlock`.
- `nt36xxx.c:3270 nvt_ts_resume`: `BOOT_UPDATE_FIRMWARE_NAME` 다운로드 후 `A3` 최대 1회 재시도 (`nvt_check_fw_reset_state`, `INIT 10회` vs `NORMAL 50회×10ms`). 그래도 `bTouchIsAwake=1` 후 공개.
- `notifier`: `DRM_PANEL_EARLY POWERDOWN→suspend`, `DRM_PANEL EVENT UNBLANK→resume` (패널 enable 이후). `pm_resume`은 `completion`만, FW 재다운 없음. `fw_update_stat`는 `nt36xxx_fw_update.c:863 =1`에서만 세팅.
- `staging display-source-167(-reset)`의 `ext_proc`에는 interlock 없음 (구간 933·965만). interlock 계열은 `source-k419167-*`+Mac 한정. 혼동 금지.

### 4.2 패널 전원 유지 (글리치 측, 수정 대상)
- `techpack/display/msm/dsi/dsi_panel.c:466 keep=true`, `468 power_on` retained shortcut (regulator/pinctrl/bias/reset 생략, `ON`만), `527 power_off` retained (레일 유지).
- 흐름: `prepare(power_on+PRE_ON)` → `enable(ON)` → `disable(OFF)` → `unprepare(POST_OFF+power_off)`.
- `staging reset/nvt-rek`는 이미 `keep=false`로 둠. 단 `reset`의 retained 분기 내 `dsi_panel_reset` 추가는 `keep=false`에서 dead code (무해하나 혼란). `nvt-rek`의 `nt36xxx.c` 변경 (`A3 재시도 → REK 대기`로 완화)는 역방향이라 본 계열에 미채택.

### 4.3 composer HAL 차단 (확인)
- 복사본 `/tmp/opencode/tb371fc/composer-service` (SHA 원본 동일, 읽기 전용 분석).
- `.rodata`: `0x1a56c "SetPowerMode "`, `0x1a57a "A3"`, `0x1a57d "proc/nvt_fw_status"`, `0x1a590 "GetVisibleDisplayRect"`. `0x2133b "HWCDisplay::%s: SetPowerMode failed. Error = %d"`.
- `0x70430` checker: `open("proc/nvt_fw_status", O_RDONLY) → read 20B → close → strncmp(buf,"A3",strlen=2)`. `0→705d0 return 1`, `!=0→ return 0`.
- 호출 `0x67ca8` (실패 시 `0x68024`로), `0x68070` (실패 시 `0x68040`으로 루프). `0x68064 usleep 100000` 후 재검사. 단기 타임아웃 없음 (관측 범위 내 무한 폴링 형태). `0x6634c`은 실패 로그 참조.
- 해석: `SetPowerMode` unblank 트랜잭션이 터치 `A3` 없이는 진행 안 됨. 커널 주석과 일치.

### 4.4 유력 실패 모델
- HAL이 `A3` 요구 + 터치가 `A0`에 머묾 + 패널 리셋 생략 → (우회 없으면) 검은 화면, (interlock+전원유지로 풀면) 리셋 없는 `ON`이라 글리치. `bTouchIsAwake` 조건 때문에 `SetPowerMode` 시점 1회 read vs 폴링 중첩의 정확한 선후는 호출부 전체 경계 미확정 (잔여).

## 5. 최소 수정 (구현済 1건)
- 대상 계열: `source-k419167-nvt-interlock` (현 부팅 interlock 계열). `erofs-v54`는 대조용으로 원래값 유지.
- 내용: `dsi_panel.c:465-469` 주석 교체 + `keep true→false`. 터치 interlock·`A3` 재시도는 손대지 않음 (넓게 속이지 않음, `nvt-rek`식 완화 거부).
- 효과: wake가 cold-boot와 동일한 regulator/pinctrl/bias/reset을 반복 → `ON` 정상 래치. HAL은 기존 좁은 interlock으로 단기 통과 후 실 `A3`로 수렴, 패널은 실리셋이라 글리치 해소 기대. `ESD recovery`와 동일 경로라 OEM 경로 보존.
- 조합 기록 (이미지 미생성): 신규 `Image.gz` 필요 시 `source-k419167-nvt-interlock(본 fix)` + `configs/tb371fc-k419167-erofs-v54.config` (`build/k419167-nvt-interlock/.config` 동등) + `unpack_audiofix_boot ramdisk/dtb0-2` + `stage-227` 동일 composer 벤더 + `avb sha256` 푸터.

## 6. 검증 (수행) vs 미검증
- 수행: `gzip -dc|strings` 3종, `sha256sum` 7건 이상, `dtc` 3종 역컴파일·diff, `llvm-objdump` 읽기 전용, `diff` 패널 1건, `checkpatch` (신규 ERROR 없음; `static false` 1건은 staging과 동일 패턴), `display-reset` 풀빌드 성공 실증 (`Image 42M·Image.gz 18M`, `09-28 10:13`, Error 없음)으로 동일 bool 변경 컴파일성 간접 입증. 직접 단일 `dsi_panel.o`는 techpack 직접 타깃 미지원+`vdso32` 환경 이슈로 중단 후 반복 안 함. `budget PASS`.
- 미검증: 실 `off/on`·글리치, `dtbo` 실대조, HAL 폴링 상한, 신규 Image 패키징·부팅. 주장 안 함.

## 7. 인프라 변경 (본 세션)
- `/etc/profile.d/distcc.sh`: `24+8=32 → 22+8=30` (`localhost/22`). `distcc --show-hosts` 확인.
- `distcc-cluster/README.md`, `CLUSTER_BUILD_GUIDE.md`, `PROMPT_TB371FC.md`×2, `PROMPT_P11.md`: `-j30` 반영.
- `tools/build-k419167-erofs.sh`: HDD 아카이브 옵트인 (`ARCHIVE_TO_HDD=1`일 때만 `mkdir/cp`, 기본 `no HDD touched`). `SRC/OUT` SSD 유지. `bash -n OK`.

## 8. 기기 복귀 시 절차 (그대로 실행)
1. `Mac:/opt/homebrew/bin/adb -s 6ab06f89` 확인, `slot-b`, `boot_completed`, 현 `boot_b (v54_audiofix_avb)`·`vendor_b (ctxmerged)`·`vbmeta_b (disabled)` SHA 백업.
2. 정상부팅→화면 `off/on` 5회→글리치 육안 + `dmesg | grep -i "dsi_panel|TB371FC|nvt"`·`logcat -b all | grep -i "SetPowerMode|nvt_fw"`·`cat /proc/nvt_fw_status` (raw `A0/A3` + `dmesg interlock` 여부) 수집.
3. 실패 시 `last_kmsg/pstore` 확보 후 백업 이미지로 `fastboot flash boot_b/vendor_b/vbmeta_b` 원복. `dtbo` 파티션 덤프 후 패널 `on/off·reset/vddio` 실대조.
4. `dtbo` 확보 전에는 기본 DTB만으로 패널 단정 금지. `P11` 기기(`HA1E02DA`)에 오플래시 금지 (`-s` 필수).

## 9. 잔여 안정화 (플래싱 전 가능)
1. 패널 실체: `recovery_*_dtbo.img`·`Downloads/Lenovo*`·`dts/qcom/*overlay*`·`techpack/display` compatible 대조, `on/off`·타이밍표.
2. HAL: `0x67ca8/0x68070` 함수 경계·상한, `0x2133b` 실패 경로 확정.
3. 터치: `EARLY vs EVENT`·`pm` 실등록 경로, `fw_update_stat`·REK 거부 근거 문서화.
4. DLKM: `audio v54·qca·nt` `Module.symvers`·`.config` diff (kABI·스플래시 방지).
5. 패키징·복귀: `mkbootimg avb sha256` 조합·수집 스크립트·원복 순서. 빌드는 `-j4` 이하·단일 검증만.

## 10. 증거 원본 경로 (재확인용)
- Mac: `/Users/heart/kernel-lab/kernels/tb371fc/{Makefile,drivers/input/touchscreen/nt36532/*,techpack/display/msm/dsi/dsi_panel.c}`, `/Users/heart/kernel-lab/vendor-lab/tb371fc/stage-{227,191}-bootcheck/vendor-root/{build.prop,bin/hw/vendor.qti.hardware.display.composer-service}`, `/Users/heart/tb371fc-flash/{Image.gz_*,boot_*_avb.img,vendor_*.simg,unpack_audiofix_boot/{kernel,ramdisk,dtb,mkbootimg_args*},unpack_a3fix/}`, `/Users/heart/kernel-lab/tb-dtb{0,1,2}.dtb`.
- Fedora: `/root/kernel-lab/tb371fc/{source-k419167-erofs-v54,source-k419167-nvt-interlock,build/k419167-nvt-interlock,staging-419325-20260927/{display-source-167{,-reset,-nvt-rek},display-out-167{-reset,-nvt-rek},logs/display-{reset,nvt-rek}167-build.log},configs/*,tools/build-k419167-erofs.sh}`, `/tmp/opencode/tb371fc/{composer-service,dtb0.dtb,dtb0-2.dts}` (분석 복사본, SHA 대조済).
