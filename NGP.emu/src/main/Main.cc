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
#include <format>
#include <vector>
#include <cstring>
#include <algorithm>
#include <ss2sp/ss2sp.h>
#include <svcsp/svcsp.h>
#include <ss2comm/ss2comm.h>
#include <ss2fg/ss2fg.h>
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
size_t NgpSystem::writeState(std::span<uint8_t> buff, SaveStateFlags flags)
{
	auto n = writeStateMDFN(buff, flags);
	/* 장면 수집(SS2 Scene Capture 온일 때만): 유저가 **직접** 세이브스테이트를 뜨는
	   순간마다 상태·램·화면 세 파일을 .ngf 폴더에 번호 붙여 떨군다.
	   자동 감지는 없다 — 찍을 장면은 사람이 고른다(제보). */
	if(ss2commSceneCap && n)
		ss2SceneDump({buff.data(), n});
	return n;
}

void NgpSystem::ss2SceneAuto()
{
	/* 장면 자동 수집(제보: 「내가 플레이하면 체크되게」) — (mode, scr) 조합이 바뀌면
	   30프레임 뒤(화면이 다 선 뒤) 상태·램·화면을 떨군다. 같은 조합은 5초 간격,
	   최대 4번까지(스토리 페이지처럼 같은 조합의 다른 장면 대비), 세션당 60장. */
	using namespace MDFN_IEN_NGP;
	static uint32_t fc; static uint16_t lastSig = 0xFFFF; static int pend = -1;
	static struct { uint16_t sig; uint8_t n; uint32_t at; } seen[64];
	static int nSeen, total;
	fc++;
	uint8_t mode = CPUExRAM[0x00A7], scr = CPUExRAM[0x01C0];
	uint16_t sig = uint16_t(mode << 8 | (scr & 15));
	if(sig != lastSig) { lastSig = sig; pend = 30; }
	if(pend <= 0 || --pend != 0 || total >= 60)
		return;
	int i = 0;
	for(; i < nSeen; i++) if(seen[i].sig == sig) break;
	if(i < nSeen)
	{
		if(seen[i].n >= 4 || fc - seen[i].at < 300)
			return;
		seen[i].n++; seen[i].at = fc;
	}
	else if(nSeen < 64)
	{
		seen[nSeen++] = {sig, 1, fc};
	}
	else return;
	total++;
	auto sz = stateSizeMDFN();
	std::vector<uint8_t> st(sz);
	auto n = writeStateMDFN(st, {});
	if(n) ss2SceneDump({st.data(), n});
}

void NgpSystem::ss2SceneDump(std::span<uint8_t> state)
{
	using namespace MDFN_IEN_NGP;
	static int idx;
	uint8_t mode = CPUExRAM[0x00A7], scr = CPUExRAM[0x01C0], stage = CPUExRAM[0x17FE];
	auto tag = std::format("_scene{:02d}_m{:02X}_s{}_st{}", idx, mode, scr, stage);
	idx++;
	auto ctx = appContext();
	FileUtils::writeToUri(ctx, gApp().contentSaveFilePath((tag + ".state").c_str()), state);
	FileUtils::writeToUri(ctx, gApp().contentSaveFilePath((tag + ".ram").c_str()), {CPUExRAM, 16384});
	/* 화면을 PPM 으로 — 게임 자리(160x152)만 */
	std::vector<uint8_t> ppm;
	auto hdr = std::format("P6\n{} {}\n255\n", ss2GameW, ss2GameH);
	ppm.insert(ppm.end(), hdr.begin(), hdr.end());
	int bpp = mSurfacePix.format().bytesPerPixel();
	for(int y = 0; y < ss2GameH; y++)
	{
		auto row = reinterpret_cast<const uint8_t*>(mSurfacePix.data()) + y * mSurfacePix.pitchBytes();
		for(int x = 0; x < ss2GameW; x++)
		{
			uint8_t r, g, b;
			if(bpp == 2)
			{
				uint16_t v; memcpy(&v, row + x*2, 2);
				r = ((v>>11)&31)*255/31; g = ((v>>5)&63)*255/63; b = (v&31)*255/31;
			}
			else
			{
				r = row[x*4]; g = row[x*4+1]; b = row[x*4+2];
			}
			ppm.push_back(r); ppm.push_back(g); ppm.push_back(b);
		}
	}
	FileUtils::writeToUri(ctx, gApp().contentSaveFilePath((tag + ".ppm").c_str()), ppm);
	log.info("장면 수집:{}", tag);
}

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
	/* 전용판 게이트 — 이 앱이 다루는 두 게임(SS2·SVC)만 연다. 범용 NGP 에뮬로
	   쓰이면 원작자(EmuEx)의 유료 앱 이익을 침해한다. 헤더 타이틀(0x24)로 판별. */
	if(MDFN_IEN_NGP::ngpc_rom.length < 0x30 ||
	   (memcmp(MDFN_IEN_NGP::ngpc_rom.data + 0x24, "SAMURAI2", 8) != 0 &&
	    memcmp(MDFN_IEN_NGP::ngpc_rom.data + 0x24, "SNKvsCAPCOM1", 12) != 0))
	{
		throw std::runtime_error{"SS2·SVC 커스텀판입니다 — 사무라이 쇼다운!2 또는\n"
		                         "SNK vs. Capcom(격투의 달인) 롬만 열 수 있습니다.\n"
		                         "다른 게임은 정식 NGP.emu 앱을 이용해 주세요."};
	}
	/* 해설 띠에 그릴 얼굴은 **사용자 롬에서 실행 중에 뽑는다** — 배포물에 그림을 넣지 않는다.
	   코어판(libretro)에도 같은 줄이 있다. 이 줄이 없으면 띠에 얼굴 자리가 빈다. */
	ss2comm_set_rom(MDFN_IEN_NGP::ngpc_rom.orig_data, MDFN_IEN_NGP::ngpc_rom.length);
	svcsp_set_rom(MDFN_IEN_NGP::ngpc_rom.orig_data, MDFN_IEN_NGP::ngpc_rom.length);
	/* 프레임 생성: 사무쇼2 롬이면 RAM 의 몸 위치(P1·P2)를 캡처마다 적어, 포즈가 통째로 바뀌는 몸도 궤적대로 옮긴다.
	   코어판 libretro.c 와 같은 줄. 이 앱은 사무쇼2·SvC 만 여니 SvC 가 아니면 사무쇼2 — SvC 는 안 물린다. */
	ss2fg_set_ram(svcsp_rom_ok() ? nullptr : MDFN_IEN_NGP::CPUExRAM);
	ss2fg_set_fx(ss2FrameGenFx);                /* 이펙트·장풍 규칙(옮기기/섞기/끔) — 메뉴에서 바꾸면 거기서도 넘긴다 */
	ss2fg_set_pose(ss2FrameGenPose);            /* 날아가는 몸 포즈 섞기(섞기/끔) */
	svcsp_set_engine(1);                        /* SVC 롬이면 모던 조작 — 끄기는 「원버튼 필살기」 옵션 공용 */
	ss2comm_overlay_spmode(svcsp_rom_ok());     /* 오버레이 SP 페이지가 svcsp 표를 쓴다 */
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
			auto pitch = mFullPix.pitchPx();
			/* 양옆 아트웍 기둥 — 그림은 엔진이 롬·VRAM에서 굽는다 */
			ss2comm_side(fb, pitch, ss2SideW, totalH, 0);
			ss2comm_side(fb + ss2SideW + ss2GameW, pitch, ss2SideW, totalH, 1);
		}
		if(ss2comm_overlay_active())
			ss2comm_overlay_draw(fb + ss2SideW + (ss2BandOn() ? ss2BandH : 0) * mFullPix.pitchPx(),
			                     mFullPix.pitchPx(), ss2GameW, ss2GameH);
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
			for(int r = 0; r < 2; r++)
			{
				ss2comm_side(ss2BandScratch, ss2SideW, ss2SideW, totalH, r);
				IG::PixmapView panel{{{ss2SideW, totalH}, IG::PixelFmtRGB565}, ss2BandScratch};
				mFullPix.subView({r ? ss2SideW + ss2GameW : 0, 0}, {ss2SideW, totalH}).writeConverted(panel);
			}
		}
		if(ss2comm_overlay_active())
		{
			/* 32비트 표면 — 스크래치(565)에 그려 변환해 얹는다. 스크래치엔 게임 그림이
			   없으므로 바탕을 검게 깔고 게임 자리 전체를 덮는다(모달처럼 보인다). */
			std::fill_n(ss2BandScratch, size_t(ss2GameW) * ss2GameH, uint16_t(0));
			ss2comm_overlay_draw(ss2BandScratch, ss2GameW, ss2GameW, ss2GameH);
			IG::PixmapView ov{{{ss2GameW, ss2GameH}, IG::PixelFmtRGB565}, ss2BandScratch};
			mFullPix.subView({ss2SideW, ss2BandOn() ? ss2BandH : 0}, {ss2GameW, ss2GameH}).writeConverted(ov);
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
	/* ── 빠른 설정 = 일시정지 메뉴 ─────────────────────────────
	   설정창이 열려 있는 동안은 에뮬레이션을 돌리지 않는다(소리도 없음).
	   마지막 프레임 표면 위에 오버레이만 다시 그려 낸다. */
	if(ss2comm_overlay_active())
	{
		if(video)
			video->startFrameWithFormat(taskCtx, ss2CommitPix());
		return;
	}
	/* ── SS2 원버튼 엔진 ────────────────────────────────────────
	   NGP.emu 는 Emulate() 안의 storeB(0x6F82, *chee) 가 주석 처리돼 있고
	   입력 이벤트 때 직접 쓴다. 그래서 프레임 직전에 여기서 한 번 덮어쓰면
	   엔진이 만든 커맨드가 그대로 게임에 들어간다. */
	if(ss2spEnabled && svcsp_rom_ok())
	{	/* ── SVC 모던 조작 — SP 키 = 기술키(방향+SP = 배치 필살기, 짧게 약/길게 강),
		   기본기 연타 = 자동 콤보(히트 확인 후 초필, 게이지 없으면 필살기).
		   A/B 기본기는 순정 그대로(탭 약/홀드 강). */
		MDFN_IEN_NGP::storeB(0x6F82, svcsp_frame_app(inputBuff, spTrig));
		static int svcSeq{};
		if(svcsp_disp_seq != svcSeq)
		{	/* 기술명 토스트 — 해설 알림과 같은 파이프 */
			svcSeq = svcsp_disp_seq;
			static std::array<std::array<char, 72>, 4> svcRing{};
			static unsigned svcRingPos{};
			auto &slot = svcRing[svcRingPos++ & 3];
			std::snprintf(slot.data(), slot.size(), "%s", svcsp_last_disp);
			auto &app = gApp();
			app.runOnMainThread([&app, msg = slot.data()](ApplicationContext)
			{
				app.postMessage(2, false, msg);
			});
		}
	}
	else if(ss2spEnabled)
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
	ss2FgNextOk = false;                 /* 새 실제 프레임 — 지난 4배 그림은 버린다 */
	ss2FgCommitPending = false;
	ss2FrameNo++;
	EmuEx::runFrame(*this, mdfnGameInfo, taskCtx, video, mSurfacePix, audio, maxAudioFrames);
	if(ss2FgCommitPending)
	{	/* 4배·런어헤드: Emulate 가 끝난 뒤 숨은 프레임을 돌려 보여 줄 그림을 표면에 만들고(못 하면 실제 N 그대로) 올린다 */
		ss2FgCommitPending = false;
		int ra = ss2RaFrames();
		if(ss2FourXActive())
			ss2Predict4(ra);
		else if(ss2SixtyPath())
		{	/* 60Hz 사이 그림(코어 패치 90): 120 출력이 아닐 때 4배의 2프레임 예측을 그대로 돌려 실제 프레임마다
			   «박자에 맞춘 자리»(τ=D)만 표면에 — 안 바뀌는 프레임에 다음 그림까지의 반 자리를 그려 30→60Hz, 예측이라 지연 없음.
			   사무쇼2가 아니면(캡처 없음) ss2Predict4 가 false → 런어헤드만 */
			ss2FgNoNext = true;
			if(!ss2Predict4(ra) && ra)
				ss2RaPlain(ra);
			ss2FgNoNext = false;
			ss2FgNextOk = false;
		}
		else if(ra)
			ss2RaPlain(ra);
		if(video)
			video->startFrameWithFormat(taskCtx, ss2CommitPix());
	}
	/* ── SS2 캐릭터 해설 ────────────────────────────────────────
	   프레임을 돌린 뒤 램을 읽어 이벤트를 잡는다(브라우저판·코어판과 같은 엔진).
	   대사는 코어가 띠에 직접 그리므로 여기서는 큐를 한 칸 미는 것과 진동만 한다. */
	if(ss2commSceneCap)
		ss2SceneAuto();
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

/* ── 프레임 생성 (120Hz) — 코어판 libretro.c 의 fg_predict 와 같은 절차 ──
   예측 프레임용 가벼운 스냅샷: 소리·플래시 섹션은 뺀다(예측 중 바뀌지 않게 막는다) +
   StateAction 밖의 정적 변수들(Z80 인터럽트선, 직렬 버퍼·플래시 커맨드 플래그). */
extern "C" int ngp_fg_mute;          /* sound.cpp  — 예측 프레임 중 소리 쓰기·플러시 무시 */
extern "C" int ngp_fg_predict;       /* mem.cpp    — 예측 프레임 중 롬/플래시 쓰기 차단 */
extern "C" int ngp_fg_state_skip;    /* neopop.cpp — StateAction 에서 건너뛸 섹션(1 소리, 2 플래시) */
extern "C" void ngp_mem_fg_extras(uint8_t *buf, int load);
extern "C" int ngp_z80_iline_get(void);
extern "C" void ngp_z80_iline_set(int v);

namespace EmuEx
{

static Mednafen::MemoryStream *ss2FgStateStream{};

bool NgpSystem::ss2PredictFrame()
{
	using namespace Mednafen;
	uint8_t extras[8];
	int ilineSave = ngp_z80_iline_get();
	if(!ss2FgStateStream)
		ss2FgStateStream = new MemoryStream(65536, -1);
	ngp_fg_state_skip = 3;
	ss2FgStateStream->setSize(0);
	ss2FgStateStream->seek(0, SEEK_SET);
	MDFNSS_SaveSM(ss2FgStateStream);
	ngp_mem_fg_extras(extras, 0);

	/* 입력은 램에 든 마지막 값 그대로(붙잡고 있다고 가정). 그림은 안 그린다(skip) — 캡처만 */
	EmulateSpecStruct espec{};
	espec.sys = this;
	espec.video = nullptr;
	espec.skip = true;
	auto mSurface = toMDFNSurface(mSurfacePix);
	espec.surface = &mSurface;
	ngp_fg_mute = 1; ngp_fg_predict = 1;
	mdfnGameInfo.Emulate(&espec);
	ngp_fg_mute = 0; ngp_fg_predict = 0;

	/* 그림은 실제 N(prev), 위치만 예측 N+1(cur) 쪽으로 반 — 없는 그림은 절대 안 나온다 */
	int ok = ss2fg_render_ex(ss2fg_prev(), ss2fg_cur(), 128, mSurfacePix.data(), mSurfacePix.pitchPx(),
		mSurfacePix.format().bytesPerPixel(), MDFN_IEN_NGP::NGPGfx->colorMap());

	ss2FgStateStream->seek(0, SEEK_SET);
	MDFNSS_LoadSM(ss2FgStateStream);
	ngp_fg_state_skip = 0;
	ngp_mem_fg_extras(extras, 1);
	ngp_z80_iline_set(ilineSave);
	ss2fg_capture_pop();                 /* 예측 프레임 캡처를 무른다 */
	return ok != 0;
}

/* 4배 — 실제 N 을 막 돌린 직후(커밋 전). N+1·N+2 를 미리 돌려 스프라이트·스크롤 각각 «다음에 바뀌는 프레임 K» 와
   «지난번 바뀐 때 L(K 보다 최대 2 프레임 앞)» 을 찾고, L~K 를 고르게 나눠 τ=N 은 표면에, τ=N+½ 은 ss2FgNext 에.
   위치만 옮긴다 — 그림(타일·포즈)은 실제 N 의 것. 코어판 fg_predict4 와 같은 계산. */
/* ── 숨은 프레임 돌리기 (4배·런어헤드 공용) — 코어판 fg_snap_save/fg_hidden/fg_snap_load 와 같은 절차 ── */
static uint8_t ss2SnapExtras[8];
static int ss2SnapIline;

bool NgpSystem::ss2SnapSave()
{
	using namespace Mednafen;
	if(!ss2FgStateStream)
		ss2FgStateStream = new MemoryStream(65536, -1);
	ngp_fg_state_skip = 3;
	ss2FgStateStream->setSize(0);
	ss2FgStateStream->seek(0, SEEK_SET);
	MDFNSS_SaveSM(ss2FgStateStream);
	ngp_mem_fg_extras(ss2SnapExtras, 0);
	ss2SnapIline = ngp_z80_iline_get();
	return true;
}

void NgpSystem::ss2SnapLoad(int pops)
{
	using namespace Mednafen;
	ss2FgStateStream->seek(0, SEEK_SET);
	MDFNSS_LoadSM(ss2FgStateStream);
	ngp_fg_state_skip = 0;
	ngp_mem_fg_extras(ss2SnapExtras, 1);
	ngp_z80_iline_set(ss2SnapIline);
	while(pops-- > 0)
		ss2fg_capture_pop();
}

/* 입력은 램에 든 마지막 값 그대로(붙잡고 있다고 가정). keep(1..n) 번째 프레임만 그려 표면에 남기고 나머지는 캡처만 */
bool NgpSystem::ss2Hidden(int n, int keep)
{
	using namespace Mednafen;
	auto mSurface = toMDFNSurface(mSurfacePix);
	ngp_fg_mute = 1; ngp_fg_predict = 1;
	for(int k = 1; k <= n; k++)
	{
		EmulateSpecStruct espec{};
		espec.sys = this;
		espec.video = nullptr;
		espec.skip = (k != keep);
		espec.surface = &mSurface;
		mdfnGameInfo.Emulate(&espec);
	}
	ngp_fg_mute = 0; ngp_fg_predict = 0;
	return true;
}

int NgpSystem::ss2RaFrames() const
{
	if(ss2RunAhead == 0 || svcsp_rom_ok())                        /* 사무쇼2 롬에서만(SvC 는 반응 프레임을 안 쟀다) */
		return 0;
	if(ss2FrameGen == 1 && ss2FrameGenMult == 2 && ss2SlotsComing())   /* 2배 프레임 생성 중엔 끔(코어판과 같음) */
		return 0;
	return ss2RunAhead > 2 ? 2 : ss2RunAhead;
}

/* 프레임 생성 없이(60Hz 화면 등) 런어헤드만 — 표면에 N+ra 그림이 남는다 */
bool NgpSystem::ss2RaPlain(int ra)
{
	ss2FgNextOk = false;
	if(!ss2fg_cur() || !ss2SnapSave())
		return false;
	ss2Hidden(ra, ra);
	ss2SnapLoad(ra);
	return true;
}

bool NgpSystem::ss2Predict4(int ra)
{
	using namespace Mednafen;
	ss2FgNextOk = false;
	if(!ss2fg_hist(0) || !ss2SnapSave())
		return false;
	ss2FgRealN++;
	/* 보여 줄 프레임 D = N+ra. 숨겨 돌린 뒤 링: [0]=D+2 [1]=D+1 [2]=D [3]=D-1 (ra=0 이면 D-1 은 실제 N-1).
	   keep=ra 라 표면에는 D 의 그림이 남는다(ra=0 이면 실제 N 그대로) */
	ss2Hidden(ra + 2, ra);
	const ss2fg_frame *f2 = ss2fg_hist(0), *f1 = ss2fg_hist(1), *f0 = ss2fg_hist(2), *fp = ss2fg_hist(3);
	int mp0 = (fp && f0) ? ss2fg_motion(fp, f0) : 0;   /* D-1→D 에 바뀐 것 — 박자 기억 */
	if(mp0 & 1) ss2FgLastSpr = ss2FgRealN;
	if(mp0 & 2) ss2FgLastScr = ss2FgRealN;
	auto bodyDiffers = [](const ss2fg_frame *a, const ss2fg_frame *b, int p)
	{
		if(!a || !b || !a->body_ok || !b->body_ok) return false;
		if(!(a->ob_act & b->ob_act & (1u << p))) return false;
		return a->ob_x[p] != b->ob_x[p] || a->ob_y[p] != b->ob_y[p];
	};
	for(int p = 0; p < 2; p++) if(bodyDiffers(fp, f0, p)) ss2FgLastBody[p] = ss2FgRealN;
	if(!f0 || !f1 || !f2)
	{
		ss2SnapLoad(ra + 2);
		return false;
	}
	int m01 = ss2fg_motion(f0, f1), m12 = ss2fg_motion(f1, f2);
	auto tpos = [](int K, int L, int h){ return (!K || K <= L) ? 0 : (256 * (h - 2 * L)) / (2 * (K - L)); };
	int Ks = (m01 & 1) ? 1 : (m12 & 1) ? 2 : 0, rel = (int)ss2FgLastSpr - (int)ss2FgRealN;
	int Ls = Ks ? std::max(rel, Ks - 2) : 0;
	int Kb = (m01 & 2) ? 1 : (m12 & 2) ? 2 : 0;
	rel = (int)ss2FgLastScr - (int)ss2FgRealN;
	int Lb = Kb ? std::max(rel, Kb - 2) : 0;
	const ss2fg_frame *tos = Ks == 1 ? f1 : Ks == 2 ? f2 : nullptr;
	const ss2fg_frame *tob = Kb == 1 ? f1 : Kb == 2 ? f2 : nullptr;
	int ts0 = tpos(Ks, Ls, 0), ts1 = tpos(Ks, Ls, 1), tb0 = tpos(Kb, Lb, 0), tb1 = tpos(Kb, Lb, 1);

	/* 몸 따로 박자(코어판 fg_predict4 와 같은 절차) — 감전·불탐으로 날아갈 때 게임은 몸을 4 프레임에 한 번 옮기므로
	   전체 박자(2 프레임)로는 «4장 미끄러지고 4장 멈춤». 몸마다 지난 바뀜·다음 바뀜을 따로 찾아(필요할 때만 2 프레임 더
	   미리 돌림) 그 사이로 보간한 자리를 엔진에 넣는다(ss2fg_body_override). */
	int extra = 0;
	bool ov[2]{};
	int ovx0[2]{}, ovy0[2]{}, ovx1[2]{}, ovy1[2]{};
	const ss2fg_frame *fk[5]{f0, f1, f2, nullptr, nullptr};
	for(int p = 0; p < 2 && f0->body_ok; p++)
	{
		int Kp = 0, lrel = (int)ss2FgLastBody[p] - (int)ss2FgRealN;
		for(int k = 1; k <= 2 && !Kp; k++) if(bodyDiffers(f0, fk[k], p)) Kp = k;
		if(!Kp && lrel >= -3 && !extra)
		{	/* 막 움직이던 몸이 앞 2 프레임 안엔 안 바뀜 — 2 프레임 더 미리 돌려 본다(그동안만 숨은 프레임 6 장) */
			ss2Hidden(2, 0);
			extra = 2;
			fk[4] = ss2fg_hist(0); fk[3] = ss2fg_hist(1);
		}
		for(int k = 3; k <= 4 && !Kp; k++) if(fk[k] && bodyDiffers(f0, fk[k], p)) Kp = k;
		if(!Kp) continue;
		/* L: 지난 바뀜이 K-4..K-3 이면 느린 박자가 이어지는 중 — 그 간격 그대로. 아니면(쉬다가 움직임) 전체와 같은 K-2 */
		int Lp = (lrel >= Kp - 4 && lrel <= Kp - 3) ? lrel : std::max(lrel, Kp - 2);
		if(Lp > 0) continue;                 /* 미끄러짐이 아직 시작 전 — 지금은 제자리 */
		if(Kp == Ks && Lp == Ls) continue;   /* 전체 박자와 같으면 하던 대로 */
		int dx = (int8_t)(uint8_t)(fk[Kp]->ob_x[p] - f0->ob_x[p]);
		int dy = (int8_t)(uint8_t)(fk[Kp]->ob_y[p] - f0->ob_y[p]);
		if(dx > 24 || dx < -24 || dy > 24 || dy < -24) continue;
		ov[p] = true;
		ovx0[p] = dx * tpos(Kp, Lp, 0); ovy0[p] = dy * tpos(Kp, Lp, 0);
		ovx1[p] = dx * tpos(Kp, Lp, 1); ovy1[p] = dy * tpos(Kp, Lp, 1);
	}

	auto bpp = mSurfacePix.format().bytesPerPixel();
	auto pitchPx = mSurfacePix.pitchPx();
	size_t pitchB = (size_t)pitchPx * bpp;
	if(ss2FgNext.size() < pitchB * ss2GameH)
		ss2FgNext.resize(pitchB * ss2GameH);
	auto cmap = MDFN_IEN_NGP::NGPGfx->colorMap();
	/* τ=N+½ — 먼저 실제 N 을 복사해 두고(못 만들면 그대로), 그 위에 그린다. 60Hz 사이 그림(ss2FgNoNext)은 건너뛴다 */
	if(!ss2FgNoNext)
	{
		for(int y = 0; y < ss2GameH; y++)
			memcpy(ss2FgNext.data() + y * pitchB, mSurfacePix.data() + y * pitchB, ss2GameW * bpp);
		for(int p = 0; p < 2; p++) if(ov[p]) ss2fg_body_override(p, 1, ovx1[p], ovy1[p]);
		if(ts1 || tb1 || ov[0] || ov[1])
			ss2fg_render2(f0, tos, ts1, tob, tb1, ss2FgNext.data(), pitchPx, bpp, cmap);
	}
	/* τ=N — 둘 다 0 이면 실제 N 그대로(표면에 이미 있다). 합성 실패면 render2 가 아무것도 안 쓴다 */
	for(int p = 0; p < 2; p++) if(ov[p]) ss2fg_body_override(p, 1, ovx0[p], ovy0[p]);
	if(ts0 || tb0 || ov[0] || ov[1])
		ss2fg_render2(f0, tos, ts0, tob, tb0, mSurfacePix.data(), pitchPx, bpp, cmap);
	ss2fg_body_override(0, 0, 0, 0); ss2fg_body_override(1, 0, 0, 0);
	ss2FgNextOk = !ss2FgNoNext;
	ss2SnapLoad(ra + 2 + extra);         /* 숨은 프레임 캡처를 무른다 */
	return true;
}

/* 빈 vsync 슬롯 — EmuSystemTask::advanceFrames 가 advanced==0 일 때 부른다 */
bool NgpSystem::interFrame(EmuSystemTaskContext taskCtx, EmuVideo &video)
{
	if(ss2FrameGen != 1 || !hasContent() || !ss2fg_cur() || ss2comm_overlay_active())
		return false;                    /* 「60Hz 사이 그림만」은 빈 슬롯을 채우지 않는다(120 출력 안 함) */
	ss2FgLastInter = ss2FrameNo;         /* 빈 슬롯이 오고 있다 — 4배·런어헤드 판정용 */
	if(ss2FgNextOk)
	{	/* 4배: 실제 호출 때 미리 만들어 둔 반 프레임 뒤 그림 */
		auto bpp = mSurfacePix.format().bytesPerPixel();
		auto pitchB = mSurfacePix.pitchPx() * bpp;
		for(int y = 0; y < ss2GameH; y++)
			memcpy(mSurfacePix.data() + y * pitchB, ss2FgNext.data() + y * pitchB, ss2GameW * bpp);
		ss2FgNextOk = false;
	}
	else
	{	/* 2배(또는 4배가 못 만든 프레임): 합성이 안 되면(표시 중 VRAM 쓰기·흑백 모드) 실제 프레임을 그대로 한 번 더 올린다 */
		ss2PredictFrame();
	}
	ss2comm_draw_hold(1);                /* 띠·토스트는 다시 그리되 수명은 실제 프레임에서만 줄인다 */
	auto pix = ss2CommitPix();
	ss2comm_draw_hold(0);
	video.startFrameWithFormat(taskCtx, pix);
	return true;
}

}

extern "C++" namespace Mednafen
{

void MDFND_commitVideoFrame(EmulateSpecStruct *espec)
{
	auto &sys = static_cast<NgpSystem&>(*espec->sys);
	if((sys.ss2FourXActive() || sys.ss2SixtyPath() || sys.ss2RaFrames() > 0) && sys.hasContent() && ss2fg_cur() && !ss2comm_overlay_active())
	{	/* 4배·런어헤드: 여기는 Emulate 안이다 — 커밋을 runFrame 으로 미루고 거기서 숨은 프레임을 돌린 뒤 올린다 */
		sys.ss2FgCommitPending = true;
		return;
	}
	espec->video->startFrameWithFormat(espec->taskCtx, sys.ss2CommitPix());
}

}
