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
