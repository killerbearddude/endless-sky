/* ReadOnlyOutfitterPanel.cpp
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

#include "ReadOnlyOutfitterPanel.h"

#include "CategoryList.h"
#include "CategoryType.h"
#include "Color.h"
#include "Edit.h"
#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "shader/LineShader.h"
#include "opengl.h"
#include "Outfit.h"
#include "OutfitInfoDisplay.h"
#include "PlayerInfo.h"
#include "Screen.h"
#include "Ship.h"
#include "image/Sprite.h"
#include "shader/SpriteShader.h"
#include "text/Truncate.h"
#include "text/DisplayText.h"
#include "text/Alignment.h"
#include "text/WrappedText.h"
#include "UI.h"

#include <algorithm>
#include <cctype>
#include <set>

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
			Border(Position(), Theme(HasFocus() ? "ui/focus" : "ui/control border"), HasFocus() ? 2.f : 1.f);
		}
	};

	string Lower(string value)
	{
		transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return tolower(c); });
		return value;
	}

	// The native game draws in centered logical coordinates. The scissor keeps long
	// descriptions and rows within their own panes at every effective UI zoom.
	void Clip(const Rectangle &r)
	{
		const double zoom = Screen::Zoom() / 100.;
		glEnable(GL_SCISSOR_TEST);
		glScissor(static_cast<int>((r.Left() - Screen::Left()) * zoom),
			static_cast<int>((Screen::Bottom() - r.Bottom()) * zoom),
			static_cast<int>(r.Width() * zoom), static_cast<int>(r.Height() * zoom));
	}

	void EndClip()
	{
		glDisable(GL_SCISSOR_TEST);
	}
}


ReadOnlyOutfitterPanel::ReadOnlyOutfitterPanel(const PlayerInfo &player, const Sale<Outfit> &stock)
	: player(player), stock(stock), search(make_shared<SearchEdit>())
{
	SetIsFullScreen(true);
	SetInterruptible(false);
	search->SetBgColor(Theme("ui/raised"));
	search->SetFontSize(14);
	search->SetCallback([this](string &) { queryDirty = true; return true; });
	AddChild(search);

	for(const auto &ship : player.Ships())
		if(ship && ship->GetPlanet() == player.GetPlanet())
			ships.push_back(ship.get());
	if(const Ship *flagship = player.Flagship())
	{
		auto it = find(ships.begin(), ships.end(), flagship);
		if(it != ships.end())
			shipIndex = it - ships.begin();
	}

	const CategoryList &native = GameData::GetCategory(CategoryType::OUTFIT);
	set<string> seen;
	for(const auto &item : GameData::Outfits())
	{
		const Outfit *outfit = &item.second;
		if(stock.Has(outfit) || player.Stock(outfit) > 0 || OwnedLicense(outfit) || player.Cargo().Get(outfit)
				|| StorageCount(outfit) || any_of(ships.begin(), ships.end(),
					[outfit](const Ship *ship) { return ship->OutfitCount(outfit) > 0; }))
		{
			all.push_back(outfit);
			seen.insert(outfit->Category());
		}
	}
	for(const auto &item : native)
		if(seen.erase(item.Name()))
			categories.push_back(item.Name());
	for(const auto &name : seen)
		categories.push_back(name);
	if(!categories.empty())
		category = categories.front();
	Refresh();
}


void ReadOnlyOutfitterPanel::Step()
{
	// No call into ShopPanel::Step, mission offers, refill, or PlayerInfo mutation.
	if(queryDirty)
	{
		queryDirty = false;
		Refresh();
	}
}


int ReadOnlyOutfitterPanel::Installed(const Outfit *outfit) const
{
	if(ships.empty())
		return 0;
	if(!allShips)
		return ships[shipIndex]->OutfitCount(outfit);
	int total = 0;
	for(const Ship *ship : ships)
		total += ship->OutfitCount(outfit);
	return total;
}


int ReadOnlyOutfitterPanel::StorageCount(const Outfit *outfit) const
{
	const auto &storage = player.PlanetaryStorage();
	const auto it = storage.find(player.GetPlanet());
	return it == storage.end() ? 0 : it->second.Get(outfit);
}


bool ReadOnlyOutfitterPanel::OwnedLicense(const Outfit *outfit) const
{
	static const string suffix = " License";
	const string &name = outfit->TrueName();
	return name.ends_with(suffix) && player.HasLicense(name.substr(0, name.size() - suffix.size()));
}


bool ReadOnlyOutfitterPanel::VisibleInSource(const Outfit *outfit) const
{
	switch(source)
	{
		case Source::ALL: return true;
		case Source::SHOP: return stock.Has(outfit) || player.Stock(outfit) > 0 || OwnedLicense(outfit);
		case Source::INSTALLED: return Installed(outfit) > 0;
		case Source::CARGO: return player.Cargo().Get(outfit) > 0;
		case Source::STORAGE: return StorageCount(outfit) > 0;
	}
	return false;
}


void ReadOnlyOutfitterPanel::Refresh()
{
	rows.clear();
	const string query = Lower(search->Text());
	for(const Outfit *outfit : all)
		if(outfit->Category() == category && VisibleInSource(outfit)
				&& (query.empty() || Lower(outfit->DisplayName()).find(query) != string::npos
					|| Lower(outfit->TrueName()).find(query) != string::npos))
			rows.push_back(outfit);
	stable_sort(rows.begin(), rows.end(), [this](const Outfit *a, const Outfit *b)
	{
		if(sort == Sort::PRICE && a->Cost() != b->Cost())
			return a->Cost() < b->Cost();
		if(a->DisplayName() != b->DisplayName())
			return a->DisplayName() < b->DisplayName();
		return a->TrueName() < b->TrueName();
	});
	if(find(rows.begin(), rows.end(), selected) == rows.end())
		selected = rows.empty() ? nullptr : rows.front();
	rowScroll.SetMaxValue(rows.size() * 52.);
	detailScroll.Set(0, 0);
}


void ReadOnlyOutfitterPanel::SelectRow(int index)
{
	if(index < 0 || index >= static_cast<int>(rows.size()))
		return;
	selected = rows[index];
	detailScroll.Set(0, 0);
	const double top = index * 52.;
	if(top < rowScroll.Value())
		rowScroll.Set(top, 0);
	else if(top + 52 > rowScroll.Value() + rowScroll.DisplaySize())
		rowScroll.Set(top + 52 - rowScroll.DisplaySize(), 0);
}


void ReadOnlyOutfitterPanel::CycleShip()
{
	if(ships.empty())
		return;
	shipIndex = (shipIndex + 1) % ships.size();
	allShips = false;
	Refresh();
}


void ReadOnlyOutfitterPanel::DrawControl(const Rectangle &bounds, const string &text, bool selected,
	const function<void()> &action)
{
	FillShader::Fill(bounds, Theme(selected ? "ui/selected" : "ui/raised"));
	Border(bounds, Theme(selected ? "ui/focus" : "ui/control border"), selected ? 2.f : 1.f);
	const Font &font = FontSet::Get(14);
	font.Draw({text, {static_cast<int>(bounds.Width() - 12), Alignment::CENTER, Truncate::MIDDLE}},
		bounds.TopLeft() + Point(6, 8), Theme("ui/text primary"));
	AddZone(bounds, action);
}


void ReadOnlyOutfitterPanel::Draw()
{
	FillShader::Fill(Rectangle(Point(), Screen::Dimensions()), Theme("ui/background"));
	ClearZones();
	const double left = Screen::Left() + 16;
	const double right = Screen::Right() - 16;
	const double top = Screen::Top() + 16;
	const double bottom = Screen::Bottom() - 16;
	const double width = right - left;
	compact = width < 1040;
	const Font &font = FontSet::Get(14);
	font.Draw("OUTFITTER CATALOG  |  READ ONLY", Point(left, top + 3), Theme("ui/text primary"));
	DrawControl(Rectangle::FromCorner(Point(right - 100, top), Point(100, 30)), "CLOSE  ESC", false,
		[this]() { GetUI().Pop(this); });

	const double controlTop = top + 42;
	const double tabWidth = min(120., (width - 16) / 5.);
	const char *sources[] = {"ALL", "SHOP", "INSTALLED", "CARGO", "STORAGE"};
	for(int i = 0; i < 5; ++i)
		DrawControl(Rectangle::FromCorner(Point(left + i * (tabWidth + 4), controlTop), Point(tabWidth, 30)),
			sources[i], source == static_cast<Source>(i), [this, i]() { source = static_cast<Source>(i); Refresh(); });

	const double searchTop = controlTop + 44;
	font.Draw("SEARCH", Point(left, searchTop + 8), Theme("ui/text secondary"));
	search->SetPosition(Rectangle::FromCorner(Point(left + 65, searchTop), Point(max(120., width * .43 - 65), 32)));
	DrawControl(Rectangle::FromCorner(Point(left + width * .45, searchTop), Point(112, 32)),
		sort == Sort::NAME ? "NAME SORT" : "PRICE SORT", false,
		[this]() { sort = sort == Sort::NAME ? Sort::PRICE : Sort::NAME; Refresh(); });
	const string shipLabel = ships.empty() ? "NO SHIP HERE" :
		(ships[shipIndex]->GivenName().empty() ? ships[shipIndex]->DisplayModelName() : ships[shipIndex]->GivenName());
	DrawControl(Rectangle::FromCorner(Point(right - 200, searchTop), Point(200, 32)),
		"SHIP: " + shipLabel, false, [this]() { CycleShip(); });
	if(ships.size() > 1)
		DrawControl(Rectangle::FromCorner(Point(right - 200, searchTop + 38), Point(200, 28)),
			allShips ? "ALL HERE (" + to_string(ships.size()) + ")" : "SELECT ALL HERE", allShips,
			[this]() { allShips = !allShips; Refresh(); });
	font.Draw("Tab: search   Left/Right: category   [ / ]: source   S: sort   H: ship   A: all ships",
		Point(left, searchTop + 54), Theme("ui/text secondary"));

	const double contentTop = searchTop + 76;
	const double contentHeight = max(80., bottom - contentTop);
	const double catWidth = compact ? 154. : 190.;
	const double detailWidth = compact ? 0. : min(350., width * .30);
	categoryBounds = Rectangle::FromCorner(Point(left, contentTop), Point(catWidth, contentHeight));
	rowBounds = Rectangle::FromCorner(Point(left + catWidth + 8, contentTop),
		Point(width - catWidth - detailWidth - (compact ? 8 : 16), contentHeight));
	detailBounds = compact ? rowBounds : Rectangle::FromCorner(Point(rowBounds.Right() + 8, contentTop),
		Point(detailWidth, contentHeight));
	DrawCategories(categoryBounds);
	if(compact && detailsPage)
	{
		DrawDetails(detailBounds);
		DrawControl(Rectangle::FromCorner(Point(rowBounds.Left() + 8, rowBounds.Top() + 8), Point(88, 28)),
			"BACK", false, [this]() { detailsPage = false; });
	}
	else
	{
		DrawCatalog(rowBounds);
		if(!compact)
			DrawDetails(detailBounds);
	}
}


void ReadOnlyOutfitterPanel::DrawCategories(const Rectangle &bounds)
{
	FillShader::Fill(bounds, Theme("ui/panel"));
	Border(bounds, Theme("ui/divider"));
	categoryScroll.SetDisplaySize(bounds.Height() - 36);
	categoryScroll.SetMaxValue(categories.size() * 32.);
	const Font &font = FontSet::Get(14);
	font.Draw("CATEGORIES", bounds.TopLeft() + Point(10, 10), Theme("ui/text secondary"));
	Clip(Rectangle::FromCorner(bounds.TopLeft() + Point(1, 34), Point(bounds.Width() - 2, bounds.Height() - 35)));
	for(size_t i = 0; i < categories.size(); ++i)
	{
		const Rectangle row = Rectangle::FromCorner(Point(bounds.Left() + 5,
			bounds.Top() + 35 + i * 32 - categoryScroll.Value()), Point(bounds.Width() - 10, 30));
		if(row.Bottom() < bounds.Top() + 34 || row.Top() > bounds.Bottom())
			continue;
		FillShader::Fill(row, Theme(categories[i] == category ? "ui/selected" : "ui/panel"));
		if(categories[i] == category)
			Border(row, Theme("ui/focus"), 2.f);
		font.Draw({categories[i], {static_cast<int>(row.Width() - 12), Alignment::LEFT, Truncate::MIDDLE}},
			row.TopLeft() + Point(6, 8), Theme("ui/text primary"));
		if(row.Top() >= bounds.Top() + 34 && row.Bottom() <= bounds.Bottom())
			AddZone(row, [this, i]() { category = categories[i]; rowScroll.Set(0, 0); Refresh(); });
	}
	EndClip();
}


void ReadOnlyOutfitterPanel::DrawCatalog(const Rectangle &bounds)
{
	FillShader::Fill(bounds, Theme("ui/panel"));
	Border(bounds, Theme("ui/divider"));
	const Font &font = FontSet::Get(14);
	font.Draw(category + "  (" + to_string(rows.size()) + ")", bounds.TopLeft() + Point(10, 9), Theme("ui/text primary"));
	rowScroll.SetDisplaySize(bounds.Height() - 42);
	rowScroll.SetMaxValue(rows.size() * 52.);
	const Rectangle viewport = Rectangle::FromCorner(bounds.TopLeft() + Point(1, 40),
		Point(bounds.Width() - 2, bounds.Height() - 41));
	if(rows.empty())
		font.Draw("No outfits in this category and source.", viewport.TopLeft() + Point(12, 18), Theme("ui/text secondary"));
	Clip(viewport);
	for(size_t i = 0; i < rows.size(); ++i)
	{
		const Outfit *outfit = rows[i];
		const Rectangle row = Rectangle::FromCorner(Point(bounds.Left() + 4,
			viewport.Top() + i * 52 - rowScroll.Value()), Point(bounds.Width() - 8, 50));
		if(row.Bottom() < viewport.Top() || row.Top() > viewport.Bottom())
			continue;
		FillShader::Fill(row, Theme(outfit == selected ? "ui/selected" : "ui/raised"));
		if(outfit == selected)
			Border(row, Theme("ui/focus"), 2.f);
		if(const Sprite *sprite = outfit->Thumbnail().GetSprite(); sprite && sprite->IsLoaded())
			SpriteShader::Draw(sprite, row.TopLeft() + Point(25, 25), min(36. / max(sprite->Width(), sprite->Height()), 1.));
		const double nameWidth = max(40., row.Width() - 185);
		font.Draw({outfit->DisplayName(), {static_cast<int>(nameWidth), Alignment::LEFT, Truncate::MIDDLE}},
			row.TopLeft() + Point(50, 7), Theme("ui/text primary"));
		font.Draw(Format::AbbreviatedNumber(outfit->Cost()) + " cr", row.TopRight() + Point(-115, 7), Theme("ui/text secondary"));
		string context = stock.Has(outfit) ? "SOLD HERE" :
			(player.Stock(outfit) > 0 ? "LOCAL STOCK" : (OwnedLicense(outfit) ? "LICENSE HELD" : "OWNED"));
		context += "  I:" + to_string(Installed(outfit)) + " C:" + to_string(player.Cargo().Get(outfit))
			+ " S:" + to_string(StorageCount(outfit));
		font.Draw({context, {static_cast<int>(row.Width() - 54), Alignment::LEFT, Truncate::BACK}},
			row.TopLeft() + Point(50, 27), Theme("ui/text muted"));
		if(row.Top() >= viewport.Top() && row.Bottom() <= viewport.Bottom())
			AddZone(row, [this, i]() { SelectRow(i); if(compact) detailsPage = true; });
	}
	EndClip();
}


void ReadOnlyOutfitterPanel::DrawDetails(const Rectangle &bounds)
{
	FillShader::Fill(bounds, Theme("ui/panel"));
	Border(bounds, Theme("ui/divider"));
	if(!selected)
	{
		FontSet::Get(14).Draw("No outfit selected.", bounds.TopLeft() + Point(12, 48), Theme("ui/text secondary"));
		return;
	}
	const Font &font = FontSet::Get(14);
	const Rectangle viewport = Rectangle::FromCorner(bounds.TopLeft() + Point(1, 40),
		Point(bounds.Width() - 2, bounds.Height() - 41));
	const double x = bounds.Left() + 12;
	const double width = bounds.Width() - 24;
	const double titleInset = compact && detailsPage ? 110. : 12.;
	font.Draw({selected->DisplayName(), {static_cast<int>(bounds.Width() - titleInset - 12), Alignment::LEFT, Truncate::MIDDLE}},
		bounds.TopLeft() + Point(titleInset, 10), Theme("ui/text primary"));
	WrappedText description(font);
	description.SetWrapWidth(max(80, static_cast<int>(width)));
	description.Wrap(selected->Description());
	OutfitInfoDisplay info(*selected, player, false, false);
	const double contentHeight = 322 + description.Height() + info.RequirementsHeight() + info.AttributesHeight();
	detailScroll.SetDisplaySize(viewport.Height());
	detailScroll.SetMaxValue(contentHeight);
	const double offset = detailScroll.Value();
	Clip(viewport);
	const Sprite *sprite = selected->Thumbnail().GetSprite();
	if(sprite && sprite->IsLoaded())
		SpriteShader::Draw(sprite, Point(bounds.Center().X(), viewport.Top() + 55 - offset),
			min(90. / max(sprite->Width(), sprite->Height()), 1.));
	double y = viewport.Top() + 110 - offset;
	font.Draw("Base price: " + Format::Number(selected->Cost()) + " credits", Point(x, y), Theme("ui/text secondary"));
	y += 24;
	font.Draw("Sold here: " + string(stock.Has(selected) ? "yes" : "no")
		+ "   Local stock: " + to_string(player.Stock(selected)), Point(x, y), Theme("ui/text secondary"));
	y += 22;
	if(OwnedLicense(selected))
	{
		font.Draw("License held: yes", Point(x, y), Theme("ui/text secondary"));
		y += 22;
	}
	font.Draw("Installed: " + to_string(Installed(selected)) +
		(ships.empty() ? " (no ship selected)" : (allShips ? " across selected ships" : " on selected ship")),
		Point(x, y), Theme("ui/text secondary"));
	y += 22;
	font.Draw("Fleet cargo: " + to_string(player.Cargo().Get(selected))
		+ "   Local storage: " + to_string(StorageCount(selected)), Point(x, y), Theme("ui/text secondary"));
	y += 24;
	if(!ships.empty())
	{
		const Ship *ship = ships[shipIndex];
		const string shipName = ship->GivenName().empty() ? ship->DisplayModelName() :
			ship->GivenName() + " (" + ship->DisplayModelName() + ")";
		font.Draw({allShips ? to_string(ships.size()) + " ships here selected" : shipName,
			{static_cast<int>(width), Alignment::LEFT, Truncate::MIDDLE}},
			Point(x, y), Theme("ui/text primary"));
		y += 22;
		auto space = [this](const char *name)
		{
			double low = ships[shipIndex]->Attributes().Get(name);
			double high = low;
			if(allShips)
				for(const Ship *candidate : ships)
				{
					const double value = candidate->Attributes().Get(name);
					low = min(low, value);
					high = max(high, value);
				}
			return Format::Number(low) + (low == high ? "" : " - " + Format::Number(high));
		};
		font.Draw("Space  outfit: " + space("outfit space") + "  weapon: " + space("weapon capacity"),
			Point(x, y), Theme("ui/text secondary"));
		y += 22;
		font.Draw("Engine: " + space("engine capacity"), Point(x, y), Theme("ui/text secondary"));
		y += 28;
	}
	else
		y += 12;
	description.Draw(Point(x, y), Theme("ui/text primary"));
	y += description.Height() + 8;
	info.DrawRequirements(Point(x, y));
	y += info.RequirementsHeight();
	info.DrawAttributes(Point(x, y));
	EndClip();
}


bool ReadOnlyOutfitterPanel::KeyDown(SDL_Keycode key, Uint16, const Command &, bool isNewPress)
{
	if(!isNewPress)
		return true;
	if(key == SDLK_ESCAPE)
	{
		if(detailsPage)
			detailsPage = false;
		else
			GetUI().Pop(this);
	}
	else if(key == SDLK_UP || key == SDLK_DOWN)
	{
		if(detailsPage)
			detailScroll.Scroll(key == SDLK_UP ? -40. : 40., 0);
		else
		{
			auto it = find(rows.begin(), rows.end(), selected);
			const int index = it == rows.end() ? 0 : static_cast<int>(it - rows.begin());
			SelectRow(index + (key == SDLK_UP ? -1 : 1));
		}
	}
	else if(key == SDLK_LEFT || key == SDLK_RIGHT)
	{
		auto it = find(categories.begin(), categories.end(), category);
		if(it != categories.end())
		{
			const int next = static_cast<int>(it - categories.begin()) + (key == SDLK_LEFT ? -1 : 1);
			if(next >= 0 && next < static_cast<int>(categories.size()))
			{
				category = categories[next];
				const double top = next * 32.;
				if(top < categoryScroll.Value())
					categoryScroll.Set(top, 0);
				else if(top + 32 > categoryScroll.Value() + categoryScroll.DisplaySize())
					categoryScroll.Set(top + 32 - categoryScroll.DisplaySize(), 0);
				rowScroll.Set(0, 0);
				Refresh();
			}
		}
	}
	else if(key == SDLK_LEFTBRACKET || key == SDLK_RIGHTBRACKET)
	{
		const int next = static_cast<int>(source) + (key == SDLK_LEFTBRACKET ? -1 : 1);
		if(next >= static_cast<int>(Source::ALL) && next <= static_cast<int>(Source::STORAGE))
		{
			source = static_cast<Source>(next);
			Refresh();
		}
	}
	else if(key == SDLK_s)
	{
		sort = sort == Sort::NAME ? Sort::PRICE : Sort::NAME;
		Refresh();
	}
	else if(key == SDLK_h)
		CycleShip();
	else if(key == SDLK_a && ships.size() > 1)
	{
		allShips = !allShips;
		Refresh();
	}
	else if(key == SDLK_TAB || key == SDLK_SLASH)
		search->SetFocus(true);
	else if(key == SDLK_RETURN && compact && selected)
		detailsPage = true;
	else
		return true; // Consume all other game shortcuts while this panel is active.
	return true;
}


bool ReadOnlyOutfitterPanel::Scroll(double, double dy)
{
	if(hoverPane == HoverPane::CATEGORY)
		categoryScroll.Scroll(-dy * 30., 0);
	else if(hoverPane == HoverPane::DETAILS || (compact && detailsPage))
		detailScroll.Scroll(-dy * 30., 0);
	else
		rowScroll.Scroll(-dy * 30., 0);
	return true;
}


bool ReadOnlyOutfitterPanel::Hover(int x, int y)
{
	const Point point(x, y);
	hoverPane = categoryBounds.Contains(point) ? HoverPane::CATEGORY :
		(detailBounds.Contains(point) && (!compact || detailsPage) ? HoverPane::DETAILS : HoverPane::ROWS);
	return true;
}
