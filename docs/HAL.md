# AREA: HAL — composer·SetPowerMode·폴링
> 조회: `grep -rh "ID:TB371FC-T.*-HAL" /root/kernel-lab/tb371fc/docs/HAL.md`. 전체는 HANDOFF 4.3절. 원본 무수정, 복사본 `/tmp/opencode/tb371fc/composer-service` (SHA 원본 동일).

## [ID:TB371FC-T01-HAL-01] 단서 (T01)
- `stage-227 vendor-root/.../composer-service`에 `proc/nvt_fw_status` 확인, 실기기 동일 여부 재확인 필요. 바이너리 읽기 전용.

## [ID:TB371FC-T08-HAL-01] 문자열·위치 (T08)
- `.rodata 0x1a56c "SetPowerMode "`, `0x1a57a "A3"`, `0x1a57d "proc/nvt_fw_status"`, `0x1a590 "GetVisibleDisplayRect"`. `0x2133b "SetPowerMode failed"`.

## [ID:TB371FC-T10-HAL-01] 차단 확정 (T10)
- `0x70430`: `open(proc/nvt_fw_status,O_RDONLY)→read 20B→close→strncmp(buf,"A3",2)`, `0→705d0 return 1` else `return 0`.
- 호출 `0x67ca8`(실패→`0x68024`), `0x68070`(실패→`0x68040` 루프), `0x68064 usleep 100ms` 재검사. 관측 내 무한 폴링형, `0x6634c` 실패 로그 참조.
- 해석: unblank가 터치 `A3` 없이는 진행 안 됨. 잔여: 호출부 전체 경계·상한 확정.

## [ID:TB371FC-T18-HAL-01] 무한 대기 확정 (T18)
- `0x67ca8` 첫 검사 실패 → `0x68024` 진입. 루프 본체 `0x68040~0x68078` 전수 디스어셈블: 카운터 증감·재시도 상한 비교 없음 (ADD는 전부 adrp 쌍 주소 계산). 유일한 탈출 = checker 성공 → `0x67cb0` display commit.
- 게이트 조건 `0x67c98 cmp w21,#1 / 0x67ca0 cmp w8,#1`: 전원 ON 전이에만 A3 검사, OFF는 통과. 즉 꺼짐은 되고 켜짐만 막힘 — 증상과 일치.
- 결론: `SetPowerMode` ON 트랜잭션은 터치 `A3`까지 100ms 간격 무한 대기. 타임아웃 없음. 커널 주석("blocks the entire unblank")과 일치 확인.
