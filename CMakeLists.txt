target_sources(ngpemu PRIVATE FILE_SET CXX_MODULES
	BASE_DIRS main
	FILES main/system.ccm
)

# SS2 원버튼 엔진 (순수 C). 램은 포인터 주입 — CPUExRAM 이 C++ 네임스페이스 안이라서.
target_sources(ngpemu PRIVATE ss2sp/ss2sp.c)
set_source_files_properties(ss2sp/ss2sp.c PROPERTIES
	COMPILE_DEFINITIONS SS2SP_RAM_POINTER)

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