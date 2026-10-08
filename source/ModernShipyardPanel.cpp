/* ModernShipyardPanel.cpp
Copyright (c) 2026 by Daniel

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.
*/

#include "ModernShipyardPanel.h"
#include "ModernShopStyle.h"

#include "CategoryList.h"
#include "CategoryType.h"
#include "Color.h"
#include "Command.h"
#include "comparators/BySeriesAndIndex.h"
#include "DialogPanel.h"
#include "Edit.h"
#include "shader/FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "shader/LineShader.h"
#include "opengl.h"
#include "PlayerInfo.h"
#include "Screen.h"
#include "Ship.h"
#include "image/Sprite.h"
#include "shader/SpriteShader.h"
#include "text/Truncate.h"
#include "text/DisplayText.h"
#include "text/Alignment.h"
#include "UI.h"

#include <algorithm>
#include <cctype>
#include <set>

using namespace std;
using namespace ModernShopStyle;



ModernShipyardPanel::ModernShipyardPanel(PlayerInfo &player, const Sale<Ship> &stock)
	: ShipyardPanel(player, stock), stock(stock), search(make_shared<SearchEdit>())
{
	search->SetBgColor(Theme("ui/raised"));
	search->SetFontSize(14);
	search->SetCallback([this](string &) { queryDirty = true; return true; });
	AddChild(search);
	RebuildCatalog();
	RefreshFleet();
}


void ModernShipyardPanel::Step()
{
	ShipyardPanel::Step();
	RefreshFleet();
	if(queryDirty)
	{
		queryDirty = false;
		Refresh();
	}
}


void ModernShipyardPanel::RebuildCatalog()
{
	all.clear();
	categoryNames.clear();
	set<string> seen;
	for(const auto &entry : GameData::Ships())
	{
		const Ship *ship = &entry.second;
		if(stock.Has(ship))
		{
			all.push_back(ship);
			seen.insert(ship->Attributes().Category());
		}
	}
	for(const auto &entry : GameData::GetCategory(CategoryType::SHIP))
		if(seen.erase(entry.Name()))
			categoryNames.push_back(entry.Name());
	for(const string &name : seen)
		categoryNames.push_back(name);
	if(find(categoryNames.begin(), categoryNames.end(), category) == categoryNames.end())
		category = categoryNames.empty() ? "" : categoryNames.front();
	Refresh();
}


void ModernShipyardPanel::Refresh()
{
	rows.clear();
	const string query = Lower(search->Text());
	for(const Ship *ship : all)
		if(ship->Attributes().Category() == category &&
				(query.empty() || Lower(ship->DisplayModelName()).find(query) != string::npos
					|| Lower(ship->VariantName()).find(query) != string::npos))
			rows.push_back(ship);
	if(sort == Sort::NATIVE)
		stable_sort(rows.begin(), rows.end(), [](const Ship *a, const Ship *b) {
			return BySeriesAndIndex<Ship>()(a->VariantName(), b->VariantName());
		});
	else
		stable_sort(rows.begin(), rows.end(), [this](const Ship *a, const Ship *b) {
			if(sort == Sort::PRICE && a->Cost() != b->Cost())
				return a->Cost() < b->Cost();
			return a->DisplayModelName() < b->DisplayModelName();
		});
	if(find(rows.begin(), rows.end(), selected) == rows.end())
		selected = rows.empty() ? nullptr : rows.front();
	selectedShip = selected;
	rowScroll.SetMaxValue(rows.size() * 52.);
	detailScroll.Set(0., 0);
}


void ModernShipyardPanel::RefreshFleet()
{
	vector<const Ship *> here;
	for(const auto &ship : player.Ships())
		if(ship && ship->GetPlanet() == player.GetPlanet())
			here.push_back(ship.get());
	if(here == fleet)
		return;
	fleet = move(here);
	fleetScroll.SetMaxValue(fleet.size() * 32.);
	if(playerShip && find(fleet.begin(), fleet.end(), playerShip) == fleet.end())
	{
		playerShip = nullptr;
		playerShips.clear();
	}
}


void ModernShipyardPanel::SelectRow(int index)
{
	if(index < 0 || index >= static_cast<int>(rows.size()))
		return;
	selected = rows[index];
	selectedShip = selected;
	detailScroll.Set(0., 0);
	const double top = index * 52.;
	if(top < rowScroll.Value())
		rowScroll.Set(top, 0);
	else if(top + 52 > rowScroll.Value() + rowScroll.DisplaySize())
		rowScroll.Set(top + 52 - rowScroll.DisplaySize(), 0);
}


void ModernShipyardPanel::DrawControl(const Rectangle &bounds, const string &label, bool selected,
	const function<void()> &action, bool enabled)
{
	FillShader::Fill(bounds, Theme(selected ? "ui/selected" : "ui/raised"));
	Border(bounds, Theme(selected ? "ui/focus" : (enabled ? "ui/control border" : "ui/divider")),
		selected ? 2.f : 1.f);
	FontSet::Get(14).Draw({label, {static_cast<int>(bounds.Width() - 12), Alignment::CENTER, Truncate::MIDDLE}},
		bounds.TopLeft() + Point(6, 8), Theme(enabled ? "ui/text primary" : "ui/text muted"));
	AddZone(bounds, action);
}


void ModernShipyardPanel::Draw()
{
	FillShader::Fill(Rectangle(Point(), Screen::Dimensions()), Theme("ui/background"));
	ClearZones();
	shipZones.clear();
	const int modifier = Modifier();
	if(modifier > 1)
	{
		selectedQuantity->SetText(to_string(modifier));
		quantityIsModifier = true;
	}
	else if(quantityIsModifier)
	{
		selectedQuantity->SetText("1");
		quantityIsModifier = false;
	}
	const double left = Screen::Left() + 16;
	const double right = Screen::Right() - 16;
	const double top = Screen::Top() + 16;
	const double bottom = Screen::Bottom() - 16;
	const double width = right - left;
	compact = width < 1040;
	FontSet::Get(14).Draw("MODERN SHIPYARD", Point(left, top + 4), Theme("ui/text primary"));
	DrawControl(Rectangle::FromCorner(Point(right - 100, top), Point(100, 30)), "CLOSE  ESC", false,
		[this]() { ShopPanel::KeyDown(SDLK_ESCAPE, 0, Command(), true); });
	const double searchTop = top + 46;
	FontSet::Get(14).Draw("SEARCH", Point(left, searchTop + 8), Theme("ui/text secondary"));
	search->SetPosition(Rectangle::FromCorner(Point(left + 65, searchTop), Point(max(120., width * .42 - 65), 32)));
	const char *sortName = sort == Sort::NATIVE ? "NATIVE ORDER" : sort == Sort::NAME ? "NAME SORT" : "PRICE SORT";
	DrawControl(Rectangle::FromCorner(Point(left + width * .44, searchTop), Point(132, 32)), sortName, false,
		[this]() { sort = static_cast<Sort>((static_cast<int>(sort) + 1) % 3); Refresh(); });
	FontSet::Get(14).Draw("F/Tab search   Left/Right category   V sort   B buy   S sell   U sell hull   K park   0-9 groups",
		Point(left, searchTop + 43), Theme("ui/text secondary"));
	const double contentTop = searchTop + 68;
	const double contentHeight = max(80., bottom - contentTop);
	const double sideWidth = compact ? 154. : 190.;
	const double detailsWidth = compact ? 0. : min(350., width * .30);
	const double fleetHeight = min(contentHeight * .45, max(100., fleet.size() * 32. + 40.));
	categoryBounds = Rectangle::FromCorner(Point(left, contentTop), Point(sideWidth, contentHeight - fleetHeight - 6));
	fleetBounds = Rectangle::FromCorner(Point(left, categoryBounds.Bottom() + 6), Point(sideWidth, fleetHeight));
	rowBounds = Rectangle::FromCorner(Point(left + sideWidth + 8, contentTop),
		Point(width - sideWidth - detailsWidth - (compact ? 8 : 16), contentHeight));
	const double actionHeight = 162.;
	detailBounds = compact
		? Rectangle::FromCorner(rowBounds.TopLeft(), Point(rowBounds.Width(), contentHeight - actionHeight))
		: Rectangle::FromCorner(Point(rowBounds.Right() + 8, contentTop), Point(detailsWidth, contentHeight - actionHeight));
	DrawCategories(categoryBounds);
	DrawFleet(fleetBounds);
	if(compact && detailsPage)
	{
		DrawDetailsPane(detailBounds);
		DrawActions(Rectangle::FromCorner(Point(detailBounds.Left(), detailBounds.Bottom()),
			Point(detailBounds.Width(), actionHeight)));
		DrawControl(Rectangle::FromCorner(rowBounds.TopLeft() + Point(8, 8), Point(88, 28)),
			"BACK", false, [this]() { detailsPage = false; });
	}
	else
	{
		DrawCatalog(rowBounds);
		if(!compact)
		{
			DrawDetailsPane(detailBounds);
			DrawActions(Rectangle::FromCorner(Point(detailBounds.Left(), detailBounds.Bottom()),
				Point(detailBounds.Width(), actionHeight)));
		}
	}
	if(compact && !detailsPage)
		selectedQuantity->SetPosition(Rectangle(Point(Screen::Right() + 100, Screen::Bottom() + 100), Point()));
}


void ModernShipyardPanel::DrawCategories(const Rectangle &bounds)
{
	FillShader::Fill(bounds, Theme("ui/panel"));
	Border(bounds, Theme("ui/divider"));
	FontSet::Get(14).Draw("CATEGORIES", bounds.TopLeft() + Point(10, 9), Theme("ui/text secondary"));
	const Rectangle viewport = Rectangle::FromCorner(bounds.TopLeft() + Point(1, 34),
		Point(bounds.Width() - 2, bounds.Height() - 35));
	categoryScroll.SetDisplaySize(viewport.Height());
	categoryScroll.SetMaxValue(categoryNames.size() * 32.);
	Clip(viewport);
	for(size_t i = 0; i < categoryNames.size(); ++i)
	{
		const Rectangle row = Rectangle::FromCorner(Point(bounds.Left() + 5,
			viewport.Top() + i * 32 - categoryScroll.Value()), Point(bounds.Width() - 10, 30));
		if(row.Bottom() < viewport.Top() || row.Top() > viewport.Bottom())
			continue;
		FillShader::Fill(row, Theme(categoryNames[i] == category ? "ui/selected" : "ui/panel"));
		if(categoryNames[i] == category)
			Border(row, Theme("ui/focus"), 2.f);
		FontSet::Get(14).Draw({categoryNames[i], {static_cast<int>(row.Width() - 12), Alignment::LEFT, Truncate::MIDDLE}},
			row.TopLeft() + Point(6, 8), Theme("ui/text primary"));
		if(row.Top() >= viewport.Top() && row.Bottom() <= viewport.Bottom())
			AddZone(row, [this, i]() { category = categoryNames[i]; rowScroll.Set(0., 0); Refresh(); });
	}
	EndClip();
}


void ModernShipyardPanel::DrawCatalog(const Rectangle &bounds)
{
	FillShader::Fill(bounds, Theme("ui/panel"));
	Border(bounds, Theme("ui/divider"));
	const Font &font = FontSet::Get(14);
	font.Draw(category + "  (" + to_string(rows.size()) + ")", bounds.TopLeft() + Point(10, 9), Theme("ui/text primary"));
	const Rectangle viewport = Rectangle::FromCorner(bounds.TopLeft() + Point(1, 40),
		Point(bounds.Width() - 2, bounds.Height() - 41));
	rowScroll.SetDisplaySize(viewport.Height());
	if(rows.empty())
		font.Draw("No ships match this category and search.", viewport.TopLeft() + Point(12, 18), Theme("ui/text secondary"));
	Clip(viewport);
	for(size_t i = 0; i < rows.size(); ++i)
	{
		const Ship *ship = rows[i];
		const Rectangle row = Rectangle::FromCorner(Point(bounds.Left() + 4,
			viewport.Top() + i * 52 - rowScroll.Value()), Point(bounds.Width() - 8, 50));
		if(row.Bottom() < viewport.Top() || row.Top() > viewport.Bottom())
			continue;
		FillShader::Fill(row, Theme(ship == selected ? "ui/selected" : "ui/raised"));
		if(ship == selected)
			Border(row, Theme("ui/focus"), 2.f);
		if(const Sprite *sprite = ship->Thumbnail().GetSprite(); sprite && sprite->IsLoaded())
			SpriteShader::Draw(sprite, row.TopLeft() + Point(25, 25),
				min(36. / max(sprite->Width(), sprite->Height()), 1.));
		font.Draw({ship->DisplayModelName(), {static_cast<int>(max(40., row.Width() - 170)), Alignment::LEFT, Truncate::MIDDLE}},
			row.TopLeft() + Point(50, 7), Theme("ui/text primary"));
		font.Draw(Format::AbbreviatedNumber(ship->Cost()) + " cr", row.TopRight() + Point(-115, 7),
			Theme("ui/text secondary"));
		font.Draw({ship->VariantName(), {static_cast<int>(row.Width() - 54), Alignment::LEFT, Truncate::BACK}},
			row.TopLeft() + Point(50, 27), Theme("ui/text muted"));
		if(row.Top() >= viewport.Top() && row.Bottom() <= viewport.Bottom())
			AddZone(row, [this, i]() { SelectRow(i); if(compact) detailsPage = true; });
	}
	EndClip();
}


void ModernShipyardPanel::DrawFleet(const Rectangle &bounds)
{
	FillShader::Fill(bounds, Theme("ui/panel"));
	Border(bounds, Theme("ui/divider"));
	FontSet::Get(14).Draw("SHIPS HERE (" + to_string(fleet.size()) + ")", bounds.TopLeft() + Point(10, 9),
		Theme("ui/text secondary"));
	const Rectangle viewport = Rectangle::FromCorner(bounds.TopLeft() + Point(1, 34),
		Point(bounds.Width() - 2, bounds.Height() - 35));
	fleetScroll.SetDisplaySize(viewport.Height());
	Clip(viewport);
	for(size_t i = 0; i < fleet.size(); ++i)
	{
		const Ship *ship = fleet[i];
		const Rectangle row = Rectangle::FromCorner(Point(bounds.Left() + 4,
			viewport.Top() + i * 32 - fleetScroll.Value()), Point(bounds.Width() - 8, 30));
		if(row.Bottom() < viewport.Top() || row.Top() > viewport.Bottom())
			continue;
		FillShader::Fill(row, Theme(playerShips.contains(const_cast<Ship *>(ship)) ? "ui/selected" : "ui/raised"));
		if(playerShips.contains(const_cast<Ship *>(ship)))
			Border(row, Theme("ui/focus"), 2.f);
		const string name = ship->GivenName().empty() ? ship->DisplayModelName() : ship->GivenName();
		FontSet::Get(14).Draw({name, {static_cast<int>(row.Width() - 40), Alignment::LEFT, Truncate::MIDDLE}},
			row.TopLeft() + Point(6, 8), Theme("ui/text primary"));
		if(row.Top() >= viewport.Top() && row.Bottom() <= viewport.Bottom())
		{
			shipZones.emplace_back(row, ship);
			if(ship == player.Flagship())
				FontSet::Get(14).Draw("F", row.TopRight() + Point(-23, 8), Theme("ui/text secondary"));
			else
			{
				const Rectangle park = Rectangle::FromCorner(row.TopRight() + Point(-27, 3), Point(24, 24));
				DrawControl(park, ship->IsParked() ? "P" : "p", ship->IsParked(), [this, ship]() {
					player.ParkShip(ship, !ship->IsParked());
				});
			}
		}
	}
	EndClip();
}


void ModernShipyardPanel::DrawDetailsPane(const Rectangle &bounds)
{
	FillShader::Fill(bounds, Theme("ui/panel"));
	Border(bounds, Theme("ui/divider"));
	const Font &font = FontSet::Get(14);
	if(!selected)
	{
		font.Draw("No ship selected.", bounds.TopLeft() + Point(12, 48), Theme("ui/text secondary"));
		return;
	}
	const double titleInset = compact && detailsPage ? 110. : 12.;
	font.Draw({selected->DisplayModelName(), {static_cast<int>(bounds.Width() - titleInset - 12), Alignment::LEFT, Truncate::MIDDLE}},
		bounds.TopLeft() + Point(titleInset, 10), Theme("ui/text primary"));
	shipInfo.Update(*selected, player, hasFleetCapacity, false, true);
	const Rectangle viewport = Rectangle::FromCorner(bounds.TopLeft() + Point(1, 40),
		Point(bounds.Width() - 2, bounds.Height() - 41));
	detailScroll.SetDisplaySize(viewport.Height());
	detailScroll.SetMaxValue(175. + shipInfo.DescriptionHeight() + shipInfo.AttributesHeight() + shipInfo.OutfitsHeight());
	Clip(viewport);
	if(const Sprite *sprite = selected->Thumbnail().GetSprite(); sprite && sprite->IsLoaded())
		SpriteShader::Draw(sprite, Point(bounds.Center().X(), viewport.Top() + 55 - detailScroll.Value()),
			min(90. / max(sprite->Width(), sprite->Height()), 1.));
	const double contentX = compact ? bounds.Center().X() - 150. : bounds.Left() + 12.;
	double y = viewport.Top() + 112 - detailScroll.Value();
	const int64_t price = player.StockDepreciation().Value(*selected, day);
	font.Draw("Purchase price: " + Format::CreditString(price), Point(contentX, y), Theme("ui/text secondary"));
	y += 25;
	font.Draw("Base price: " + Format::CreditString(selected->Cost()), Point(contentX, y), Theme("ui/text secondary"));
	y += 28;
	shipInfo.DrawDescription(Point(contentX, y));
	y += shipInfo.DescriptionHeight() + 10;
	shipInfo.DrawAttributes(Point(contentX, y));
	y += shipInfo.AttributesHeight();
	shipInfo.DrawOutfits(Point(contentX, y));
	EndClip();
}


void ModernShipyardPanel::DrawActions(const Rectangle &bounds)
{
	FillShader::Fill(bounds, Theme("ui/panel"));
	Border(bounds, Theme("ui/divider"));
	const Font &font = FontSet::Get(14);
	font.Draw("Credits: " + Format::CreditString(player.Accounts().Credits()),
		bounds.TopLeft() + Point(10, 10), Theme("ui/text primary"));
	const TransactionResult buy = CanDoBuyButton();
	const string buyReason = buy.HasMessage() ? buy.Message() : "Select a ship to buy.";
	if(!buy)
		font.Draw({buyReason, {static_cast<int>(bounds.Width() - 20), Alignment::LEFT, Truncate::MIDDLE}},
			bounds.TopLeft() + Point(10, 33), Theme("ui/text secondary"));
	const double buttonWidth = (bounds.Width() - 28) / 3.;
	const double buttonY = bounds.Top() + 63;
	DrawControl(Rectangle::FromCorner(Point(bounds.Left() + 7, buttonY), Point(buttonWidth, 32)),
		"BUY  B", false, [this, buy, buyReason]() {
			if(buy) NativeAction('b');
			else GetUI().Push(DialogPanel::Info(buyReason));
		}, static_cast<bool>(buy));
	DrawControl(Rectangle::FromCorner(Point(bounds.Left() + 14 + buttonWidth, buttonY), Point(buttonWidth, 32)),
		"SELL  S", false, [this]() {
			if(playerShips.empty()) GetUI().Push(DialogPanel::Info("Select an owned ship to sell."));
			else NativeAction('s');
		}, !playerShips.empty());
	DrawControl(Rectangle::FromCorner(Point(bounds.Left() + 21 + buttonWidth * 2, buttonY), Point(buttonWidth, 32)),
		"SELL HULL  U", false, [this]() {
			if(playerShips.empty()) GetUI().Push(DialogPanel::Info("Select an owned ship to sell."));
			else NativeAction('u');
		}, !playerShips.empty());
	font.Draw("Quantity:", bounds.TopLeft() + Point(10, 112), Theme("ui/text secondary"));
	selectedQuantity->SetPosition(Rectangle::FromCorner(bounds.TopLeft() + Point(85, 106), Point(86, 22)));
	font.Draw(playerShips.empty() ? "Select an owned ship to sell." :
		to_string(playerShips.size()) + " owned ship(s) selected.",
		bounds.TopLeft() + Point(10, 139), Theme("ui/text secondary"));
}


void ModernShipyardPanel::NativeAction(SDL_Keycode key)
{
	selectedShip = selected;
	const TransactionResult result = HandleShortcuts(key);
	if(result.HasMessage())
		GetUI().Push(DialogPanel::Info(result.Message()));
}


void ModernShipyardPanel::ReorderSelectedShip(int direction)
{
	if(!playerShip)
		return;
	const auto it = find_if(player.Ships().begin(), player.Ships().end(),
		[this](const shared_ptr<Ship> &ship) { return ship.get() == playerShip; });
	if(it == player.Ships().end())
		return;
	const int index = it - player.Ships().begin();
	const int next = index + direction;
	if(next >= 0 && next < static_cast<int>(player.Ships().size()))
	{
		player.ReorderShip(index, next);
		RefreshFleet();
	}
}


bool ModernShipyardPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(!isNewPress)
		return true;
	if(key == SDLK_ESCAPE && detailsPage)
		detailsPage = false;
	else if(key == SDLK_UP || key == SDLK_DOWN)
	{
		if(mod & (KMOD_CTRL | KMOD_GUI))
			ReorderSelectedShip(key == SDLK_UP ? -1 : 1);
		else if(compact && detailsPage)
			detailScroll.Scroll(key == SDLK_UP ? -40. : 40., 0);
		else
		{
			auto it = find(rows.begin(), rows.end(), selected);
			SelectRow((it == rows.end() ? 0 : it - rows.begin()) + (key == SDLK_UP ? -1 : 1));
		}
	}
	else if(key == SDLK_LEFT || key == SDLK_RIGHT)
	{
		auto it = find(categoryNames.begin(), categoryNames.end(), category);
		const int next = (it == categoryNames.end() ? 0 : it - categoryNames.begin()) + (key == SDLK_LEFT ? -1 : 1);
		if(next >= 0 && next < static_cast<int>(categoryNames.size()))
		{
			category = categoryNames[next];
			rowScroll.Set(0., 0);
			Refresh();
		}
	}
	else if(key == SDLK_PAGEUP || key == SDLK_PAGEDOWN || key == SDLK_HOME || key == SDLK_END)
	{
		ScrollVar<double> *scroll = hoverPane == Pane::CATEGORIES ? &categoryScroll :
			(hoverPane == Pane::FLEET ? &fleetScroll :
				(hoverPane == Pane::DETAILS ? &detailScroll : &rowScroll));
		if(key == SDLK_HOME || key == SDLK_END)
			scroll->Set(key == SDLK_HOME ? 0. : scroll->MaxValue(), 0);
		else
			scroll->Scroll((key == SDLK_PAGEUP ? -1. : 1.) * scroll->DisplaySize() * .85, 0);
	}
	else if(key == SDLK_v)
	{
		sort = static_cast<Sort>((static_cast<int>(sort) + 1) % 3);
		Refresh();
	}
	else if(key == SDLK_TAB || key == SDLK_SLASH || key == SDLK_f)
		search->SetFocus(true);
	else if(key == SDLK_RETURN && compact && selected && !detailsPage)
		detailsPage = true;
	else if(key == SDLK_b || key == SDLK_s || key == SDLK_u || key == SDLK_r)
		NativeAction(key);
	else if(key == SDLK_ESCAPE || key == 'l' || key == 'd' || key == 'k'
			|| (key == 'p' && (mod & KMOD_SHIFT)) || (key >= '0' && key <= '9')
			|| command.Has(Command::HELP) || command.Has(Command::MAP)
			|| (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
		return ShopPanel::KeyDown(key, mod, command, isNewPress);
	return true;
}


bool ModernShipyardPanel::Click(int x, int y, MouseButton button, int clicks)
{
	return button == MouseButton::LEFT && fleetBounds.Contains(Point(x, y))
		? ShopPanel::Click(x, y, button, clicks) : false;
}


bool ModernShipyardPanel::Drag(double dx, double dy)
{
	return dragShip ? ShopPanel::Drag(dx, dy) : false;
}


bool ModernShipyardPanel::Release(int x, int y, MouseButton button)
{
	return ShopPanel::Release(x, y, button);
}


bool ModernShipyardPanel::Scroll(double, double dy)
{
	ScrollVar<double> *scroll = hoverPane == Pane::CATEGORIES ? &categoryScroll :
		(hoverPane == Pane::FLEET ? &fleetScroll :
			(hoverPane == Pane::DETAILS || (compact && detailsPage) ? &detailScroll : &rowScroll));
	scroll->Scroll(-dy * 30., 0);
	return true;
}


bool ModernShipyardPanel::Hover(int x, int y)
{
	const Point point(x, y);
	hoverPane = categoryBounds.Contains(point) ? Pane::CATEGORIES :
		(fleetBounds.Contains(point) ? Pane::FLEET :
			(detailBounds.Contains(point) && (!compact || detailsPage) ? Pane::DETAILS : Pane::CATALOG));
	return true;
}


void ModernShipyardPanel::EndEditing()
{
	search->SetFocus(false);
}
