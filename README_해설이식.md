# NGP.emu 해설 이식 — 손댄 곳

브라우저 실행기 → 코어(libretro) → APK(NGP.emu) 순으로 같은 엔진(`ss2comm.c`)을 씁니다.
APK는 두 가지로 보여 줍니다. **설정 → SS2 Commentary Display** 에서 고릅니다.

- **Band above screen** — 게임 화면 **위에 30px 띠**를 붙여 코어판과 같은 그림으로 직접 그립니다.
  게임 글꼴 + 화자 얼굴. 얼굴은 배포물에 넣지 않고 **사용자 롬에서 실행 중에 뽑습니다**
- **Notification** — 앱 알림(토스트) 한 줄. 게임 화면을 안 건드립니다

## 넣는 순서 — **패치 안 씁니다. 파일을 그대로 덮어씁니다.**

깃허브 웹 업로드로는 `patch` 를 돌릴 수 없어서, 이번 판부터는 **패치를 적용한 완성본**을
그대로 넣었습니다. 아래 경로에 맞춰 덮어쓰기만 하면 됩니다.

| 이 꾸러미 | 저장소에서의 위치 | 왜 |
|---|---|---|
| `NGP.emu/src/CMakeLists.txt` | 같은 자리 | **`ss2comm/ss2comm.c` 를 빌드에 추가** |
| `NGP.emu/src/main/system.ccm` | 같은 자리 | **`ss2commEnabled` · `ss2commSpeaker` 선언**, 램 주입, 리셋 |
| `NGP.emu/src/main/options.cc` | 같은 자리 | 두 설정 저장·불러오기 (키 274·275) |
| `NGP.emu/src/main/EmuMenuViews.cc` | 같은 자리 | 시스템 옵션에 해설 항목 2개 |
| `NGP.emu/src/main/Main.cc` | 같은 자리 | `runFrame()` 에서 해설 한 줄 → 토스트 |
| `NGP.emu/src/main/AppMeta.cc` | 같은 자리 | 버튼을 A·B·A+B·SP 넷으로 (SP2~SP8 제거) |
| `NGP.emu/src/ss2sp/` (3개) | 같은 자리 | 원버튼 엔진 |
| `NGP.emu/src/ss2comm/` (5개) | 같은 자리 | 해설 엔진 |

> ⚠️ **여섯 파일은 한 벌입니다.** `Main.cc` 만 올리면 `ss2commEnabled` 가 없다며
> 빌드가 깨집니다 — 2026-08-21 Actions 실패가 정확히 그것이었습니다.

## 띠는 어떻게 붙였나

코어판은 매 프레임 게임 그림을 `memmove` 로 30줄 아래로 밀고 그 자리에 띠를 그립니다.
앱판은 **밀 필요가 없습니다.** 프레임버퍼를 처음부터 160×182 로 잡고,
게임이 그려질 자리를 30줄 아래로 가리키면 끝입니다.

```cpp
mSurfacePix = mFullPix.subView({0, ss2BandOn() ? ss2BandH : 0}, {ss2GameW, ss2GameH});
```

화면 크기는 프레임마다 넘기는 값이라(`startFrameWithFormat(taskCtx, PixmapDesc)`)
띠를 끄면 아래 152줄만 커밋해 **원래 화면으로 돌아갑니다.**

세로가 30줄 늘어난 만큼 화면비가 눌리는데, 프레임워크에 그걸 되돌리는 자리가 있습니다.
**PCE.emu 가 보이는 줄 수를 바꿀 때 쓰는 것과 같은 함수**입니다.

```cpp
double NgpSystem::videoAspectRatioScale() const
{
	return ss2BandOn() ? double(ss2GameH) / double(ss2GameH + ss2BandH) : 1.;
}
```

`ss2comm_draw()` 는 **RGB565 전용**입니다. 화면이 32비트로 돌 때는 따로 그린 뒤
띠 30줄만 `writeConverted()` 로 변환해 얹습니다.

### 재 본 것 — **실제로 빌드해서 확인했습니다**

안드로이드 APK 는 이 환경에서 못 만듭니다 (NDK 내려받기 `dl.google.com` 차단).
대신 **같은 소스를 리눅스로 빌드**했습니다. 이 저장소에는 `linux-x86_64` 프리셋이 있고,
`system.ccm` · `Main.cc` · `options.cc` · `EmuMenuViews.cc` · `AppMeta.cc` 는
안드로이드판과 **같은 파일**입니다.

```
$ cmake --build build/linux-x86_64 --config Release
[5/5] Linking CXX executable build/linux/ngpemu

$ file build/linux/ngpemu
ELF 64-bit LSB pie executable, x86-64

$ strings build/linux/ngpemu | grep 'SS2 Commentary'
SS2 Commentary Display
SS2 Commentator
Band above screen
Notification
```

- 컴파일러 **clang 21** — 안드로이드 CI(NDK r30-beta1)와 같은 세대입니다
- 오브젝트 30개 전부 생성, 우리 파일 5개 포함, **에러 0**
- 실행 파일까지 링크 완료. 새 메뉴 문자열과 해설 대사가 실제로 들어가 있습니다

### 그 전에 따로 잰 것

```
게임 영역(30~181줄) 훼손 픽셀 : 0     ← 띠가 게임 그림을 침범하지 않는다
얼굴칸(왼쪽 30x30) 그려진 픽셀: 286   ← 롬에서 뽑은 얼굴이 실제로 그려진다
RGB565 / RGBA8888 / BGRA8888  → 커밋 160x182, 비율보정 0.8352  (띠 끔: 152, 1.0)
```

**아직 못 잰 것**: 안드로이드 전용 경로(NDK 시스루트, 안드로이드 백엔드)와 실기 동작.
다만 우리가 손댄 파일에는 안드로이드 전용 코드가 없습니다.

### 리눅스로 빌드해 보려면

```bash
export IMAGINE_PATH=$PWD/imagine EMUFRAMEWORK_PATH=$PWD/EmuFramework IMAGINE_SDK_PATH=$PWD/imagine-sdk
# clang 21 이상 + cmake 4.3.x 필요 (프로젝트가 import std 를 씀)
apt install mold libclang-rt-18-dev libxcb-icccm4-dev libxkbcommon-x11-dev libegl1-mesa-dev \
            libxcb-randr0-dev libpulse-dev libasound2-dev libgl1-mesa-dev libbluetooth-dev \
            libdrm-dev libxcb-xfixes0-dev libxcb-xinput-dev libflac-dev
(cd imagine/bundle/all && ./makeAll-linux-x86_64-static.sh install)   # 번들 libarchive (crc32 패치본)
for d in imagine EmuFramework; do (cd $d && cmake --preset linux-x86_64 \
   -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=lld \
   && cmake --build build/linux-x86_64 --config Release -j && cmake --install build/linux-x86_64 --config Release); done
(cd NGP.emu && cmake --preset linux-x86_64 ... && cmake --build build/linux-x86_64 --config Release -j)
```

> 시스템 libarchive 는 안 됩니다. 이 저장소가 `archive_entry_crc32()` 를 쓰는데
> 그건 번들에 든 패치본에만 있습니다.

## 알아 둘 것

- 토스트는 UI 스레드 것이라 에뮬 스레드에서 바로 못 띄웁니다.
  `runOnMainThread()` 로 넘깁니다 — 프레임워크가 스크린샷 결과를 알릴 때 쓰는 것과 같은 길입니다.
- 델리게이트 저장 공간이 16바이트뿐이라 `std::string` 은 못 싣습니다.
  대사는 링 버퍼(4칸 × 160바이트)에 복사해 두고 그 포인터만 넘깁니다.
- 기술명은 롬 버전을 타므로 해설에서 다루지 않습니다. 흥·분위기 쪽입니다.
- APK 빌드는 GitHub Actions에서 돕니다(로컬 NDK 없이). 이 저장소에 올린 뒤
  **Actions → build → Run workflow**.

## 판본 (2026-08-22, v0.6)

브라우저·코어와 **같은 엔진 파일**입니다. `ss2sp.c` · `ss2sp_moves.h` ·
`ss2comm.c` · `ss2comm_lines.h` 는 코어 저장소의 것과 바이트 단위로 같습니다 —
`ss2comm.c` 의 첫 `#include` 한 줄만 이식 경로(`<ss2comm/ss2comm.h>`)로 다릅니다.

- 승부가 난 뒤 승리 포즈를 필살기로 오인해 「비오의!」를 외치던 것 — 막음
- 눌러 둔 SP가 한참 뒤에 저절로 터지던 것 — 선입력 창 1.5초 → 0.67초, 새 입력이 오면 폐기
- 준비 화면마다 「천천히 골라라」가 나오던 것 — 전투를 벗어날 때 타이머 리셋 + 결과 뒤 침묵 적용
- **`SS2COMM_MSG_NOSLOT` 참조 제거** — 정의가 없는 상수라 그대로 두면 빌드가 깨집니다.
  지금은 `ss2sp_card_block` 이 1(카드 없음)만 쓰므로 분기 자체가 필요 없습니다.

### v0.6 에서 **APK 에는 안 들어간 것**

v0.5.7~v0.5.9 에서 늘어난 것은 **브라우저 실행기의 화면·조작** 쪽이라 APK 와 무관합니다.
아래 셋은 아직 브라우저 전용입니다 — 원하시면 다음 판에 넣습니다.

| 기능 | 브라우저 | 코어 | APK |
|---|:--:|:--:|:--:|
| 원버튼 필살기 · 해설 | ○ | ○ | ○ |
| 촬영 모드(기술명 ▶ 커맨드 자막) | ○ | ✕ | ✕ |
| 더미 상대(반격 없음 · 시간·체력 최대) | ○ | ✕ | ✕ |
| 가로 모드 버튼 배치 | ○ | — | — |

`SS2SP_RAM_POINTER` 를 정의한 상태로 `ss2sp.c` · `ss2comm.c` 를 호스트 gcc 로
문법 검사까지 돌려 확인했습니다.
