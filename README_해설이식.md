# NGP.emu 해설 이식 — 손댄 곳

브라우저 실행기 → 코어(libretro) → APK(NGP.emu) 순으로 같은 엔진(`ss2comm.c`)을 씁니다.
APK는 화면에 직접 그리지 않고 **앱 알림(토스트)** 으로 한 줄씩 띄웁니다 —
안드로이드 시스템 글꼴이라 한글이 그대로 나오고, 게임 화면을 건드리지 않습니다.

## 넣는 순서

1. `NGP.emu/src/ss2comm/` 폴더를 그대로 얹는다
   (`ss2comm.c` · `ss2comm.h` · `ss2comm_lines.h` · `ss2comm_font.h` · `ss2comm_font11.h`).
   `NGP.emu/src/ss2sp/` 도 같이 최신으로 갈아 준다 (`ss2sp.c` · `ss2sp.h` · `ss2sp_moves.h`).
2. `ngpemu_ss2comm.patch` 를 `NGP.emu/` 에서 적용한다.

```
cd NGP.emu
patch -p1 < src/ngpemu_ss2comm.patch
```

## 패치가 건드리는 파일 (다섯 개)

| 파일 | 내용 |
|---|---|
| `src/CMakeLists.txt` | `ss2comm/ss2comm.c` 한 줄 (램 포인터 매크로는 원버튼 엔진과 공용) |
| `src/main/system.ccm` | 설정 키 2개(274·275), `ss2commEnabled` · `ss2commSpeaker`, 램 포인터 주입, 리셋 |
| `src/main/Main.cc` | `runFrame()` 끝에서 `ss2comm_frame()` 호출 → UI 스레드로 넘겨 `postMessage()` |
| `src/main/options.cc` | 두 설정의 저장·불러오기 |
| `src/main/EmuMenuViews.cc` | 시스템 옵션에 `SS2 Character Commentary` · `SS2 Commentator` 항목 |

## 알아 둘 것

- 토스트는 UI 스레드 것이라 에뮬 스레드에서 바로 못 띄웁니다.
  `runOnMainThread()` 로 넘깁니다 — 프레임워크가 스크린샷 결과를 알릴 때 쓰는 것과 같은 길입니다.
- 델리게이트 저장 공간이 16바이트뿐이라 `std::string` 은 못 싣습니다.
  대사는 링 버퍼(4칸 × 160바이트)에 복사해 두고 그 포인터만 넘깁니다.
- 기술명은 롬 버전을 타므로 해설에서 다루지 않습니다. 흥·분위기 쪽입니다.
- APK 빌드는 GitHub Actions에서 돕니다(로컬 NDK 없이). 이 저장소에 올린 뒤
  **Actions → build → Run workflow**.

## 판본 (2026-08-20, v0.5.4)

브라우저·코어와 같은 엔진이라 이번 고침이 그대로 들어 있습니다.

- 승부가 난 뒤 승리 포즈를 필살기로 오인해 「비오의!」를 외치던 것 — 막음
- 눌러 둔 SP가 한참 뒤에 저절로 터지던 것 — 선입력 창 1.5초 → 0.67초, 새 입력이 오면 폐기
- 준비 화면마다 「천천히 골라라」가 나오던 것 — 전투를 벗어날 때 타이머 리셋 + 결과 뒤 침묵 적용
- **`SS2COMM_MSG_NOSLOT` 참조 제거** — 정의가 없는 상수라 그대로 두면 빌드가 깨집니다.
  지금은 `ss2sp_card_block` 이 1(카드 없음)만 쓰므로 분기 자체가 필요 없습니다.

`SS2SP_RAM_POINTER` 를 정의한 상태로 `ss2sp.c` · `ss2comm.c` 를 따로 컴파일·링크·실행해
확인했습니다(호스트 gcc 기준).
