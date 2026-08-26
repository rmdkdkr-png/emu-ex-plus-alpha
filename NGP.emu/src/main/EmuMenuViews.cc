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

#include <ss2sp/ss2sp.h>
#include <ss2comm/ss2comm.h>

import system;
import emuex;
import imagine;
import std;

namespace EmuEx
{

using namespace IG;
using MainAppHelper = EmuAppHelperBase<MainApp>;

/* ═══════════════════════════════════════════════════════════════════
   SS2 원버튼 — 기술 배치
   엔진(ss2sp.c)이 유파·기술·슬롯을 전부 들고 있다. 여기는 화면만 그린다.
   ═══════════════════════════════════════════════════════════════════ */

/* 슬롯 순서는 엔진과 같다: 중립 · 앞 · 뒤 · 아래 · ↘ · ↙ · 공중 */
constexpr const char *ss2SlotName[]
{
	"SP", "→ SP", "← SP", "↓ SP", "↘ SP", "↙ SP", "Air SP"
};

/* 유파 id("hanzo_bst") → 보기 좋은 이름("한조 · 나찰")
   유파는 브라우저 실행기와 같은 표기: _s = 수라(베기), _bst = 나찰(힘) */
static std::string ss2StyleLabel(int style)
{
	struct Name { std::string_view id, ko; };
	static constexpr Name names[]
	{
		{"kazuki", "카즈키"},   {"sogetsu", "소게츠"},  {"haohmaru", "하오마루"},
		{"genjuro", "겐주로"},  {"nakoruru", "나코루루"},{"rimururu", "리무루루"},
		{"hanzo", "한조"},      {"galford", "갈포드"},  {"asura", "아수라"},
		{"charlotte", "샤를로트"},{"morozumi", "모로즈미"},{"ukyo", "우쿄"},
		{"jubei", "쥬베이"},    {"shiki", "시키"},      {"yuga", "유가"},
	};
	std::string_view id{ss2sp_style_id(style)};
	auto cut = id.rfind('_');
	std::string_view base = cut == std::string_view::npos ? id : id.substr(0, cut);
	std::string_view suf  = cut == std::string_view::npos ? std::string_view{} : id.substr(cut + 1);
	std::string_view ko = base;
	for(const auto &n : names)
	{
		if(base == n.id) { ko = n.ko; break; }
	}
	std::string out{ko};
	out += " · ";
	out += (suf == "bst") ? "나찰" : "수라";
	return out;
}

/* "부동격  236+A  잡기" */
static std::string ss2MoveLabel(int style, int mv)
{
	if(mv < 0)
		return "— 없음 —";
	char note[32]{};
	ss2sp_move_notation(style, mv, note, sizeof(note));
	std::string out{ss2sp_move_name(style, mv)};
	out += "  ";
	out += note;
	int f = ss2sp_move_flags(style, mv);
	if(f & 16) out += "  잡기";
	if(f & 2)  out += "  카드";
	if(f & 1)  out += "  근접";
	if(f & 4)  out += "  공중";
	if(f & 8)  out += "  미검증";
	return out;
}

/* ── 유파 고르기 (30개) ───────────────────────────────────────── */
class SS2StylePickView : public TableView
{
public:
	/* DelegateFunc 기본 저장공간은 포인터 2개다. 32비트 ABI에서 [this, k] 캡처가
	   딱 걸리므로 넉넉히 잡는다. */
	using OnPick = DelegateFuncS<sizeof(void*) * 4, void(int style)>;

	SS2StylePickView(ViewAttachParams attach, OnPick onPick_):
		TableView{"캐릭터 / 유파", attach, rows},
		onPick{onPick_}
	{
		int n = ss2sp_style_count();
		rows.reserve(size_t(n));
		for(int i = 0; i < n; i++)
		{
			rows.emplace_back(ss2StyleLabel(i), attach,
				[this, i]
				{
					onPick(i);
					dismiss();
				});
		}
	}

private:
	OnPick onPick;
	std::vector<TextMenuItem> rows;
};

/* ── 기술 고르기 ──────────────────────────────────────────────── */
class SS2MovePickView : public TableView
{
public:
	using OnPick = DelegateFuncS<sizeof(void*) * 4, void(int mv)>;

	SS2MovePickView(ViewAttachParams attach, int style, OnPick onPick_):
		TableView{"기술 고르기", attach, rows},
		onPick{onPick_}
	{
		int n = ss2sp_move_count(style);
		rows.reserve(size_t(n) + 1);
		rows.emplace_back("— 없음 —", attach,
			[this]
			{
				onPick(-1);
				dismiss();
			});
		for(int i = 0; i < n; i++)
		{
			rows.emplace_back(ss2MoveLabel(style, i), attach,
				[this, i]
				{
					onPick(i);
					dismiss();
				});
		}
	}

private:
	OnPick onPick;
	std::vector<TextMenuItem> rows;
};

/* ── 슬롯 7칸 ─────────────────────────────────────────────────── */
class SS2SlotView : public TableView
{
public:
	SS2SlotView(ViewAttachParams attach):
		TableView{"기술 배치", attach, item}
	{
		int cur = ss2sp_cur_style();
		style = cur >= 0 ? cur : lastStyle;
		reload();
	}

private:
	int style{};
	static inline int lastStyle{};
	std::vector<TextMenuItem> rows;
	std::vector<MenuItem*> item;

	void reload()
	{
		lastStyle = style;
		rows.clear();
		item.clear();
		rows.reserve(16);   /* 1 + 7 + 1. 재할당되면 item 의 포인터가 죽는다 */

		{
			std::string s{"캐릭터    "};
			s += ss2StyleLabel(style);
			auto &r = rows.emplace_back(std::move(s), attachParams(),
				[this](const Input::Event &e)
				{
					pushAndShow(makeView<SS2StylePickView>(
						SS2StylePickView::OnPick{[this](int st){ style = st; reload(); }}), e);
				});
			item.emplace_back(&r);
		}

		int slots = ss2sp_slot_count();
		for(int k = 0; k < slots; k++)
		{
			std::string s{ss2SlotName[k]};
			s += "    ";
			s += ss2MoveLabel(style, ss2sp_get_slot(style, k));
			auto &r = rows.emplace_back(std::move(s), attachParams(),
				[this, k](const Input::Event &e)
				{
					pushAndShow(makeView<SS2MovePickView>(style,
						SS2MovePickView::OnPick{[this, k](int mv){ ss2sp_set_slot(style, k, mv); reload(); }}), e);
				});
			item.emplace_back(&r);
		}

		{
			/* 여기서 reload() 를 부르면 지금 실행 중인 이 항목 자신을 파괴한다.
			   그래서 되돌린 뒤 화면을 닫는다(다시 열면 기본값이 보인다). */
			auto &r = rows.emplace_back("기본 배치로 되돌리기", attachParams(),
				[this]
				{
					ss2sp_reset_slots();
					dismiss();
				});
			item.emplace_back(&r);
		}
	}
};

/* ── 시스템 옵션에 붙이는 항목들 ──────────────────────────────── */
class CustomSystemOptionView : public SystemOptionView, public MainAppHelper
{
	using MainAppHelper::system;
	using MainAppHelper::app;

	BoolMenuItem ngpLanguage
	{
		"NGP Language", attachParams(),
		system().optionNGPLanguage,
		"Japanese", "English",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().optionNGPLanguage = item.flipBoolValue(*this);
		}
	};

	BoolMenuItem ss2spEnabled
	{
		"SS2 One-button Specials", attachParams(),
		system().ss2spEnabled,
		"Off", "On",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().ss2spEnabled = item.flipBoolValue(*this);
		}
	};

	/* SS2 캐릭터 해설 — 게임 화면 위 띠에 얼굴과 함께 그린다 */
	BoolMenuItem ss2commEnabled
	{
		"SS2 Character Commentary", attachParams(),
		system().ss2commEnabled,
		"Off", "On",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().ss2commEnabled = item.flipBoolValue(*this);
		}
	};

	/* v0.7: 해설자가 15명이다. 이름은 엔진(ss2comm)이 들고 있으므로 여기서 받아 만든다 —
	   대사표를 늘려도 이 파일은 안 고쳐도 된다. */
	std::vector<TextMenuItem> ss2commSpeakerItem = [this]
	{
		std::vector<TextMenuItem> v;
		const int n = ss2comm_speaker_count();
		v.reserve(n);
		for(int i = 0; i < n; i++)
			v.emplace_back(ss2comm_speaker_name(i), attachParams(),
				[this, i](){ system().ss2commSpeaker = (uint8_t)i; });
		return v;
	}();

	MultiChoiceMenuItem ss2commSpeaker
	{
		"SS2 Commentator", attachParams(),
		(int)system().ss2commSpeaker,
		ss2commSpeakerItem
	};


	/* 심판(쿠로코) — 호명·N회전·승부!·한 판!·팻말 호명. 온이면 심판이 해설창을
	   우선 쓰고 캐릭터챗은 후순위로 기다린다. */
	BoolMenuItem ss2commRef
	{
		"SS2 Referee (Kuroko)", attachParams(),
		system().ss2commRef,
		"Off", "On",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().ss2commRef = item.flipBoolValue(*this);
		}
	};

	/* 양옆 아트웍 — 넓은 화면의 빈 좌우 기둥에 초상·이름을 세운다.
	   그림은 사용자 롬에서 실행 중에 굽는다 — 배포물에는 그림이 없다. */
	BoolMenuItem ss2commSides
	{
		"SS2 Side Art", attachParams(),
		system().ss2commSides,
		"Off", "On",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().ss2commSides = item.flipBoolValue(*this);
		}
	};

	/* 기둥 배경 고르기 — 자동(화면 밖 이어붙임)이 어색한 스테이지에서 돌려 가며 고른다 */
	/* 장면 수집 — 온이면 세이브스테이트를 뜰 때마다 상태·램·화면 파일이
	   세이브 폴더(.ngf 옆)에 쌓인다. 관찰용 — 평소엔 꺼 둔다. */
	BoolMenuItem ss2commSceneCap
	{
		"SS2 Scene Capture", attachParams(),
		system().ss2commSceneCap,
		"Off", "On",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().ss2commSceneCap = item.flipBoolValue(*this);
		}
	};

	/* 큰 장면에 짧게 울린다. 화면 버튼 햅틱과 별개라 버튼 진동은 꺼 둔 채 쓸 수 있다. */
	BoolMenuItem ss2commVibrate
	{
		"SS2 Commentary Vibration", attachParams(),
		system().ss2commVibrate,
		"Off", "On",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			system().ss2commVibrate = item.flipBoolValue(*this);
		}
	};

	TextMenuItem ss2spSlots
	{
		"SS2 Move Assignment", attachParams(),
		[this](const Input::Event &e)
		{
			pushAndShow(makeView<SS2SlotView>(), e);
		}
	};

	/* 버전 정보 — 앱 버전은 metadata/conf.mk 의 android_metadata_version 과 같이 올린다 */
	TextMenuItem ss2Version
	{
		"버전 정보", attachParams(),
		[this]()
		{
			app().postMessage(6, false,
				"NGPcustumSP 1.5.85-SS2-1.0.2\n"
				"해설 엔진 SS2comm v" SS2COMM_VERSION
				" · Robert Broglia의 NGP.emu(EmuEx)·Mednafen 기반 · GPL");
		}
	};

	BoolMenuItem saveFilenameType = saveFilenameTypeMenuItem(*this, system());

public:
	CustomSystemOptionView(ViewAttachParams attach): SystemOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&ngpLanguage);
		item.emplace_back(&ss2spEnabled);
		item.emplace_back(&ss2spSlots);
		item.emplace_back(&ss2commEnabled);
		item.emplace_back(&ss2commSpeaker);
		item.emplace_back(&ss2commRef);
		item.emplace_back(&ss2commSides);
		item.emplace_back(&ss2commSceneCap);
		item.emplace_back(&ss2commVibrate);
		item.emplace_back(&ss2Version);
		item.emplace_back(&saveFilenameType);
	}
};

std::unique_ptr<View> EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::SYSTEM_OPTIONS: return std::make_unique<CustomSystemOptionView>(attach);
		/* 화면 버튼 / 패드 키로 기술 배치 화면에 바로 들어온다 */
		case ViewID::CUSTOM_1: return std::make_unique<SS2SlotView>(attach);
		default: return nullptr;
	}
}

}
