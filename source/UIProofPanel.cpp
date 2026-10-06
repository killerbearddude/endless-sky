/* UIProofPanel.cpp
Copyright (c) 2026 by Daniel

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "UIProofPanel.h"

#include "Color.h"
#include "Edit.h"
#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "GameData.h"
#include "shader/LineShader.h"
#include "Screen.h"
#include "UI.h"

#include <algorithm>
#include <array>
#include <functional>
#include <string>

using namespace std;

namespace {
	const Color &Theme(const char *name)
	{
		return *GameData::Colors().Get(name);
	}

	void Border(const Rectangle &r, const Color &color, float width = 1.f)
	{
		LineShader::Draw(r.TopLeft(), r.TopRight(), width, color);
		LineShader::Draw(r.TopRight(), r.BottomRight(), width, color);
		LineShader::Draw(r.BottomRight(), r.BottomLeft(), width, color);
		LineShader::Draw(r.BottomLeft(), r.TopLeft(), width, color);
	}

	class ProofPopup final : public Panel {
	public:
		explicit ProofPopup(function<void()> onClose) : onClose(move(onClose))
		{
			SetInterruptible(false);
		}

		void Draw() override
		{
			FillShader::Fill(box, Theme("ui/raised"));
			Border(box, Theme("ui/focus"));
			const Font &font = FontSet::Get(14);
			font.Draw("Popup: input stays here", box.TopLeft() + Point(16, 20), Theme("ui/text primary"));
			font.Draw("Escape, Enter, or click outside to close.",
				box.TopLeft() + Point(16, 60), Theme("ui/text secondary"));
		}

	protected:
		bool KeyDown(SDL_Keycode key, Uint16, const Command &, bool isNewPress) override
		{
			if((key == SDLK_ESCAPE || key == SDLK_RETURN) && isNewPress)
			{
				Close();
				return true;
			}
			return true;
		}

		bool Click(int x, int y, MouseButton, int) override
		{
			if(!box.Contains(Point(x, y)))
				Close();
			return true;
		}

	private:
		void Close()
		{
			GetUI().Pop(this);
			onClose();
		}

		function<void()> onClose;
		const Rectangle box{Point(), Point(360, 160)};
	};

	constexpr array<const char *, 14> ROLE_NAMES = {
		"ui/background", "ui/panel", "ui/raised", "ui/hover", "ui/divider",
		"ui/control border", "ui/text primary", "ui/text secondary", "ui/text muted",
		"ui/focus", "ui/selected", "ui/positive", "ui/caution", "ui/danger"
	};
}



UIProofPanel::UIProofPanel()
	: first(make_shared<Edit>()), second(make_shared<Edit>())
{
	SetIsFullScreen(true);
	SetInterruptible(false);
	first->SetBgColor(Theme("ui/raised"));
	second->SetBgColor(Theme("ui/raised"));
	AddChild(first);
	AddChild(second);
}



void UIProofPanel::Step()
{
	if(!initialFocusSet)
		initialFocusSet = first->SetFocus(true);
}



void UIProofPanel::Draw()
{
	FillShader::Fill(Rectangle(Point(), Screen::Dimensions()), Theme("ui/background"));

	const double width = min(760., static_cast<double>(Screen::Width() - 32));
	const double height = min(580., static_cast<double>(Screen::Height() - 32));
	const Rectangle panel(Point(), Point(width, height));
	FillShader::Fill(panel, Theme("ui/panel"));
	Border(panel, Theme("ui/divider"));

	const Font &font = FontSet::Get(14);
	const double left = panel.Left() + 24;
	const double top = panel.Top();
	font.Draw("M1-A NATIVE UI PROOF", Point(left, top + 20), Theme("ui/text primary"));
	font.Draw("Type b, m, or other shortcuts in the focused field.",
		Point(left, top + 48), Theme("ui/text secondary"));

	const Point fieldSize(width - 48, 34);
	const Rectangle firstBounds(Point(panel.Center().X(), top + 100), fieldSize);
	const Rectangle secondBounds(Point(panel.Center().X(), top + 160), fieldSize);
	first->SetPosition(firstBounds);
	second->SetPosition(secondBounds);
	font.Draw("FIRST FIELD", Point(left, firstBounds.Top() - 19), Theme("ui/text muted"));
	font.Draw("SECOND FIELD", Point(left, secondBounds.Top() - 19), Theme("ui/text muted"));
	Border(firstBounds, first->HasFocus() ? Theme("ui/focus") : Theme("ui/control border"),
		first->HasFocus() ? 2.f : 1.f);
	Border(secondBounds, second->HasFocus() ? Theme("ui/focus") : Theme("ui/control border"),
		second->HasFocus() ? 2.f : 1.f);
	if(first->HasFocus())
		font.Draw("1", firstBounds.TopRight() + Point(-14, -17), Theme("ui/focus"));
	if(second->HasFocus())
		font.Draw("2", secondBounds.TopRight() + Point(-14, -17), Theme("ui/focus"));

	popupButton = Rectangle::FromCorner(Point(left, top + 203), Point(160, 34));
	FillShader::Fill(popupButton, Theme(hoveringButton ? "ui/hover" : "ui/selected"));
	Border(popupButton, Theme("ui/control border"));
	font.Draw("OPEN POPUP (P)", popupButton.TopLeft() + Point(10, 8), Theme("ui/text primary"));
	ClearZones();
	AddZone(popupButton, [this]() { OpenPopup(); });

	font.Draw("Tab / Shift+Tab: focus cycle   Escape: release focus or close",
		Point(left, top + 256), Theme("ui/text secondary"));
	font.Draw("Parent B shortcut count: " + to_string(shortcutCount),
		Point(left, top + 280), Theme("ui/text primary"));
	font.Draw("Status:", Point(left, top + 304), Theme("ui/text muted"));
	font.Draw("READY", Point(left + 65, top + 304), Theme("ui/positive"));
	font.Draw("CAUTION", Point(left + 120, top + 304), Theme("ui/caution"));
	font.Draw("DANGER", Point(left + 205, top + 304), Theme("ui/danger"));

	const double swatchTop = panel.Bottom() - 166;
	const double columnWidth = (width - 48) / 3;
	for(size_t i = 0; i < ROLE_NAMES.size(); ++i)
	{
		const int col = i % 3;
		const int row = i / 3;
		const Point origin(left + col * columnWidth, swatchTop + row * 30);
		const Rectangle swatch = Rectangle::FromCorner(origin, Point(22, 20));
		FillShader::Fill(swatch, Theme(ROLE_NAMES[i]));
		Border(swatch, Theme("ui/control border"));
		font.Draw(string(ROLE_NAMES[i]).substr(3), origin + Point(30, 2), Theme("ui/text primary"));
	}
}



bool UIProofPanel::KeyDown(SDL_Keycode key, Uint16, const Command &, bool isNewPress)
{
	if(!isNewPress)
		return true;
	if(key == SDLK_ESCAPE)
		GetUI().Pop(this);
	else if(key == SDLK_TAB)
		first->SetFocus(true);
	else if(key == 'p')
		OpenPopup();
	else if(key == 'b')
		++shortcutCount;
	else
		return false;
	return true;
}



bool UIProofPanel::Hover(int x, int y)
{
	hoveringButton = popupButton.Contains(Point(x, y));
	return hoveringButton;
}



void UIProofPanel::OpenPopup()
{
	first->SetFocus(false);
	second->SetFocus(false);
	GetUI().Push(new ProofPopup([this]() { RestoreFocus(); }));
}



void UIProofPanel::RestoreFocus()
{
	first->SetFocus(true);
}
