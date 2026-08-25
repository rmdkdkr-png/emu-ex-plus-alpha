include $(EMUFRAMEWORK_PATH)/metadata/conf.mk
metadata_name = NGP.emu SS2
metadata_exec = ngpemu
# ⚠ metadata_pkgName 은 바꾸지 말 것 — 워크플로가 NgpEmu-release.apk 를 이름으로 복사한다.
metadata_pkgName = NgpEmu
metadata_supportedFileExtensions += ngc ngp npc
# 스토어판(com.explusalpha.NgpEmu)과 다른 ID → 나란히 설치되고 세이브도 서로 안 건드린다.
metadata_id = com.rmdkdkr.ngpemu.ss2
metadata_vendor = Robert Broglia
pnd_metadata_description = Neo Geo Pocket Color emulator using components from Mednafen

# ── SS2 커스텀 층 버전 ─────────────────────────────────────────
# 프로젝트 버전은 SS2-x.y.z 로 표기한다. 바탕(EmuEx 1.5.85)은 그대로 두고
# versionName 에 병기, versionCode 는 바탕(16010585)에서 +5 부터 릴리즈마다 +1.
android_metadata_version = 1.5.85-SS2-1.0.1
android_gen_metadata_args += --version-code=16010591
