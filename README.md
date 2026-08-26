> ## 🔱 이 저장소는 포크입니다 — NGPcustumSP
>
> Robert Broglia 의 [emu-ex-plus-alpha](https://github.com/Rakashazi/emu-ex-plus-alpha) 를
> 갈라 나온 것으로, **NGP.emu 에만** 네오지오 포켓 『사무라이 쇼다운!2』 전용 층을 얹었습니다 —
> 한국어 캐릭터 해설, 원버튼 필살기, 양옆 일러스트 기둥.
>
> | | |
> |---|---|
> | 앱 받는 곳 | https://github.com/rmdkdkr-png/CustumApKS |
> | 해설·필살기 엔진 소스 | https://github.com/rmdkdkr-png/ss2-sp-core |
> | 손댄 곳 | `NGP.emu/` 아래 — `src/ss2comm/`(엔진 사본), `src/main/`, `metadata/conf.mk`, `res/icons/` |
>
> 앱 ID 를 `com.rmdkdkr.ngpemu.ss2` 로 바꿔 스토어판과 나란히 깔리게 했고,
> 표시명도 **NGPcustumSP** 로 따로 붙였습니다. 원작 NGP.emu·NEO.emu 는 Robert Broglia 가
> 스토어에서 파는 앱입니다 — 그 앱들을 대체하려는 물건이 아니며,
> 이쪽은 사쇼!2 롬이 아니면 아예 열리지 않도록 잠가 두었습니다.
>
> **GPL v3** 을 그대로 따릅니다. 아래는 원본 README 입니다.

---

# EX Emulators

The EX Emulator project is a series of emulators primarily targeting Android and Linux with a minimalist UI 
and a focus on low audio/video latency. They consist of 3 components:

1. Imagine, a platform abstraction library
2. EmuFramework, a common application framework library
3. The emulator applications themselves

## Requirements

Android: Any 64-bit device or a 32-bit device with at least Android 2.3 going as far back as the Xperia Play  
Linux: A desktop with OpenGL 3.3 support or the Pandora handheld

Saturn.emu requires a 64-bit device.

## Building

See doc/INSTALL in Imagine and EmuFramework

## Android builds

### Google Play

The latest stable builds are available on the Play Store via my [developer page](https://play.google.com/store/apps/collection/cluster?gsr=SktqGFp2dmEzNmZEOXhIaXV4b2ZkYXBaTHc9PbICKwoOCgpjb20uUGNlRW11EAcSFwgCEhM1MDUxMDg2NTA4NjQ2MzQ4OTg2GACwEgA%3D:S:ANO1ljKaDG4&hl) 
and directly help fund development.

### Nightly builds

Latest builds from continuous integration:

| Name                    | Status                            | File                                       |
|-------------------------|-----------------------------------|--------------------------------------------|
| EX emulators            | [![Build Status][Build]][Actions] | [![Emulator][Download]][EX emulators]      |
| 2600 emu                | [![Build Status][Build]][Actions] | [![Emulator][Download]][2600 emu]          |
| C64 emu                 | [![Build Status][Build]][Actions] | [![Emulator][Download]][C64 emu]           |
| GBA emu                 | [![Build Status][Build]][Actions] | [![Emulator][Download]][GBA emu]           |
| GBC emu                 | [![Build Status][Build]][Actions] | [![Emulator][Download]][GBC emu]           |
| Lynx emu                | [![Build Status][Build]][Actions] | [![Emulator][Download]][Lynx emu]          |
| MD emu                  | [![Build Status][Build]][Actions] | [![Emulator][Download]][MD emu]            |
| MSX emu                 | [![Build Status][Build]][Actions] | [![Emulator][Download]][MSX emu]           |
| NEO emu                 | [![Build Status][Build]][Actions] | [![Emulator][Download]][NEO emu]           |
| NES emu                 | [![Build Status][Build]][Actions] | [![Emulator][Download]][NES emu]           |
| NGP emu                 | [![Build Status][Build]][Actions] | [![Emulator][Download]][NGP emu]           |
| PCE emu                 | [![Build Status][Build]][Actions] | [![Emulator][Download]][PCE emu]           |
| Saturn emu              | [![Build Status][Build]][Actions] | [![Emulator][Download]][Saturn emu]        |
| Snes9x EX 1.43          | [![Build Status][Build]][Actions] | [![Emulator][Download]][Snes9x EX 1.43  ]  |
| Snes9x EX Plus          | [![Build Status][Build]][Actions] | [![Emulator][Download]][Snes9x EX Plus]    |
| Swan emu                | [![Build Status][Build]][Actions] | [![Emulator][Download]][Swan emu]          |

[Actions]: https://github.com/Rakashazi/emu-ex-plus-alpha/actions/workflows/build.yml
[Build]: https://github.com/Rakashazi/emu-ex-plus-alpha/actions/workflows/build.yml/badge.svg
[Download]: https://img.shields.io/badge/Download-blue
[EX emulators]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/EX-Emulators.zip
[2600 emu]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/2600Emu.zip
[C64 emu]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/C64Emu.zip
[GBA emu]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/GbaEmu.zip
[GBC emu]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/GbcEmu.zip
[Lynx emu]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/LynxEmu.zip
[MD emu]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/MdEmu.zip
[MSX emu]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/MsxEmu.zip
[NEO emu]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/NeoEmu.zip
[NES emu]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/NesEmu.zip
[NGP emu]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/NgpEmu.zip
[PCE emu]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/PceEmu.zip
[Saturn emu]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/SaturnEmu.zip
[Snes9x EX 1.43]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/Snes9xEX.zip
[Snes9x EX Plus]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/Snes9xEXPlus.zip
[Swan emu]: https://github.com/Rakashazi/emu-ex-plus-alpha/releases/download/Pre-release/SwanEmu.zip
