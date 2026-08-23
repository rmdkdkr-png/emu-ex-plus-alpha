# NGP.emu SS2 — 안드로이드 APK 빌드

이 문서 하나로 빈 컨테이너에서 APK까지 간다.
2026-08-23에 **실제로 이 순서대로 처음부터 끝까지 돌려서** 나온 기록이다.
시간·용량은 측정값이고, 함정은 전부 실제로 밟아 본 것이다.

만들어지는 것:

```
NGP.emu/build/android/build/outputs/apk/release/NgpEmu-release.apk
package = com.rmdkdkr.ngpemu.ss2      (스토어판 com.explusalpha.NgpEmu 와 달라 나란히 설치된다)
ABI     = arm64-v8a / armeabi-v7a / x86 / x86_64
크기    ≈ 4.5 MB
```

---

## 0. 먼저 — 이 방에서 되나?

```sh
curl -s -o /dev/null -w '%{http_code}\n' https://dl.google.com/android/repository/repository2-3.xml
```

**200이 아니면 이 문서대로 못 한다.** 안드로이드 SDK·NDK를 구글에서 받아야 하는데
그게 막힌 방이 있다. 그런 방에서는 아래 「구글이 막힌 방이라면」을 봐라.

디스크는 **15GB쯤** 든다 (빌드 트리 4.4G + NDK가 그 안에 3.5G + SDK 0.5G + 번들 작업물 0.5G).

---

## 1. 시스템 의존물

```sh
sudo apt-get update -qq
sudo apt-get install -y --no-install-recommends \
  autoconf automake autopoint bash gcc-arm-linux-gnueabi file gawk gettext \
  git libtool libtool-bin make nasm pkg-config unzip wget openjdk-21-jdk
```

JDK는 **21**. `java -version`으로 확인.

## 2. 소스

```sh
git clone --depth 1 https://github.com/rmdkdkr-png/emu-ex-plus-alpha
cd emu-ex-plus-alpha
```

`NGP.emu/src/ss2comm/` 와 `NGP.emu/src/ss2sp/` 가 SS2 얹은 부분이다.

## 3. CMake — **4.3.x 로 고정한다**

```sh
pip install --break-system-packages cmake==4.3.4
cmake --version          # 4.3.4 여야 한다. /usr/local/bin 이 앞에 와야 한다
```

> **함정.** 4.4 이상이면 `imagine/cmake/config.cmake` 의
> `CMAKE_EXPERIMENTAL_CXX_IMPORT_STD` UUID(`451f2fe2-a8a2-47c3-bc32-94786d8fc91b`)가
> 안 맞아 `import std` 가 꺼지고 imagine/EmuFramework configure 가 죽는다.
> 우분투 기본 3.28로도 안 된다. **4.3.x 여야 한다.**

## 4. NDK — **r30-beta1**

```sh
wget https://dl.google.com/android/repository/android-ndk-r30-beta1-linux.zip
unzip -q android-ndk-r30-beta1-linux.zip && mv android-ndk-r30-beta1 android-ndk
rm android-ndk-r30-beta1-linux.zip
cat android-ndk/source.properties | grep Revision   # 30.0.14904198-beta1
```

713MB 받아서 3.5GB로 풀린다.

> **함정.** 더 낮은 NDK면 `'flat_set' file not found` 로 죽는다. clang/libc++ 이 낡아서다.

## 5. 안드로이드 SDK — **compileSdk 36**

`imagine/make/gradle/app/build.gradle` 이 `compileSdk 36`, AGP 9.0.0, Gradle 9.4.1 이다.
**34로는 안 된다.**

```sh
mkdir -p ~/android-sdk/cmdline-tools && cd /tmp
wget https://dl.google.com/android/repository/commandlinetools-linux-13114758_latest.zip -O cmdt.zip
unzip -q cmdt.zip -d /tmp/cmdt
mv /tmp/cmdt/cmdline-tools ~/android-sdk/cmdline-tools/latest

export ANDROID_HOME=~/android-sdk
yes | $ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager --sdk_root=$ANDROID_HOME --licenses
$ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager --sdk_root=$ANDROID_HOME \
  "platform-tools" "platforms;android-36" "build-tools;36.0.0"
```

Gradle 9.4.1 은 wrapper 가 알아서 받는다. AGP 9.0.0 은 `dl.google.com/dl/android/maven2` 에서 온다.

## 6. 디버그 서명 키

```sh
mkdir -p ~/.android
keytool -genkey -v -keystore ~/.android/debug.keystore -storepass android \
  -alias androiddebugkey -keypass android -keyalg RSA -validity 10000 \
  -dname "CN=Android Debug,O=Android,C=US"
```

**없으면 마지막 서명 단계에서 죽는다.**

## 7. 환경 변수

```sh
export ROOT=$PWD                       # emu-ex-plus-alpha 를 클론한 자리
export ANDROID_NDK_PATH=$ROOT/android-ndk
export ANDROID_HOME=~/android-sdk
export ANDROID_SDK_ROOT=$ANDROID_HOME
export IMAGINE_PATH=$ROOT/imagine
export EMUFRAMEWORK_PATH=$ROOT/EmuFramework
export IMAGINE_SDK_PATH=$ROOT/imagine-sdk
export PATH=/usr/local/bin:$ANDROID_HOME/build-tools/36.0.0:$ANDROID_HOME/platform-tools:$PATH
export JAVA_HOME=/usr/lib/jvm/java-21-openjdk-amd64
mkdir -p $IMAGINE_SDK_PATH
```

## 8. 번들 라이브러리 → 프레임워크

**순서를 지켜라.** 번들이 먼저다.

```sh
(cd imagine/bundle/all && ./makeAll-android.sh install)     # 약 4분 30초
$IMAGINE_PATH/android.sh config
$IMAGINE_PATH/android.sh installLinks --config Release      # 약 3분
$EMUFRAMEWORK_PATH/android.sh config
$EMUFRAMEWORK_PATH/android.sh installLinks --config Release # 약 20분  ← 여기가 제일 길다
```

> **함정.** 번들을 건너뛰면 `archive_entry_crc32` undefined 로 링크가 죽는다.
> 시스템 libarchive 를 물어서다. 번들은 패치본이라 반드시 먼저 깔아야 한다.

끝나면 이게 있어야 한다:

```sh
ls imagine-sdk/android-arm64/lib/   # libimagine.a libemuframework.a libarchive.a libc++.a ...
```

## 9. APK

```sh
cd NGP.emu
make -f android.mk android-apk CONFIG=Release V=1 -j$(nproc)     # 약 1분 20초
```

**첫 빌드 총합 ≈ 34분.**

## 10. 두 번째부터는 35초다

`ss2comm.c` 같은 걸 고치고 9번만 다시 돌리면 된다.
imagine·EmuFramework 는 정적 라이브러리로 이미 깔려 있어서 NGP.emu 만 다시 돈다.
gradle 44개 태스크 중 5개만 실행된다.

**빌드 트리(4.4GB)를 지우지 마라.** 그게 35초의 이유다.

---

## 11. 서명 — v1+v2+v3 로 다시 걸어 준다

gradle 이 뽑는 것은 v1+v2 다. 그대로도 안드로이드 11+ 에 **설치된다**
(11 이상이 요구하는 건 v2 까지다). 그래도 v3 를 걸어 두면 뒤탈이 없다.

```sh
apksigner sign --ks ~/.android/debug.keystore --ks-pass pass:android --key-pass pass:android \
  --v1-signing-enabled true --v2-signing-enabled true --v3-signing-enabled true \
  --out NgpEmu-signed.apk \
  build/android/build/outputs/apk/release/NgpEmu-release.apk
apksigner verify -v NgpEmu-signed.apk
```

> **함정.** 서명을 `jarsigner` 로 보지 마라. jarsigner 는 **v1 밖에 못 읽어서**
> v2/v3 가 걸려 있어도 "서명 안 된 것으로 취급한다"고 경고한다.
> 그 경고를 보고 "v1 뿐이라 설치가 안 된다"고 오진한 적이 있다. **`apksigner` 로 봐라.**

## 12. 확인

```sh
APK=build/android/build/outputs/apk/release/NgpEmu-release.apk

aapt2 dump badging $APK | head -1        # com.rmdkdkr.ngpemu.ss2
unzip -l $APK | grep -E '\.so|gpOverlay' # lib/*/libmain.so 4종, assets/gpOverlay.png

mkdir -p /tmp/chk && cd /tmp/chk && unzip -o -q $APK lib/arm64-v8a/libmain.so
grep -a -c -F "SS2 Commentator" lib/arm64-v8a/libmain.so
grep -a -c -F "겐주로"          lib/arm64-v8a/libmain.so
```

> **함정 둘.**
> - 라이브러리 이름은 `libngpemu.so` 가 아니라 **`libmain.so`** 다.
> - `res/overlays/gpOverlay.png` 는 APK 안에서 **`assets/gpOverlay.png`** 로 간다.
> - 한글은 **`strings` 로 안 잡힌다.** ASCII 만 뽑는 도구라서다.
>   `grep -a -F` 로 바이트를 직접 찾아라.

---

## 13. 글꼴 — 새 글자를 쓰려면

대사에 **글꼴에 없는 글자**를 넣으면 화면에 **공백**으로 나온다.
「리쿠도렛카」가 「리쿠도 카」로 나왔던 것이 그 사고다.

대사를 고쳤으면 글꼴을 다시 뽑아라. 생성기와 받는 법은 실행기 저장소에 있다:

```
https://github.com/rmdkdkr-png/ss2-sp-runner  →  tools/gen_font.js
```

BDF 원본은 저장소에 없다(폰트 파일 자체는 재배포하지 않는다).
`gen_font.js` 머리 주석에 받는 곳 두 군데가 적혀 있다.

```sh
node tools/gen_font.js <NGP.emu/src/ss2comm 자리> <BDF 자리>
```

## 14. 시험 — 롬도 에뮬레이터도 없이 돈다

```
https://github.com/rmdkdkr-png/ss2-sp-core  →  src/test_flow.sh
```

흐름·총평·짝꿍 11건을 host 에서 검증한다. **APK 굽기 전에 이걸 먼저 돌려라.**

## 15. 세 갈래가 어긋나면 안 된다

`ss2comm.*` · `ss2sp.*` 는 **코어판과 앱판이 같은 파일을 쓴다.**
딱 한 줄만 다르다 — 앱판은 `#include <ss2comm/ss2comm.h>`, 코어판은 `#include "ss2comm.h"`.

한쪽만 고치면 어긋난다. 고쳤으면 확인해라:

```sh
md5sum ss2-sp-core/src/ss2comm_lines.h \
       emu-ex-plus-alpha/NGP.emu/src/ss2comm/ss2comm_lines.h
```

그리고 `ss2comm_lines.h` 는 **실행기 `index.html` 의 SPEAKERS 에서 뽑는 생성물**이다.
헤더만 고치면 나중에 생성기를 돌리는 순간 되돌아간다. **실행기도 같이 고쳐라.**

세 저장소:

| 갈래 | 저장소 |
|---|---|
| 브라우저판 | `rmdkdkr-png/ss2-sp-runner` |
| 코어판(libretro) | `rmdkdkr-png/ss2-sp-core` |
| 앱판(이 저장소) | `rmdkdkr-png/emu-ex-plus-alpha` |

---

## 16. 구글이 막힌 방이라면

`dl.google.com` · `maven.google.com` 이 막힌 방에서는 **안드로이드 SDK 를 못 받는다.**
그런 방에서 쓰던 우회는 이렇다 (이 문서를 쓴 방에서는 필요 없었다):

- 깃허브 Actions 로 빌드 — `.github/workflows/build.yml`. 왕복 10분쯤
- 방 안에서 급히 만들어야 하면 apktool + smali. `apktool.jar` 안에
  `prebuilt/linux/aapt2_64` 가 들어 있다. d8/dx 가 없어 Activity 를 smali 로 쓴다.
  서명은 `uber-apk-signer`

**구글이 열린 방이 있으면 그 방에서 굽는 게 압도적으로 빠르다** — 35분 왕복 대 35초다.

---

## 17. 안 해도 되는 것

- 롬·세이브스테이트는 **어디에도 넣지 않는다.** 빌드에 필요 없다
- SNK 그림은 배포물에 안 넣는다. 해설자 얼굴은 **실행 중에 사용자 롬에서 뽑는다**
- 꾸러미를 만들 때마다: `find . \( -name '*.ngc' -o -name '*.ngp' -o -name 'st_*.bin' \)`

## 18. 이 문서에 없는 것

- `android_arch=arm64` 로 하나만 굽는 방법 — 인계문에 적혀 있으나 이 방에서는 안 써 봤다.
  네 아키텍처 다 구워도 1분 20초라 굳이 필요하지 않았다
- 릴리스 서명(개인 설치용이라 디버그 키로 충분하다)
