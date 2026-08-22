# build.yml — 이제 손댈 것 없습니다

예전에는 여기에 "APK 직링크가 살아나도록 두 줄을 고치세요" 라고 적혀 있었습니다.
그 두 줄은 **이 꾸러미의 `.github/workflows/build.yml` 에 이미 들어 있습니다.**

그 파일을 통째로 올리시면(연필 편집으로 갈아 끼우기) 아래가 한꺼번에 됩니다.

- 릴리스에 `NgpEmu.apk` 가 붙어 직링크가 산다
  https://github.com/rmdkdkr-png/emu-ex-plus-alpha/releases/download/Pre-release/NgpEmu.apk
- 맨 앞에 **SS2 파일 점검**(20초) — 파일이 빠졌거나 옛 판이면 거기서 멈춘다
- 밑작업(NDK·imagine·EmuFramework)을 내용 해시로 캐시 — 안 바뀌었으면 건너뛴다
- 에뮬레이터 15종을 다 만들던 것을 **NGP.emu 하나로**
- **Run workflow** 버튼이 생긴다 (`workflow_dispatch`)
