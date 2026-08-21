# 앱 포크(emu-ex-plus-alpha) README.md 맨 위에 붙여넣을 고지

아래 블록을 `README.md` 첫 줄(`# EX Emulators` 위)에 그대로 붙여넣으면 됩니다.
GPL-3.0 §5(a)의 변경 고지 + 소스 안내를 겸합니다.

---

> ## 이 저장소는 수정판(포크)입니다
>
> [Rakashazi/emu-ex-plus-alpha](https://github.com/Rakashazi/emu-ex-plus-alpha) (GPL-3.0)의 포크로,
> NGP.emu에 『사무라이 스피리츠! 2』(NGPC) **원버튼 필살기 엔진(SS2SP)** 을 넣은 것입니다.
> 라이선스는 원본과 같은 **GPL-3.0**이며 전문은 `COPYING` 에 있습니다.
>
> **변경 내용** (2026년):
> - `NGP.emu/src/ss2sp/` — 원버튼 엔진 신규 (`ss2sp.c` · `ss2sp.h` · `ss2sp_moves.h`)
> - `NGP.emu/src/ss2comm/` — 캐릭터 해설 엔진 신규 (`ss2comm.c` · `ss2comm.h` · `ss2comm_font.h`)
>   글리프는 갈무리 글꼴(SIL Open Font License 1.1)에서 필요한 글자만 옮긴 것입니다
> - `NGP.emu/src/main/` — 엔진 연결, [기술 배치] 메뉴, 기본 화면 버튼(세이브·로드·리셋·터치설정·기술배치)
> - `EmuFramework/` — 커스텀 뷰 ID 추가 등 위 기능에 필요한 최소 수정
> - 앱 이름·패키지 ID 변경 (`com.rmdkdkr.ngpemu.ss2`) — 스토어판 NGP.emu와 나란히 설치됩니다
> - 아이콘 교체
>
> 이 저장소의 소스에서 GitHub Actions로 APK가 빌드됩니다. APK를 재배포할 때는
> 이 저장소 링크를 함께 제공해 주십시오(GPL-3.0 소스 제공 의무).
>
> 게임 롬은 포함하지 않습니다. SAMURAI SPIRITS™ / SAMURAI SHODOWN™ 저작권은
> SNK CORPORATION에 있으며, 이 프로젝트는 SNK와 무관한 비공식 팬 제작물입니다.

---

## 왜 이거면 충분한가 (요약)

- **소스 제공 의무**: APK가 이 공개 저장소에서 빌드되므로, 저장소 자체가 대응 소스다.
- **변경 고지 (§5a)**: 위 블록 + git 커밋 이력.
- **라이선스 유지**: 원본 `COPYING` 을 지우지 않았고, 같은 GPL-3.0으로 배포한다.
- **추가 제한 금지**: 아무 제한도 더하지 않았다.
- APK를 저장소 밖(커뮤니티 등)에 올릴 때만 소스 링크를 같이 붙이면 된다.
