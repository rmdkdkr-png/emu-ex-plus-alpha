# build.yml 수정 (APK 직링크용) — .github/workflows/build.yml 연필 편집

1) Release 잡의 Re-ZIP artifacts 스텝에서 이 줄을 찾는다:
        zip -r "../dist/EX-Emulators.zip" *.apk
   바로 아래에 추가:
        cp NgpEmu-*.apk ../dist/NgpEmu.apk

2) 파일 맨 아래에서:
        artifacts: "dist/*.zip"
   를 이렇게 수정:
        artifacts: "dist/*.zip,dist/*.apk"

커밋하면 다음 빌드부터 릴리스에 NgpEmu.apk가 붙고 아래 직링크가 살아난다:
https://github.com/rmdkdkr-png/emu-ex-plus-alpha/releases/download/Pre-release/NgpEmu.apk
