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

#include <emuframework/macros.h>
#include <mednafen/types.h>
import system;
import emuex;
import imagine;
import std;
#include <emuframework/EmuAppInlines.hh>

namespace EmuEx
{

const std::string_view AppMeta::creditsViewStr{CREDITS_INFO_STRING "(c) 2011-2026\nRobert Broglia\nwww.explusalpha.com\n\nPortions (c) the\nMednafen Team\nmednafen.github.io"};
const std::string_view AppMeta::configFilename{"NgpEmu.config"};
const bool AppMeta::needsGlobalInstance{true};
const AspectRatioInfo AppMeta::aspectRatioInfo{"20:19 (Original)", {20, 19}};
const NameFilterFunc AppMeta::defaultFsFilter = [](std::string_view name)
{
	return endsWithAnyCaseless(name, ".ngc", ".ngp", ".npc", ".ngpc");
};

constexpr auto dpadKeyInfo = makeArray<KeyInfo>
(
	NgpKey::Up,
	NgpKey::Right,
	NgpKey::Down,
	NgpKey::Left
);

constexpr auto optionKeyInfo = makeArray<KeyInfo>(NgpKey::Option);

constexpr auto faceKeyInfo = makeArray<KeyInfo>
(
	NgpKey::A,
	NgpKey::B
);

constexpr auto turboFaceKeyInfo = turbo(faceKeyInfo);

/* SS2 원버튼 필살기 — 실기에 없는 가상 키. 화면 버튼으로도 따로 뜬다. */
constexpr auto spKeyInfo = makeArray<KeyInfo>
(
	NgpKey::SP1,
	NgpKey::SP2,
	NgpKey::SP3,
	NgpKey::SP4,
	NgpKey::SP5,
	NgpKey::SP6,
	NgpKey::SP7,
	NgpKey::SP8
);

/* A+B 동시입력(분노 폭발) — 터치로는 두 버튼을 같은 프레임에 누르기가 어렵다 */
constexpr auto abKeyInfo = makeArray<KeyInfo>(NgpKey::AB);

constexpr auto gpKeyInfo = concatToArrayNow<dpadKeyInfo, optionKeyInfo, faceKeyInfo, turboFaceKeyInfo, spKeyInfo, abKeyInfo>;

std::span<const KeyCategory> AppMeta::keyCategories()
{
	static constexpr std::array categories
	{
		KeyCategory{"Gamepad", gpKeyInfo},
	};
	return categories;
}

std::string_view AppMeta::systemKeyCodeToString(KeyCode c)
{
	switch(NgpKey(c))
	{
		case NgpKey::Up: return "Up";
		case NgpKey::Right: return "Right";
		case NgpKey::Down: return "Down";
		case NgpKey::Left: return "Left";
		case NgpKey::Option: return "Option";
		case NgpKey::A: return "A";
		case NgpKey::B: return "B";
		case NgpKey::SP1: return "SP 1";
		case NgpKey::SP2: return "SP 2";
		case NgpKey::SP3: return "SP 3";
		case NgpKey::SP4: return "SP 4";
		case NgpKey::SP5: return "SP 5";
		case NgpKey::SP6: return "SP 6";
		case NgpKey::SP7: return "SP 7";
		case NgpKey::SP8: return "SP 8";
		case NgpKey::AB: return "A+B";
		default: return "";
	}
}

std::span<const KeyConfigDesc> AppMeta::defaultKeyConfigs()
{
	using namespace Input;

	static constexpr std::array pcKeyboardMap
	{
		KeyMapping{NgpKey::Up, Keycode::UP},
		KeyMapping{NgpKey::Right, Keycode::RIGHT},
		KeyMapping{NgpKey::Down, Keycode::DOWN},
		KeyMapping{NgpKey::Left, Keycode::LEFT},
		KeyMapping{NgpKey::Option, Keycode::ENTER},
		KeyMapping{NgpKey::B, Keycode::Z},
		KeyMapping{NgpKey::A, Keycode::X},
		/* SS2 원버튼 — 남는 키에 얹는다 */
		KeyMapping{NgpKey::SP1, Keycode::A},
		KeyMapping{NgpKey::SP2, Keycode::S},
		KeyMapping{NgpKey::SP3, Keycode::D},
		KeyMapping{NgpKey::SP4, Keycode::F},
		KeyMapping{NgpKey::SP5, Keycode::Q},
		KeyMapping{NgpKey::SP6, Keycode::W},
		KeyMapping{NgpKey::SP7, Keycode::E},
		KeyMapping{NgpKey::SP8, Keycode::R},
		KeyMapping{NgpKey::AB, Keycode::C},
	};

	static constexpr std::array genericGamepadMap
	{
		KeyMapping{NgpKey::Up, Keycode::UP},
		KeyMapping{NgpKey::Right, Keycode::RIGHT},
		KeyMapping{NgpKey::Down, Keycode::DOWN},
		KeyMapping{NgpKey::Left, Keycode::LEFT},
		KeyMapping{NgpKey::Option, Keycode::GAME_START},
		KeyMapping{NgpKey::B, Keycode::GAME_X},
		KeyMapping{NgpKey::A, Keycode::GAME_A},
		/* NGPC는 버튼이 셋뿐이라 패드의 나머지가 통째로 논다 — 거기에 필살기를 단다 */
		KeyMapping{NgpKey::SP1, Keycode::GAME_B},
		KeyMapping{NgpKey::SP2, Keycode::GAME_Y},
		KeyMapping{NgpKey::SP3, Keycode::GAME_L1},
		KeyMapping{NgpKey::SP4, Keycode::GAME_R1},
		KeyMapping{NgpKey::SP5, Keycode::GAME_L2},
		KeyMapping{NgpKey::SP6, Keycode::GAME_R2},
		KeyMapping{NgpKey::SP7, Keycode::GAME_LEFT_THUMB},
		KeyMapping{NgpKey::SP8, Keycode::GAME_RIGHT_THUMB},
		KeyMapping{NgpKey::AB, Keycode::GAME_SELECT},
	};

	static constexpr std::array wiimoteMap
	{
		KeyMapping{NgpKey::Up, WiimoteKey::UP},
		KeyMapping{NgpKey::Right, WiimoteKey::RIGHT},
		KeyMapping{NgpKey::Down, WiimoteKey::DOWN},
		KeyMapping{NgpKey::Left, WiimoteKey::LEFT},
		KeyMapping{NgpKey::B, WiimoteKey::_1},
		KeyMapping{NgpKey::A, WiimoteKey::_2},
		KeyMapping{NgpKey::Option, WiimoteKey::PLUS},
	};

	return genericKeyConfigs<pcKeyboardMap, genericGamepadMap, wiimoteMap>();
}

bool AppMeta::allowsTurboModifier(KeyCode c)
{
	switch(NgpKey(c))
	{
		case NgpKey::A ... NgpKey::B:
			return true;
		default: return false;
	}
}

constexpr FRect gpImageCoords(IRect cellRelBounds)
{
	/* 원본은 256x128(8x4칸)이었다. SS2 원버튼 버튼 그림 9개를 넣느라 세로를 늘렸다.
	   2의 거듭제곱을 유지하려고 320이 아니라 512로 잡았다(빈 칸은 압축되어 거의 공짜).
	   위 4칸 줄은 원본 그대로다. */
	constexpr F2Size imageSize{256, 512};
	constexpr int cellSize = 32;
	return (cellRelBounds.relToAbs() * cellSize).as<float>() / imageSize;
}

AssetDesc AppMeta::vControllerAssetDesc(KeyInfo key)
{
	static constexpr struct VirtualControllerAssets
	{
		AssetDesc dpad{AssetFileID::gamepadOverlay, gpImageCoords({{}, {4, 4}})},

		a{AssetFileID::gamepadOverlay,      gpImageCoords({{4, 0}, {2, 2}})},
		b{AssetFileID::gamepadOverlay,      gpImageCoords({{6, 0}, {2, 2}})},
		option{AssetFileID::gamepadOverlay, gpImageCoords({{4, 2}, {2, 1}}), {1, 2}},

		blank{AssetFileID::gamepadOverlay, gpImageCoords({{6, 2}, {2, 2}})},

		/* SS2 원버튼 — 아래 4칸 줄(y=4~7)에 새로 그려 넣은 것들.
		   빈 원과 구분이 안 된다는 지적을 받아 글자를 넣었다. */
		sp1{AssetFileID::gamepadOverlay, gpImageCoords({{0, 4}, {2, 2}})},
		sp2{AssetFileID::gamepadOverlay, gpImageCoords({{2, 4}, {2, 2}})},
		sp3{AssetFileID::gamepadOverlay, gpImageCoords({{4, 4}, {2, 2}})},
		sp4{AssetFileID::gamepadOverlay, gpImageCoords({{6, 4}, {2, 2}})},
		sp5{AssetFileID::gamepadOverlay, gpImageCoords({{0, 6}, {2, 2}})},
		sp6{AssetFileID::gamepadOverlay, gpImageCoords({{2, 6}, {2, 2}})},
		sp7{AssetFileID::gamepadOverlay, gpImageCoords({{4, 6}, {2, 2}})},
		sp8{AssetFileID::gamepadOverlay, gpImageCoords({{6, 6}, {2, 2}})},
		ab {AssetFileID::gamepadOverlay, gpImageCoords({{0, 8}, {2, 2}})};
	} virtualControllerAssets;

	if(key[0] == 0)
		return virtualControllerAssets.dpad;
	switch(NgpKey(key[0]))
	{
		case NgpKey::A: return virtualControllerAssets.a;
		case NgpKey::B: return virtualControllerAssets.b;
		case NgpKey::Option: return virtualControllerAssets.option;
		case NgpKey::SP1: return virtualControllerAssets.sp1;
		case NgpKey::SP2: return virtualControllerAssets.sp2;
		case NgpKey::SP3: return virtualControllerAssets.sp3;
		case NgpKey::SP4: return virtualControllerAssets.sp4;
		case NgpKey::SP5: return virtualControllerAssets.sp5;
		case NgpKey::SP6: return virtualControllerAssets.sp6;
		case NgpKey::SP7: return virtualControllerAssets.sp7;
		case NgpKey::SP8: return virtualControllerAssets.sp8;
		case NgpKey::AB:  return virtualControllerAssets.ab;
		default: return virtualControllerAssets.blank;
	}
}

SystemInputDeviceDesc AppMeta::inputDeviceDesc(int idx)
{
	static constexpr std::array gamepadComponents
	{
		InputComponentDesc{"D-Pad", dpadKeyInfo, InputComponent::dPad, LB2DO},
		InputComponentDesc{"Face Buttons", faceKeyInfo, InputComponent::button, RB2DO},
		InputComponentDesc{"Option", optionKeyInfo, InputComponent::button, RB2DO},
		/* 화면에는 SP 하나만 얹는다(방향과 조합해 다 쓴다).
		   SP2~SP6 은 물리 패드에만 남겨 화면을 어지럽히지 않는다. */
		InputComponentDesc{"SP Button", {&spKeyInfo[0], 1}, InputComponent::button, RB2DO},
		InputComponentDesc{"A+B", abKeyInfo, InputComponent::button, RB2DO},
		InputComponentDesc{"SP Buttons (all 8)", spKeyInfo, InputComponent::button, RB2DO, {.altConfig = true}},
		/* 2~4개짜리 묶음 — 화면에 몇 개만 얹고 싶을 때. Add New Button Group 에서 골라 쓴다. */
		InputComponentDesc{"SP Buttons (2)", {&spKeyInfo[0], 2}, InputComponent::button, RB2DO, {.altConfig = true}},
		InputComponentDesc{"SP Buttons (4)", {&spKeyInfo[0], 4}, InputComponent::button, RB2DO, {.altConfig = true}},
	};
	static constexpr SystemInputDeviceDesc gamepadDesc{"Gamepad", gamepadComponents};
	return gamepadDesc;
}

void AppMeta::onCustomizeNavView(AppNavView& view)
{
	const Gfx::LGradientStopDesc navViewGrad[] =
	{
		{ .0, Gfx::PackedColor::format.build((101./255.) * .4, (45./255.) * .4, (193./255.) * .4, 1.) },
		{ .3, Gfx::PackedColor::format.build((101./255.) * .4, (45./255.) * .4, (193./255.) * .4, 1.) },
		{ .97, Gfx::PackedColor::format.build((34./255.) * .4, (15./255.) * .4, (64./255.) * .4, 1.) },
		{ 1., view.separatorColor() },
	};
	view.setBackgroundGradient(navViewGrad);
}

}
