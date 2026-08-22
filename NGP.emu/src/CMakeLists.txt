target_sources(ngpemu PRIVATE FILE_SET CXX_MODULES
	BASE_DIRS main
	FILES main/system.ccm
)

# SS2 원버튼 엔진 (순수 C). 램은 포인터 주입 — CPUExRAM 이 C++ 네임스페이스 안이라서.
# set_source_files_properties 는 이 빌드에서 먹지 않았다(정의가 컴파일 줄에 안 붙음).
# 타깃 전체 정의로 건다 — 이 매크로를 보는 파일은 ss2sp.c 하나뿐이라 부작용이 없다.
target_sources(ngpemu PRIVATE ss2sp/ss2sp.c)
# SS2 캐릭터 해설 엔진 (순수 C, 램 포인터 주입은 위와 같은 매크로를 쓴다).
# 브라우저판 runner.html 의 commStep() / 코어판 ss2comm.c 와 같은 소스다.
target_sources(ngpemu PRIVATE ss2comm/ss2comm.c)
target_compile_definitions(ngpemu PRIVATE SS2SP_RAM_POINTER)

target_sources(ngpemu PRIVATE
	main/Main.cc
	main/AppMeta.cc
	main/options.cc
	main/EmuMenuViews.cc
	ngp/bios.cpp
	ngp/biosHLE.cpp
	ngp/dma.cpp
	ngp/flash.cpp
	ngp/gfx.cpp
	ngp/T6W28_Apu.cpp
	ngp/gfx_scanline_mono.cpp
	ngp/gfx_scanline_colour.cpp
	ngp/interrupt.cpp
	ngp/mem.cpp
	ngp/neopop.cpp
	ngp/rom.cpp
	ngp/rtc.cpp
	ngp/sound.cpp
	ngp/Z80_interface.cpp
	ngp/TLCS-900h/TLCS900h_interpret_single.cpp
	ngp/TLCS-900h/TLCS900h_interpret.cpp
	ngp/TLCS-900h/TLCS900h_registers.cpp
	ngp/TLCS-900h/TLCS900h_interpret_reg.cpp
	ngp/TLCS-900h/TLCS900h_interpret_src.cpp
	ngp/TLCS-900h/TLCS900h_interpret_dst.cpp
)