/*  This file is part of NGP.emu.

	NGP.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	NGP.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with NGP.emu.  If not, see <http://www.gnu.org/licenses/> */

module;
#include <mednafen/mednafen.h>
#include <ss2sp/ss2sp.h>
#include <mednafen/state-driver.h>
#include <mednafen/MemoryStream.h>
#include <ngp/neopop.h>
#include <ngp/mem.h>   /* storeB() — 원본 Main.cc 는 안 쓰던 헤더다. 엔진이 패드 바이트를 직접 쓰므로 필요. */
#include <ngp/flash.h>
#include <ngp/sound.h>

module system;

extern "C++" namespace MDFN_IEN_NGP
{
	void SetPixelFormat(Mednafen::MDFN_PixelFormat);
	uint32 GetSoundRate();
}

namespace EmuEx
{

extern "C++" std::string_view EmuSystem::shortSystemName() const { return "NGP"; }
extern "C++" std::string_view EmuSystem::systemName() const { return "Neo Geo Pocket"; }

void NgpSystem::reset(EmuApp &, ResetMode mode)
{
	assume(hasContent());
	MDFN_IEN_NGP::reset();
}

FS::FileString NgpSystem::stateFilename(int slot, std::string_view name) const
{
	return stateFilenameMDFN(*MDFNGameInfo, slot, name, 'a', noMD5InFilenames);
}

size_t NgpSystem::stateSize() { return stateSizeMDFN(); }
void NgpSystem::readState(EmuApp&, std::span<uint8_t> buff) { readStateMDFN(buff); }
size_t NgpSystem::writeState(std::span<uint8_t> buff, SaveStateFlags flags) { return writeStateMDFN(buff, flags); }

static FS::PathString saveFilename(const EmuApp &app)
{
	return app.contentSaveFilePath(".ngf");
}

void NgpSystem::loadBackupMemory(EmuApp &)
{
	log.info("loading flash");
	MDFN_IEN_NGP::FLASH_LoadNV();
}

void NgpSystem::onFlushBackupMemory(EmuApp &, BackupMemoryDirtyFlags)
{
	log.info("saving flash");
	MDFN_IEN_NGP::FLASH_SaveNV();
}

WallClockTimePoint NgpSystem::backupMemoryLastWriteTime(const EmuApp &app) const
{
	return appContext().fileUriLastWriteTime(saveFilename(app).c_str());
}

void NgpSystem::closeSystem()
{
	mdfnGameInfo.CloseGame();
}

void NgpSystem::loadContent(IO &io, EmuSystemCreateParams, OnLoadProgressDelegate)
{
	static constexpr size_t maxRomSize = 0x400000;
	EmuEx::loadContent(*this, mdfnGameInfo, io, maxRomSize);
	MDFN_IEN_NGP::SetPixelFormat(toMDFNSurface(mSurfacePix).format);
}

bool NgpSystem::onVideoRenderFormatChange(EmuVideo &, PixelFormat fmt)
{
	mSurfacePix = {{vidBufferPx, fmt}, pixBuff};
	if(!hasContent())
		return false;
	MDFN_IEN_NGP::SetPixelFormat(toMDFNSurface(mSurfacePix).format);
	return false;
}

void NgpSystem::configAudioRate(FrameRate outputFrameRate, int outputRate)
{
	uint32 mixRate = std::round(audioMixRate(outputRate, outputFrameRate));
	if(mixRate == GetSoundRate())
		return;
	log.info("set sound mix rate:{}", mixRate);
	MDFNNGPC_SetSoundRate(mixRate);
}

void NgpSystem::runFrame(EmuSystemTaskContext taskCtx, EmuVideo *video, EmuAudio *audio)
{
	static constexpr size_t maxAudioFrames = 48000 / AppMeta::minFrameRate;
	/* ── SS2 원버튼 엔진 ────────────────────────────────────────
	   NGP.emu 는 Emulate() 안의 storeB(0x6F82, *chee) 가 주석 처리돼 있고
	   입력 이벤트 때 직접 쓴다. 그래서 프레임 직전에 여기서 한 번 덮어쓰면
	   엔진이 만든 커맨드가 그대로 게임에 들어간다. */
	if(ss2spEnabled)
	{
		ss2sp_set_layout(ss2spLayoutSP);
		MDFN_IEN_NGP::storeB(0x6F82, ss2sp_frame(inputBuff, spTrig));
	}
	EmuEx::runFrame(*this, mdfnGameInfo, taskCtx, video, mSurfacePix, audio, maxAudioFrames);
}

}

using namespace EmuEx;

extern "C++" namespace MDFN_IEN_NGP
{

bool system_io_flash_read(uint8_t* buffer, uint32_t len)
{
	auto saveStr = saveFilename(gApp());
	return FileUtils::readFromUri(gAppContext(), saveStr, {buffer, len}) > 0;
}

void system_io_flash_write(uint8_t* buffer, uint32 len)
{
	if(!len)
		return;
	auto saveStr = saveFilename(gApp());
	NgpSystem::log.info("writing flash:{}", saveStr);
	FileUtils::writeToUri(gAppContext(), saveStr, {buffer, len});
}

}

extern "C++" namespace Mednafen
{

void MDFND_commitVideoFrame(EmulateSpecStruct *espec)
{
	espec->video->startFrameWithFormat(espec->taskCtx, static_cast<NgpSystem&>(*espec->sys).mSurfacePix);
}

}
