/* ModernShopStyle.h
Copyright (c) 2026 by Daniel

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.
*/

#pragma once

#include "Color.h"
#include "Command.h"
#include "Edit.h"
#include "GameData.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "shader/LineShader.h"
#include "opengl.h"
#include "Rectangle.h"
#include "Screen.h"

#include <algorithm>
#include <cctype>
#include <string>

// Drawing and search behavior shared by the modern Outfitter and Shipyard.
// Catalog data, details, fleet state, and transactions stay in their panels.
namespace ModernShopStyle {
	inline const Color &Theme(const char *name)
	{
		return *GameData::Colors().Get(name);
	}

	inline void Border(const Rectangle &r, const Color &color, float width = 1.f)
	{
		LineShader::Draw(r.TopLeft(), r.TopRight(), width, color);
		LineShader::Draw(r.TopRight(), r.BottomRight(), width, color);
		LineShader::Draw(r.BottomRight(), r.BottomLeft(), width, color);
		LineShader::Draw(r.BottomLeft(), r.TopLeft(), width, color);
	}

	inline void Clip(const Rectangle &r)
	{
		const double zoom = Screen::Zoom() / 100.;
		glEnable(GL_SCISSOR_TEST);
		glScissor(static_cast<int>((r.Left() - Screen::Left()) * zoom),
			static_cast<int>((Screen::Bottom() - r.Bottom()) * zoom),
			static_cast<int>(r.Width() * zoom), static_cast<int>(r.Height() * zoom));
	}

	inline void EndClip()
	{
		glDisable(GL_SCISSOR_TEST);
	}

	inline std::string Lower(std::string value)
	{
		std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return std::tolower(c); });
		return value;
	}

	class SearchEdit final : public Edit {
	public:
		bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override
		{
			if(key == SDLK_TAB)
			{
				SetFocus(false);
				return true;
			}
			return Edit::KeyDown(key, mod, command, isNewPress);
		}

		void Draw() override
		{
			Edit::Draw();
			if(Text().empty() && !HasFocus())
				FontSet::Get(14).Draw("Search...", Position().TopLeft() + Point(8, 8), Theme("ui/text muted"));
			if(HasFocus())
				Border(Position(), Theme("ui/focus"), 2.f);
		}
	};
}
