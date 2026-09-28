# AREA: BUILD — distcc·인프라·빌드·SSD
> 조회: `grep -rh "ID:TB371FC-T.*-BUILD" /root/kernel-lab/tb371fc/docs/BUILD.md`.

## [ID:TB371FC-T06-BUILD-01] 상한 합의 (T06)
- `22+8=30`, `-j30` 상한, 커널 `-j24~28` 시작, `12GiB`·무응답 사례로 무제한 금지.

## [ID:TB371FC-T07-BUILD-01] 적용 (T07)
- `/etc/profile.d/distcc.sh` `localhost/24→/22`, 문서 5건 `-j32→-j30`, `tools/build-k419167-erofs.sh` HDD 옵트인 (`ARCHIVE_TO_HDD=1`만, 기본 무참조, `SRC/OUT` SSD).
- 검증: `show-hosts`, `grep j32` 0건, distcc HDD 0건, `bash -n OK`, `budget PASS`.
- 되돌리기: TURN_LOG T07 역치환 명령.

## [ID:TB371FC-T10-BUILD-01] 패널 검증 (T10)
- `diff` 1건, `checkpatch` 신규 ERROR 없음, `display-reset` 풀빌드 성공 실증 (`Image 42M·Image.gz 18M`, `09-28 10:13`). 직접 단일 `dsi_panel.o`는 techpack 타깃 미지원+vdso32로 중단 후 반복 안 함. `budget PASS`. `/root/HDD` 기본 미접촉, `-j4` 이하 준수.

## [ID:TB371FC-T21-BUILD-01] DLKM 커플링 사전대조 (T21, 빌드 없음)
- `Module.symvers`: `nvt-interlock` 767K (`1fea6867...`) vs `erofs-v54` 668K (`b517d1ad...`) — 상이. 예: `apr_register`는 nvt 측에만 (`techpack/audio` 모듈셋 차이). 계열 혼합 금지.
- `.config` 102줄 차이: nvt O= 측은 tweaked (`HZ_300/BFQ/BBR/FQ_CODEL/ZRAM_DEDUP/EROFS`, `CLANG 90003`) vs 정석 v54 (`90001`, JUMP_LABEL 없음). 신규 Image는 현 O= `.config` 유지 (config 전환 금지).
- 본 fix 영향: built-in `static bool` 1건 → export·symvers 영향 0 (빌드 시 symvers diff로 최종 확인 예정).
- 패키징 규칙: 신규 `Image.gz`는 동일 계열 O= 산출 DLKM (`qca/audio/nt`, `Module.symvers` 동봉)과만 조합. `nt36532`는 built-in(`y`)이라 모듈 갱신 불필요. 모듈 서명(`certs/tb371fc-stock-module-signing`) 유지.
