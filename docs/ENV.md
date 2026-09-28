# AREA: ENV — 환경·SSH·경로·ADB·리소스
> 조회: `grep -rh "ID:TB371FC-T.*-ENV" /root/kernel-lab/tb371fc/docs/ENV.md`
> 또는 `bash /root/kernel-lab/tb371fc/docs/query.sh ENV [TNN]`. 상세 전체는 `HANDOFF_TB371FC_DISPLAY_20260929.md` 1절.

## [ID:TB371FC-T01-ENV-01] 초기 확인 (T01)
- 호스트 `fedora-heart` (Debian, 12GiB, nproc 24/28), `fedora-lenovo` (172.30.1.4, nproc 8, 19GiB).
- Mac `mac-workspace` (127.0.0.1:40222, heart) 접속 성공, `MacBook-Pro-418.local Darwin 27.2.0 arm64`.
- 경로: `/Users/heart/kernel-lab`, `kernels/tb371fc`, `vendor-lab/.../stage-227`, `/Users/heart/tb371fc-flash`, `unpack_audiofix_boot/`.
- SSD `PASS (65.03/80GiB)`. USB 패스스루 없음.

## [ID:TB371FC-T03-ENV-01] ADB 전수 (T03)
- Fedora `adb 37.0.0 devices 빈`, `fastboot 빈`, `6ab06f89 not found`.
- Mac `/opt/homebrew/bin/adb 37.0.1 devices 빈`, `fastboot 빈`. PATH에 없음, 전체 경로로 확인.
- 결론: `6ab06f89`·`HA1E02DA` 미연결 → 플래싱 불가, 읽기 조사만. 이후 전 턴의 전제.

## [ID:TB371FC-T05-ENV-01] 고사양 분담 (T05)
- Mac 1순위 `-j10`, 원격 2순위. 현 작업은 대량 빌드 불필요, `-j4` 이하만.

## [ID:TB371FC-T06-ENV-01] 코어 실측 (T06)
- `heart nproc 24` + `lenovo 8`인데 `32` 과다 → `22+8=30` 상한, 커널 `-j24~28` 시작, `12GiB` 경고.

## [ID:TB371FC-T07-ENV-01] distcc 적용 (T07)
- `/etc/profile.d/distcc.sh` → `localhost/22`, `show-hosts` 확인. 기존 셸은 재`source` 필요.
