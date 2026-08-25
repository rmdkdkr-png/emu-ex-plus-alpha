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
#include <ss2comm/ss2comm.h>
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
	/* 해설 띠에 그릴 얼굴은 **사용자 롬에서 실행 중에 뽑는다** — 배포물에 그림을 넣지 않는다.
	   코어판(libretro)에도 같은 줄이 있다. 이 줄이 없으면 띠에 얼굴 자리가 빈다. */
	ss2comm_set_rom(MDFN_IEN_NGP::ngpc_rom.orig_data, MDFN_IEN_NGP::ngpc_rom.length);
	MDFN_IEN_NGP::SetPixelFormat(toMDFNSurface(mSurfacePix).format);
}

void NgpSystem::ss2SyncSurface()
{
	/* 띠는 화면 **위**에 붙는다. 그래서 게임은 버퍼의 32줄 아래부터 그려지게 한다 —
	   코어판처럼 매 프레임 그림을 memmove 로 밀 필요가 없다.
	   심판은 제 칸이 없다 — ss2comm_draw 가 게임 자리 맨 아래 32줄에 오버레이로 얹는다. */
	mSurfacePix = mFullPix.subView({ss2SideW, ss2BandOn() ? ss2BandH : 0}, {ss2GameW, ss2GameH});
}

IG::MutablePixmapView NgpSystem::ss2CommitPix()
{
	bool sides = ss2SidesOn();
	if(!ss2BandOn() && !sides)
		return mSurfacePix;                 /* 띠도 기둥도 없음 — 게임 화면만 올린다 */
	int totalH = ss2GameH + (ss2BandOn() ? ss2BandH : 0);   /* 띠 없이 기둥만도 된다 */
	if(mFullPix.format().bytesPerPixel() == 2)
	{
		auto fb = reinterpret_cast<uint16_t*>(mFullPix.data());
		if(ss2BandOn())
			ss2comm_draw(fb + ss2SideW, mFullPix.pitchPx(), ss2GameW, ss2GameH);
		if(sides)
		{
			/* 기둥 바탕감 — 게임 화면 좌우 가장자리를 엔진에 준다(앰비언트 배경) */
			int gameTop = ss2BandOn() ? ss2BandH : 0;
			auto pitch = mFullPix.pitchPx();
			ss2comm_side_feed(fb + gameTop*pitch + ss2SideW,
			                  fb + gameTop*pitch + ss2SideW + ss2GameW - 16, pitch);
			/* 양옆 아트웍 기둥 — 그림은 엔진이 사용자 롬에서 굽는다 */
			ss2comm_side(fb, pitch, ss2SideW, totalH, 0);
			ss2comm_side(fb + ss2SideW + ss2GameW, pitch, ss2SideW, totalH, 1);
		}
	}
	else
	{
		/* 화면이 32비트다. 엔진은 RGB565 만 그리므로 따로 그린 뒤 얹는다.
		   해설창은 맨 위 32줄. 심판 오버레이는 게임 자리 **맨 위** 32줄(해설창 바로 아래)인데,
		   엔진이 이번 프레임에 실제로 그렸을 때만 그 부분을 덧변환한다 —
		   안 그린 프레임에 변환하면 게임 그림이 검은 상자로 덮인다. */
		if(ss2BandOn())
		{
			ss2comm_draw(ss2BandScratch, ss2GameW, ss2GameW, ss2GameH);
			IG::PixmapView band{{{ss2GameW, ss2BandH}, IG::PixelFmtRGB565}, ss2BandScratch};
			mFullPix.subView({ss2SideW, 0}, {ss2GameW, ss2BandH}).writeConverted(band);
			if(int refH = ss2comm_ref_overlay())
			{
				int top = ss2BandH;   /* 게임 자리의 첫 줄부터 — 엔진과 같은 자리 */
				IG::PixmapView ref{{{ss2GameW, refH}, IG::PixelFmtRGB565}, ss2BandScratch + top * ss2GameW};
				mFullPix.subView({ss2SideW, top}, {ss2GameW, refH}).writeConverted(ref);
			}
		}
		if(sides)
		{
			/* 32비트 화면 — 게임 가장자리를 565 로 눌러 엔진에 준다(앰비언트 배경) */
			static uint16_t edge[2][16 * ss2GameH];
			int gameTop = ss2BandOn() ? ss2BandH : 0;
			for(int r = 0; r < 2; r++)
			{
				IG::MutablePixmapView e{{{16, ss2GameH}, IG::PixelFmtRGB565}, edge[r]};
				e.writeConverted(mFullPix.subView({r ? ss2SideW + ss2GameW - 16 : ss2SideW, gameTop}, {16, ss2GameH}));
			}
			ss2comm_side_feed(edge[0], edge[1], 16);
			for(int r = 0; r < 2; r++)
			{
				ss2comm_side(ss2BandScratch, ss2SideW, ss2SideW, totalH, r);
				IG::PixmapView panel{{{ss2SideW, totalH}, IG::PixelFmtRGB565}, ss2BandScratch};
				mFullPix.subView({r ? ss2SideW + ss2GameW : 0, 0}, {ss2SideW, totalH}).writeConverted(panel);
			}
		}
	}
	if(sides)
		return ss2BandOn() ? IG::MutablePixmapView{mFullPix}
		                   : mFullPix.subView({0, 0}, {ss2GameW + 2*ss2SideW, ss2GameH});
	return mFullPix.subView({ss2SideW, 0}, {ss2GameW, totalH});
}

double NgpSystem::videoAspectRatioScale() const
{
	double w = ss2GameW + (ss2SidesOn() ? 2. * ss2SideW : 0.);
	double h = ss2GameH + (ss2BandOn() ? double(ss2BandH) : 0.);
	return (w * ss2GameH) / (double(ss2GameW) * h);
}

bool NgpSystem::onVideoRenderFormatChange(EmuVideo &, PixelFormat fmt)
{
	mFullPix = {{vidBufferPx, fmt}, pixBuff};
	ss2SyncSurface();
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
		/* SP 를 눌렀는데 아무것도 안 나간 이유를 알려 준다 —
		   1 = 카드가 없어 카드 기술을 걸렀다, 2 = 그 자리에도 중립에도 기술이 없다. */
		if(ss2sp_card_block)
		{
			/* 지금은 1(카드 없음)만 쓴다 — 빈 자리는 그냥 베기로 떨어진다 */
			ss2sp_card_block = 0;
			ss2comm_notify(SS2COMM_MSG_NOCARD);
		}
	}
	/* 띠 설정을 프레임 돌리기 **전에** 반영한다 — 게임이 그려질 자리가 여기서 정해진다.
	   4 = 화면 밖 위 띠 (코어판과 같은 모드). 해설을 켜면 늘 코어가 직접 그린다. */
	ss2comm_draw_enable(ss2BandOn() ? 4 : 0);
	ss2SyncSurface();
	EmuEx::runFrame(*this, mdfnGameInfo, taskCtx, video, mSurfacePix, audio, maxAudioFrames);
	/* ── SS2 캐릭터 해설 ────────────────────────────────────────
	   프레임을 돌린 뒤 램을 읽어 이벤트를 잡는다(브라우저판·코어판과 같은 엔진).
	   대사는 코어가 띠에 직접 그리므로 여기서는 큐를 한 칸 미는 것과 진동만 한다. */
	ss2comm_set_enabled(ss2commEnabled || ss2commRef || ss2commSides);
	if(ss2commEnabled || ss2commRef || ss2commSides)
	{
		ss2comm_set_speaker(ss2commSpeaker);
		ss2comm_set_ref(ss2commRef);
		ss2comm_set_chat(ss2commEnabled);
		/* 해설은 코어가 띠에 직접 그린다. 큐는 여기서 한 칸씩 밀어 준다. */
		auto line = ss2comm_frame();
		/* 진동 두 갈래: 해설창 강조줄(금빛) + 심판 구령(승부!·한 판! — 「쿵」).
		   thump 는 읽어야 지워지므로 진동을 꺼 두었어도 매 프레임 비워 준다. */
		int thump = ss2comm_thump();
		if(ss2commVibrate && ((line && ss2comm_impact()) || thump))
		{
			/* 큰 장면에서만 짧게. 화면 버튼 햅틱(VController 32ms)과 다른 경로라
			   버튼 진동을 꺼 두어도 이것만 울린다. */
			auto &app = gApp();
			app.runOnMainThread([&app](ApplicationContext)
			{
				if(app.vibrationManager.hasVibrator())
					app.vibrationManager.vibrate(IG::Milliseconds{40});
			});
		}
	}
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
	espec->video->startFrameWithFormat(espec->taskCtx, static_cast<NgpSystem&>(*espec->sys).ss2CommitPix());
}

}
