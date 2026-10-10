/* ModernOutfitterPanel.cpp
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

#include "ModernOutfitterPanel.h"
#include "ModernShopStyle.h"

#include "CategoryList.h"
#include "CategoryType.h"
#include "Color.h"
#include "Command.h"
#include "DialogPanel.h"
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
#include "System.h"
#include "image/Sprite.h"
#include "shader/SpriteShader.h"
#include "text/Truncate.h"
#include "text/DisplayText.h"
#include "text/Alignment.h"
#include "text/WrappedText.h"
#include "UI.h"
#include "Weapon.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <set>
#include <utility>

using namespace std;
using namespace ModernShopStyle;

namespace {
	using PartStat = pair<string, string>;

	vector<PartStat> SelectedPartStats(const Outfit &outfit)
	{
		vector<PartStat> stats;
		set<string> shown;
		if(outfit.Mass())
			stats.emplace_back("Mass", Format::Number(outfit.Mass()) + " t");
		shown.insert("mass");
		auto Add = [&](const char *attribute, const char *label)
		{
			shown.insert(attribute);
			const double value = outfit.Get(attribute);
			if(value)
				stats.emplace_back(label, OutfitInfoDisplay::FormatAttribute(attribute, value));
		};
		Add("outfit space", "Outfit space (t)");
		Add("weapon capacity", "Weapon space (t)");
		Add("engine capacity", "Engine space (t)");
		Add("gun ports", "Gun ports");
		Add("turret mounts", "Turret mounts");
		if(const Weapon *weapon = outfit.GetWeapon().get())
		{
			if(weapon->Ammo())
				stats.emplace_back("Ammunition", weapon->Ammo()->DisplayName());
			if(weapon->AmmoUsage())
				stats.emplace_back("Ammo per shot", Format::Number(weapon->AmmoUsage()));
			if(weapon->Range())
				stats.emplace_back("Range", Format::Number(weapon->Range()));
			if(weapon->ShieldDamage())
				stats.emplace_back("Shield dmg/shot", Format::Number(weapon->ShieldDamage()));
			if(weapon->HullDamage())
				stats.emplace_back("Hull dmg/shot", Format::Number(weapon->HullDamage()));
		}
		Add("thrust", "Thrust");
		Add("reverse thrust", "Reverse thrust");
		Add("turn", "Turning");
		Add("thrusting energy", "Thrust energy/s");
		Add("thrusting heat", "Thrust heat/s");
		Add("turning energy", "Turn energy/s");
		Add("turning heat", "Turn heat/s");
		Add("energy generation", "Energy gen/s");
		Add("solar collection", "Solar collect/s");
		Add("energy consumption", "Energy use/s");
		Add("energy capacity", "Energy capacity");
		Add("heat generation", "Heat gen/s");
		Add("cooling", "Cooling/s");
		Add("shield generation", "Shield gen/s");
		Add("hull repair rate", "Hull repair/s");
		Add("fuel generation", "Fuel gen/s");
		Add("fuel consumption", "Fuel use/s");
		Add("cargo space", "Cargo space (t)");
		Add("bunks", "Bunks");
		Add("required crew", "Required crew");
		for(const auto &[name, value] : outfit)
			if(value && !shown.contains(name) && stats.size() < 14)
				stats.emplace_back(name, OutfitInfoDisplay::FormatAttribute(name, value));
		if(stats.empty())
			stats.emplace_back("Type", outfit.TrueName().ends_with(" License") ? "License" : "Special item");
		return stats;
	}

	double AttributeValue(const map<string, double> &attributes, const string &name)
	{
		auto it = attributes.find(name);
		return it == attributes.end() ? 0. : it->second;
	}

	const char *RouteName(OutfitterPanel::OutfitLocation location)
	{
		switch(location)
		{
			case OutfitterPanel::OutfitLocation::Ship: return "Installed";
			case OutfitterPanel::OutfitLocation::Shop: return "Shop";
			case OutfitterPanel::OutfitLocation::Cargo: return "Cargo";
			case OutfitterPanel::OutfitLocation::Storage: return "Storage";
		}
		return "Unknown";
	}

	optional<OutfitterPanel::OutfitLocation> ParseRouteLocation(const string &name)
	{
		if(name == "shop")
			return OutfitterPanel::OutfitLocation::Shop;
		if(name == "ship")
			return OutfitterPanel::OutfitLocation::Ship;
		if(name == "cargo")
			return OutfitterPanel::OutfitLocation::Cargo;
		if(name == "storage")
			return OutfitterPanel::OutfitLocation::Storage;
		return nullopt;
	}

	bool ShowItemEffect(const OutfitterPanel::TransferPlan &plan,
		const OutfitterPanel::ItemEffect &item)
	{
		return item.outfit == plan.outfit || item.cargoBefore != item.cargoAfter
			|| item.storageBefore != item.storageAfter || item.stockBefore != item.stockAfter;
	}
}


ModernOutfitterPanel::ModernOutfitterPanel(PlayerInfo &player, const Sale<Outfit> &stock)
	: OutfitterPanel(player, stock), stock(stock), search(make_shared<SearchEdit>())
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

	RebuildCatalog();
}



bool ModernOutfitterPanel::SelectOutfitForTest(const string &name, int quantity, bool selectAllShips)
{
	if(!OutfitterPanel::SelectOutfitForTest(name, quantity, selectAllShips))
		return false;
	selected = GameData::Outfits().Find(name);
	category = selected->Category();
	allShips = selectAllShips;
	Refresh();
	return true;
}



bool ModernOutfitterPanel::SelectOutfitRouteForTest(const string &from, const string &to)
{
	const auto source = ParseRouteLocation(from);
	const auto destination = ParseRouteLocation(to);
	if(!source || !destination || source == destination)
		return false;
	actionFrom = *source;
	actionTo = *destination;
	previewDirty = true;
	UpdatePreview();
	return true;
}


void ModernOutfitterPanel::Step()
{
	OutfitterPanel::Step();
	const bool top = GetUI().IsTop(this);
	if(top && !wasTop)
		RebuildCatalog();
	wasTop = top;
	if(customShipSelection && customShips != playerShips)
	{
		customShips = playerShips;
		previewDirty = true;
	}
	RefreshShips();
	if(queryDirty)
	{
		queryDirty = false;
		Refresh();
	}
	if(lastQuantity != selectedQuantity->Text())
	{
		lastQuantity = selectedQuantity->Text();
		previewDirty = true;
	}
}



vector<const Ship *> ModernOutfitterPanel::SelectedShipsForView() const
{
	vector<const Ship *> selectedShips;
	if(customShipSelection)
	{
		for(const Ship *ship : customShips)
			selectedShips.push_back(ship);
	}
	else if(allShips)
		selectedShips = ships;
	else if(!ships.empty())
		selectedShips.push_back(ships[shipIndex]);
	return selectedShips;
}



void ModernOutfitterPanel::RefreshShips()
{
	vector<const Ship *> here;
	for(const auto &ship : player.Ships())
		if(ship && ship->GetPlanet() == player.GetPlanet())
			here.push_back(ship.get());
	if(here == ships)
		return;
	const Ship *previous = ships.empty() ? nullptr : ships[shipIndex];
	ships = std::move(here);
	auto it = find(ships.begin(), ships.end(), previous);
	shipIndex = it == ships.end() ? 0 : it - ships.begin();
	if(customShipSelection)
	{
		set<Ship *> available;
		for(const Ship *ship : ships)
			available.insert(const_cast<Ship *>(ship));
		for(auto selected = customShips.begin(); selected != customShips.end(); )
			if(!available.contains(*selected))
				selected = customShips.erase(selected);
			else
				++selected;
	}
	previewDirty = true;
	RebuildCatalog();
}



void ModernOutfitterPanel::RebuildCatalog()
{
	all.clear();
	categories.clear();
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
	if(find(categories.begin(), categories.end(), category) == categories.end())
		category = categories.empty() ? "" : categories.front();
	Refresh();
}


int ModernOutfitterPanel::Installed(const Outfit *outfit) const
{
	int total = 0;
	for(const Ship *ship : SelectedShipsForView())
		total += ship->OutfitCount(outfit);
	return total;
}


int ModernOutfitterPanel::StorageCount(const Outfit *outfit) const
{
	const auto &storage = player.PlanetaryStorage();
	const auto it = storage.find(player.GetPlanet());
	return it == storage.end() ? 0 : it->second.Get(outfit);
}


bool ModernOutfitterPanel::OwnedLicense(const Outfit *outfit) const
{
	static const string suffix = " License";
	const string &name = outfit->TrueName();
	return name.ends_with(suffix) && player.HasLicense(name.substr(0, name.size() - suffix.size()));
}


bool ModernOutfitterPanel::VisibleInSource(const Outfit *outfit) const
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


void ModernOutfitterPanel::Refresh()
{
	previewDirty = true;
	const Outfit *previous = selected;
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
	if(selected != previous)
		lastResult.reset();
	rowScroll.SetMaxValue(rows.size() * 52.);
	detailScroll.Set(0, 0);
}


void ModernOutfitterPanel::SelectRow(int index)
{
	if(index < 0 || index >= static_cast<int>(rows.size()))
		return;
	if(selected != rows[index])
		lastResult.reset();
	selected = rows[index];
	previewDirty = true;
	detailScroll.Set(0, 0);
	const double top = index * 52.;
	if(top < rowScroll.Value())
		rowScroll.Set(top, 0);
	else if(top + 52 > rowScroll.Value() + rowScroll.DisplaySize())
		rowScroll.Set(top + 52 - rowScroll.DisplaySize(), 0);
}


void ModernOutfitterPanel::CycleShip()
{
	if(ships.empty())
		return;
	shipIndex = (shipIndex + 1) % ships.size();
	allShips = false;
	customShipSelection = false;
	customShips.clear();
	previewDirty = true;
	Refresh();
}


void ModernOutfitterPanel::DrawControl(const Rectangle &bounds, const string &text, bool selected,
	const function<void()> &action)
{
	FillShader::Fill(bounds, Theme(selected ? "ui/selected" : "ui/raised"));
	if(selected)
		FillShader::Fill(Rectangle::FromCorner(bounds.TopLeft(), Point(3, bounds.Height())), Theme("ui/focus"));
	const Font &font = FontSet::Get(14);
	const int inset = text == "INSTALLED" || text == "TO INSTALLED" ? 2 : 6;
	font.Draw({text, {static_cast<int>(bounds.Width() - 2 * inset), Alignment::CENTER, Truncate::MIDDLE}},
		bounds.TopLeft() + Point(inset, 8), Theme("ui/text primary"));
	AddZone(bounds, action);
}



void ModernOutfitterPanel::SyncNativeSelection()
{
	selectedOutfit = selected;
	if(customShipSelection)
	{
		playerShips = customShips;
		if(!playerShips.contains(playerShip))
			playerShip = playerShips.empty() ? nullptr : *playerShips.begin();
		return;
	}
	playerShips.clear();
	playerShip = nullptr;
	if(ships.empty())
		return;
	playerShip = const_cast<Ship *>(ships[shipIndex]);
	if(allShips)
		for(const Ship *ship : ships)
			playerShips.insert(const_cast<Ship *>(ship));
	else
		playerShips.insert(playerShip);
}



void ModernOutfitterPanel::UpdatePreview()
{
	if(!previewDirty)
		return;
	SyncNativeSelection();
	preview = PreviewMoveOutfit(actionFrom, actionTo, "transfer");
	previewDirty = false;
}



void ModernOutfitterPanel::CommitSelectedRoute()
{
	UpdatePreview();
	if(!preview || (!preview->success && !preview->hasEffects))
	{
		feedback = preview ? preview->reason : "No transfer is available.";
		GetUI().Push(DialogPanel::Info(feedback));
		return;
	}
	CommitResult result = CommitMoveOutfit(*preview, "transfer");
	if(result.stale)
	{
		lastResult.reset();
		preview = result.plan;
		feedback = "State changed. Review the updated transfer before confirming again.";
		return;
	}
	if(!result.committed || !result.matched)
	{
		feedback = "The native result differed from the preview. Inspect the transfer state.";
		GetUI().Push(DialogPanel::Info(feedback));
		previewDirty = true;
		return;
	}
	lastResult = result.plan;
	const int64_t requested = result.plan.quantityPerShip
		? static_cast<int64_t>(result.plan.requestedQuantity) * result.plan.selectedShipCount
		: result.plan.requestedQuantity;
	feedback = string(result.plan.fulfilledQuantity < requested ? "Partial: moved " : "Moved ")
		+ to_string(result.plan.fulfilledQuantity) + " of " + to_string(requested) + " outfit(s); credits "
		+ to_string(result.plan.creditsAfter - result.plan.creditsBefore) + ".";
	RebuildCatalog();
	UpdatePreview();
}



void ModernOutfitterPanel::HandleNativeShortcut(SDL_Keycode key)
{
	SyncNativeSelection();
	const TransferPlan plan = PreviewShortcut(key);
	CommitResult result = CommitMoveOutfit(plan, "native shortcut");
	if(result.stale)
	{
		lastResult.reset();
		feedback = "State changed. Review the updated native action before trying again.";
		previewDirty = true;
	}
	else if(!result.committed)
	{
		lastResult.reset();
		feedback = result.plan.reason.empty() ? "No outfits moved." : result.plan.reason;
		if(!result.plan.reason.empty())
			GetUI().Push(DialogPanel::Info(result.plan.reason));
	}
	else if(!result.matched)
	{
		lastResult.reset();
		feedback = "The native shortcut result differed from its authoritative preview.";
		GetUI().Push(DialogPanel::Info(feedback));
	}
	else
	{
		lastResult = result.plan;
		const int64_t requested = result.plan.quantityPerShip
			? static_cast<int64_t>(result.plan.requestedQuantity) * result.plan.selectedShipCount
			: result.plan.requestedQuantity;
		string route = "Native ";
		route += static_cast<char>(toupper(key));
		route += ": " + string(RouteName(result.plan.from)) + " -> " + RouteName(result.plan.to);
		if(result.plan.from == OutfitLocation::Ship && result.plan.to == OutfitLocation::Cargo)
			for(const ItemEffect &item : result.plan.items)
				if(item.outfit == selected && item.storageAfter > item.storageBefore)
					route += " (overflow to Storage)";
		feedback = route + "; moved " + to_string(result.plan.fulfilledQuantity) + " of "
			+ to_string(requested) + "; credits "
			+ to_string(result.plan.creditsAfter - result.plan.creditsBefore) + ".";
	}
	RebuildCatalog();
	UpdatePreview();
}



void ModernOutfitterPanel::ReorderSelectedShip(int direction)
{
	if(!playerShip)
		return;
	const auto &fleet = player.Ships();
	auto current = find_if(fleet.begin(), fleet.end(), [this](const shared_ptr<Ship> &ship) {
		return ship.get() == playerShip;
	});
	if(current == fleet.end())
		return;
	const int from = current - fleet.begin();
	const int to = from + direction;
	if(to < 0 || to >= static_cast<int>(fleet.size()))
		return;
	player.ReorderShip(from, to);
	RefreshShips();
	previewDirty = true;
}



void ModernOutfitterPanel::DrawActions(const Rectangle &bounds)
{
	FillShader::Fill(bounds, Theme("ui/raised"));
	Border(bounds, Theme("ui/divider"));
	const double x = bounds.Left() + 6;
	const double width = bounds.Width() - 12;
	const double rowWidth = (width - 12) / 4.;
	const pair<OutfitLocation, const char *> sources[] = {
		{OutfitLocation::Shop, "SHOP"}, {OutfitLocation::Ship, "SHIP"},
		{OutfitLocation::Cargo, "CARGO"}, {OutfitLocation::Storage, "STORE"}
	};
	for(int i = 0; i < 4; ++i)
	{
		const OutfitLocation location = sources[i].first;
		DrawControl(Rectangle::FromCorner(Point(x + i * (rowWidth + 4), bounds.Top() + 5),
			Point(rowWidth, 27)), sources[i].second, actionFrom == location,
			[this, location]() {
				actionFrom = location;
				if(actionTo == actionFrom)
					actionTo = actionFrom == OutfitLocation::Ship ? OutfitLocation::Shop : OutfitLocation::Ship;
				previewDirty = true;
				UpdatePreview();
			});
	}
	int destination = 0;
	const double destinationWidth = (width - 8) / 3.;
	for(const auto &[location, name] : sources)
	{
		if(location == actionFrom)
			continue;
		const int index = destination++;
		DrawControl(Rectangle::FromCorner(Point(x + index * (destinationWidth + 4), bounds.Top() + 37),
			Point(destinationWidth, 27)), string("TO ") + name, actionTo == location,
			[this, location]() { actionTo = location; previewDirty = true; UpdatePreview(); });
	}
	const Font &font = FontSet::Get(14);
	font.Draw("QUANTITY", Point(x, bounds.Top() + 77), Theme("ui/text secondary"));
	selectedQuantity->SetPosition(Rectangle::FromCorner(Point(x + 76, bounds.Top() + 70), Point(76, 28)));
	const Rectangle transfer = Rectangle::FromCorner(Point(bounds.Right() - 118, bounds.Top() + 70), Point(112, 28));
	const bool canTransfer = preview && (preview->success || preview->hasEffects);
	FillShader::Fill(transfer, Theme(canTransfer ? "ui/focus" : "ui/panel"));
	font.Draw("TRANSFER", transfer.TopLeft() + Point(19, 8),
		Theme(canTransfer ? "ui/background" : "ui/text muted"));
	AddZone(transfer, [this]() { CommitSelectedRoute(); });
	if(preview)
	{
		const int64_t requested = preview->quantityPerShip
			? static_cast<int64_t>(preview->requestedQuantity) * preview->selectedShipCount
			: preview->requestedQuantity;
		const string meaning = preview->quantityPerShip ? " per selected ship" : " total";
		font.Draw({"Requested " + to_string(preview->requestedQuantity) + meaning + " (up to "
			+ to_string(requested) + ")", {static_cast<int>(width), Alignment::LEFT, Truncate::BACK}},
			Point(x, bounds.Top() + 105), Theme("ui/text secondary"));
		const string outcome = (preview->success || preview->hasEffects)
			? "Preview: " + to_string(preview->fulfilledQuantity)
			+ " fulfilled; credits " + to_string(preview->creditsAfter - preview->creditsBefore)
			: "Unavailable: " + preview->reason;
		WrappedText status(font);
		status.SetAlignment(Alignment::LEFT);
		status.SetWrapWidth(static_cast<int>(width));
		status.Wrap(outcome);
		status.Draw(Point(x, bounds.Top() + 126), Theme(preview->success || preview->hasEffects
			? "ui/text primary" : "ui/caution"));
		string shipsLine;
		if(preview->quantityPerShip)
		{
			int affected = 0;
			for(const ShipEffect &effect : preview->ships)
				affected += effect.outfitsBefore != effect.outfitsAfter || effect.crewBefore != effect.crewAfter;
			shipsLine = "Ships: " + to_string(preview->selectedShipCount) + " selected, "
				+ to_string(preview->eligibleShipCount) + " eligible, " + to_string(affected) + " affected";
		}
		else
			shipsLine = selected && (selected->Get("map") || selected->TrueName().ends_with(" License"))
				? "Permanent purchase: one item" : "Hold quantity is a total";
		if(status.Height() <= 32 && feedback.empty())
			font.Draw({shipsLine, {static_cast<int>(width), Alignment::LEFT, Truncate::BACK}},
				Point(x, bounds.Top() + 166), Theme("ui/text secondary"));
	}
	if(!feedback.empty() && (!preview || preview->success))
		font.Draw({feedback, {static_cast<int>(width), Alignment::LEFT, Truncate::BACK}},
			Point(x, bounds.Top() + 174), Theme("ui/text primary"));
}


void ModernOutfitterPanel::Draw()
{
	FillShader::Fill(Rectangle(Point(), Screen::Dimensions()), Theme("ui/background"));
	ClearZones();
	shipZones.clear();
	const int modifier = Modifier();
	if(modifier > 1)
	{
		selectedQuantity->SetText(to_string(modifier));
		quantityIsModifier = true;
		previewDirty = true;
	}
	else if(quantityIsModifier)
	{
		selectedQuantity->SetText("1");
		quantityIsModifier = false;
		previewDirty = true;
	}
	const double left = Screen::Left() + 12;
	const double right = Screen::Right() - 12;
	const double top = Screen::Top() + 10;
	const double bottom = Screen::Bottom() - 12;
	const double width = right - left;
	compact = width < 1040;
	const Font &font = FontSet::Get(14);
	FillShader::Fill(Rectangle::FromCorner(Point(Screen::Left(), Screen::Top()),
		Point(Screen::Width(), 50)), Theme("ui/panel"));
	FillShader::Fill(Rectangle::FromCorner(Point(Screen::Left(), Screen::Top() + 49),
		Point(Screen::Width(), 1)), Theme("ui/divider"));
	FontSet::Get(18).Draw("OUTFITTER", Point(left + 4, top + 4), Theme("ui/text primary"));
	font.Draw("PORT  /  OUTFITTER", Point(left + 184, top + 8), Theme("ui/text secondary"));
	DrawControl(Rectangle::FromCorner(Point(right - 104, top), Point(104, 30)), "BACK  ESC", false,
		[this]() { GetUI().Pop(this); });
	const double sideWidth = compact ? 154. : 172.;
	const double rightWidth = compact ? 0. : min(360., max(310., width * .27));
	const double centerLeft = left + sideWidth + 8;
	const double centerWidth = width - sideWidth - rightWidth - (compact ? 8 : 24);
	const double rightLeft = centerLeft + centerWidth + 8;
	const double controlTop = top + 50;
	font.Draw("EQUIPMENT", Point(left + 8, controlTop + 10), Theme("ui/text secondary"));
	search->SetPosition(Rectangle::FromCorner(Point(centerLeft, controlTop), Point(196, 34)));
	const double tabWidth = max(54., (centerWidth - 208 - 16) / 5.);
	const char *sources[] = {"ALL", "SHOP", "INSTALLED", "CARGO", "STORAGE"};
	for(int i = 0; i < 5; ++i)
		DrawControl(Rectangle::FromCorner(Point(centerLeft + 208 + i * (tabWidth + 4), controlTop), Point(tabWidth, 34)),
			sources[i], source == static_cast<Source>(i), [this, i]() { source = static_cast<Source>(i); Refresh(); });
	DrawControl(Rectangle::FromCorner(Point(compact ? centerLeft : rightLeft, controlTop + (compact ? 42 : 0)), Point(112, 34)),
		sort == Sort::NAME ? "NAME SORT" : "PRICE SORT", false,
		[this]() { sort = sort == Sort::NAME ? Sort::PRICE : Sort::NAME; Refresh(); });
	const vector<const Ship *> viewShips = SelectedShipsForView();
	const string shipLabel = viewShips.empty() ? "NO SHIP SELECTED" : (viewShips.size() > 1
		? to_string(viewShips.size()) + " SHIPS SELECTED"
		: (viewShips.front()->GivenName().empty() ? viewShips.front()->DisplayModelName()
			: viewShips.front()->GivenName()));
	if(compact)
		DrawControl(Rectangle::FromCorner(Point(right - 195, controlTop + 42), Point(195, 28)),
			"SHIP: " + shipLabel, false, [this]() { CycleShip(); });
	else
		font.Draw({"Balance  " + Format::CreditString(player.Accounts().Credits()),
			{static_cast<int>(rightWidth - 124), Alignment::RIGHT, Truncate::MIDDLE}},
			Point(rightLeft + 120, controlTop + 9), Theme("ui/text primary"));
	const double contentTop = top + (compact ? 130 : 96);
	const double contentHeight = max(80., bottom - contentTop);
	const double catWidth = sideWidth;
	const double fleetHeight = min(contentHeight * .35, max(128., ships.size() * 32. + 42.));
	categoryBounds = Rectangle::FromCorner(Point(left, contentTop),
		Point(catWidth, contentHeight - fleetHeight - 8));
	fleetBounds = Rectangle::FromCorner(Point(left, categoryBounds.Bottom() + 8), Point(catWidth, fleetHeight));
	rowBounds = Rectangle::FromCorner(Point(centerLeft, contentTop),
		Point(centerWidth, compact ? contentHeight : min(contentHeight * .50,
			max(220., rows.size() * 52. + 64.))));
	const double footerHeight = 216.;
	detailBounds = compact
		? Rectangle::FromCorner(rowBounds.TopLeft(), Point(rowBounds.Width(), contentHeight - footerHeight))
		: Rectangle::FromCorner(Point(centerLeft, rowBounds.Bottom() + 8),
			Point(centerWidth, bottom - rowBounds.Bottom() - 8));
	DrawCategories(categoryBounds);
	DrawFleet(fleetBounds);
	if(compact && !detailsPage)
		selectedQuantity->SetPosition(Rectangle(Point(Screen::Right() + 100, Screen::Bottom() + 100), Point()));
	if(compact && detailsPage)
	{
		UpdatePreview();
		DrawDetailsPane(detailBounds);
		DrawActions(Rectangle::FromCorner(Point(detailBounds.Left(), detailBounds.Bottom()),
			Point(detailBounds.Width(), footerHeight)));
		DrawControl(Rectangle::FromCorner(Point(rowBounds.Left() + 8, rowBounds.Top() + 8), Point(88, 28)),
			"BACK", false, [this]() { detailsPage = false; });
	}
	else
	{
		DrawCatalog(rowBounds);
		if(!compact)
		{
			UpdatePreview();
			DrawDetailsPane(detailBounds);
			const double shipHeight = min(360., 230. + max(0., contentHeight - 582.) * .7);
			const double actionHeight = 196.;
			DrawShipContext(Rectangle::FromCorner(Point(rightLeft, contentTop),
				Point(rightWidth, shipHeight)));
			DrawPreviewComparison(Rectangle::FromCorner(Point(rightLeft, contentTop + shipHeight + 8),
				Point(rightWidth, contentHeight - shipHeight - actionHeight - 16)));
			DrawActions(Rectangle::FromCorner(Point(rightLeft, bottom - actionHeight),
				Point(rightWidth, actionHeight)));
		}
	}
}


void ModernOutfitterPanel::DrawShipContext(const Rectangle &bounds)
{
	FillShader::Fill(bounds, Theme("ui/panel"));
	const Font &font = FontSet::Get(14);
	const vector<const Ship *> selectedShips = SelectedShipsForView();
	if(selectedShips.empty())
	{
		font.Draw("CURRENT SHIP", bounds.TopLeft() + Point(12, 10), Theme("ui/text secondary"));
		font.Draw("Select a ship in the fleet list.", bounds.TopLeft() + Point(12, 50), Theme("ui/text primary"));
		return;
	}
	const Ship *ship = selectedShips.front();
	const Outfit &attributes = ship->Attributes();
	const string name = ship->GivenName().empty() ? ship->DisplayModelName() : ship->GivenName();
	const bool roomy = bounds.Height() > 270.;
	const double identityX = roomy ? 148. : 103.;
	const double sectionY = roomy ? 153. : 98.;
	if(const Sprite *sprite = ship->Thumbnail().GetSprite(); sprite && sprite->IsLoaded())
		SpriteShader::Draw(sprite, bounds.TopLeft() + Point(roomy ? 75. : 51., roomy ? 80. : 54.),
			min((roomy ? 120. : 74.) / max(sprite->Width(), sprite->Height()), roomy ? 2. : 1.5));
	font.Draw("CURRENT SHIP", bounds.TopLeft() + Point(identityX, roomy ? 20. : 8.), Theme("ui/text secondary"));
	FontSet::Get(18).Draw({name, {static_cast<int>(bounds.Width() - identityX - 12.), Alignment::LEFT, Truncate::MIDDLE}},
		bounds.TopLeft() + Point(identityX, roomy ? 42. : 28.), Theme("ui/text primary"));
	font.Draw({ship->DisplayModelName(), {static_cast<int>(bounds.Width() - identityX - 12.), Alignment::LEFT, Truncate::MIDDLE}},
		bounds.TopLeft() + Point(identityX, roomy ? 69. : 53.), Theme("ui/text secondary"));
	if(ships.size() > 1)
		DrawControl(Rectangle::FromCorner(bounds.TopLeft() + Point(identityX, roomy ? 100. : 72.),
			Point(bounds.Width() - identityX - 12., 24)),
			"NEXT SHIP", false, [this]() { CycleShip(); });
	FillShader::Fill(Rectangle::FromCorner(bounds.TopLeft() + Point(12, sectionY),
		Point(bounds.Width() - 24, 1)), Theme("ui/divider"));
	const double columnWidth = (bounds.Width() - 36.) / 2.;
	const double leftX = bounds.Left() + 12.;
	const double rightX = leftX + columnWidth + 12.;
	font.Draw("CAPACITY", Point(leftX, bounds.Top() + sectionY + 3.), Theme("ui/text secondary"));
	font.Draw("SHIP / PERFORMANCE", Point(rightX, bounds.Top() + sectionY + 3.), Theme("ui/text secondary"));
	auto Row = [&](double x, int index, const string &label, const string &value)
	{
		const double y = bounds.Top() + sectionY + (roomy ? 24. + 19. * index : 21. + 15. * index);
		font.Draw({label, {64, Alignment::LEFT, Truncate::MIDDLE}}, Point(x, y), Theme("ui/text secondary"));
		font.Draw({value, {static_cast<int>(columnWidth - 66.), Alignment::RIGHT, Truncate::MIDDLE}},
			Point(x + 65., y), Theme("ui/text primary"));
	};
	Row(leftX, 0, "Outfit", Format::Number(attributes.Get("outfit space")));
	Row(leftX, 1, "Weapon", Format::Number(attributes.Get("weapon capacity")));
	Row(leftX, 2, "Engine", Format::Number(attributes.Get("engine capacity")));
	Row(leftX, 3, "Ports", Format::Number(attributes.Get("gun ports")) + "/" +
		Format::Number(attributes.Get("turret mounts")));
	Row(leftX, 4, "Cargo", Format::Number(ship->Cargo().Used()) + "/" +
		Format::Number(attributes.Get("cargo space")));
	Row(leftX, 5, "Crew", Format::Number(ship->Crew()) + "/" + Format::Number(attributes.Get("bunks")));
	Row(leftX, 6, "Mass", Format::Number(ship->Mass()));
	Row(rightX, 0, "Shields", Format::Number(ship->ShieldLevel()));
	Row(rightX, 1, "Hull", Format::Number(ship->HullLevel()));
	Row(rightX, 2, "Fuel", Format::Number(ship->FuelLevel()) + "/" + Format::Number(ship->MaxFuel()));
	Row(rightX, 3, "Energy", Format::Number(ship->EnergyLevel()));
	Row(rightX, 4, "Heat gen", Format::Number(60. * attributes.Get("heat generation")) + "/s");
	Row(rightX, 5, "Accel", Format::Number(3600. * ship->Acceleration()));
	Row(rightX, 6, "Turn", Format::Number(60. * ship->TurnRate()));
}


void ModernOutfitterPanel::DrawPreviewComparison(const Rectangle &bounds)
{
	FillShader::Fill(bounds, Theme("ui/panel"));
	const Font &font = FontSet::Get(14);
	font.Draw("CURRENT  ->  AFTER", bounds.TopLeft() + Point(12, 8), Theme("ui/text primary"));
	if(!preview || preview->ships.empty())
	{
		font.Draw("Select a ship and route to preview changes.", bounds.TopLeft() + Point(12, 48),
			Theme("ui/text secondary"));
		return;
	}
	const vector<const Ship *> selectedShips = SelectedShipsForView();
	const Ship *ship = selectedShips.empty() ? nullptr : selectedShips.front();
	const ShipEffect *effect = nullptr;
	for(const ShipEffect &candidate : preview->ships)
		if(ship && candidate.fleetIndex < player.Ships().size()
				&& player.Ships()[candidate.fleetIndex].get() == ship)
		{
			effect = &candidate;
			break;
		}
	if(!effect)
	{
		font.Draw("Selected ship has no projected change.", bounds.TopLeft() + Point(12, 48),
			Theme("ui/text secondary"));
		return;
	}
	struct Row {
		string label;
		string before;
		string after;
		const char *color;
	};
	vector<Row> rows;
	set<string> displayedAttributes = {"outfit space", "weapon capacity", "engine capacity", "cargo space", "mass"};
	auto Add = [&](const string &label, double before, double after, const char *color = "ui/text primary")
	{
		if(before != after)
			rows.push_back({label, Format::Number(before), Format::Number(after), color});
	};
	auto AddAttribute = [&](const char *name, const char *label, bool increaseIsGood = false,
		bool increaseIsCaution = false)
	{
		displayedAttributes.insert(name);
		const double before = AttributeValue(effect->attributesBefore, name);
		const double after = AttributeValue(effect->attributesAfter, name);
		if(before != after)
			rows.push_back({label, OutfitInfoDisplay::FormatAttribute(name, before),
				OutfitInfoDisplay::FormatAttribute(name, after),
				increaseIsGood && after > before ? "ui/positive" :
					(increaseIsCaution && after > before ? "ui/caution" : "ui/text primary")});
	};
	Add("Outfit t", effect->outfitSpaceBefore, effect->outfitSpaceAfter,
		effect->outfitSpaceAfter > effect->outfitSpaceBefore ? "ui/positive" : "ui/text primary");
	Add("Weapon t", effect->weaponSpaceBefore, effect->weaponSpaceAfter,
		effect->weaponSpaceAfter > effect->weaponSpaceBefore ? "ui/positive" : "ui/text primary");
	Add("Engine t", effect->engineSpaceBefore, effect->engineSpaceAfter,
		effect->engineSpaceAfter > effect->engineSpaceBefore ? "ui/positive" : "ui/text primary");
	Add("Cargo t", effect->cargoSpaceBefore, effect->cargoSpaceAfter,
		effect->cargoSpaceAfter > effect->cargoSpaceBefore ? "ui/positive" : "ui/text primary");
	Add("Mass t", effect->massBefore, effect->massAfter);
	if(effect->accelerationBefore != effect->accelerationAfter)
		rows.push_back({"Accel", Format::Number(3600. * effect->accelerationBefore),
			Format::Number(3600. * effect->accelerationAfter), "ui/text primary"});
	if(effect->turnRateBefore != effect->turnRateAfter)
		rows.push_back({"Turn rate", Format::Number(60. * effect->turnRateBefore),
			Format::Number(60. * effect->turnRateAfter), "ui/text primary"});
	AddAttribute("thrust", "Thrust", true);
	AddAttribute("reverse thrust", "Reverse", true);
	AddAttribute("turn", "Turning", true);
	AddAttribute("thrusting energy", "Thrust energy/s", false, true);
	AddAttribute("thrusting heat", "Thrust heat/s", false, true);
	AddAttribute("turning energy", "Turn energy/s", false, true);
	AddAttribute("turning heat", "Turn heat/s", false, true);
	AddAttribute("energy generation", "Energy gen/s", true);
	AddAttribute("solar collection", "Solar/s", true);
	AddAttribute("energy consumption", "Energy use/s", false, true);
	AddAttribute("heat generation", "Heat gen/s", false, true);
	AddAttribute("cooling", "Cooling/s", true);
	AddAttribute("shield generation", "Shields/s", true);
	AddAttribute("hull repair rate", "Hull/s", true);
	AddAttribute("fuel generation", "Fuel gen/s", true);
	AddAttribute("fuel consumption", "Fuel use/s", false, true);
	AddAttribute("energy capacity", "Energy cap.");
	Add("Crew", effect->crewBefore, effect->crewAfter);
	Add("Fuel", effect->fuelBefore, effect->fuelAfter);
	Add("Energy", effect->energyBefore, effect->energyAfter);
	for(const auto &[name, before] : effect->attributesBefore)
		if(!displayedAttributes.contains(name))
		{
			const double after = AttributeValue(effect->attributesAfter, name);
			if(before != after)
				rows.push_back({name, OutfitInfoDisplay::FormatAttribute(name, before),
					OutfitInfoDisplay::FormatAttribute(name, after), "ui/text primary"});
		}
	for(const auto &[name, after] : effect->attributesAfter)
		if(!displayedAttributes.contains(name) && !effect->attributesBefore.contains(name) && after)
			rows.push_back({name, "0", OutfitInfoDisplay::FormatAttribute(name, after), "ui/text primary"});
	const size_t capacity = 1 + static_cast<size_t>(max(0., (bounds.Height() - 65.) / 18.));
	const size_t visible = min(capacity, rows.size());
	const double rowStart = max(49., (bounds.Height() - 18. * visible) * .5);
	font.Draw({to_string(preview->selectedShipCount) + " selected, " +
		to_string(preview->eligibleShipCount) + " eligible",
		{static_cast<int>(bounds.Width() - 24), Alignment::RIGHT, Truncate::MIDDLE}},
		bounds.TopLeft() + Point(12, 8), Theme("ui/text secondary"));
	font.Draw("CURRENT", bounds.TopLeft() + Point(135, 31), Theme("ui/text muted"));
	font.Draw("AFTER", bounds.TopLeft() + Point(235, 31), Theme("ui/text muted"));
	if(rows.empty())
		font.Draw(preview->success || preview->hasEffects ? "Ship unchanged; hold or credits may change."
			: "Transfer blocked; ship unchanged.", bounds.TopLeft() + Point(12, 60),
			Theme(preview->success || preview->hasEffects ? "ui/text secondary" : "ui/caution"));
	for(size_t i = 0; i < visible; ++i)
	{
		const double y = bounds.Top() + rowStart + 18. * i;
		font.Draw({rows[i].label, {112, Alignment::LEFT, Truncate::MIDDLE}},
			Point(bounds.Left() + 12, y), Theme("ui/text secondary"));
		font.Draw({rows[i].before, {65, Alignment::RIGHT, Truncate::MIDDLE}},
			Point(bounds.Left() + 124, y), Theme("ui/text primary"));
		font.Draw("->", Point(bounds.Left() + 196, y), Theme("ui/text muted"));
		font.Draw({rows[i].after, {75, Alignment::RIGHT, Truncate::MIDDLE}},
			Point(bounds.Left() + 216, y), Theme(rows[i].color));
	}
	if(rows.size() > visible)
		font.Draw("+" + to_string(rows.size() - visible) + " more below", bounds.TopLeft() + Point(12, 30),
			Theme("ui/text muted"));
}


void ModernOutfitterPanel::DrawFleet(const Rectangle &bounds)
{
	FillShader::Fill(bounds, Theme("ui/panel"));
	Border(bounds, Theme("ui/divider"));
	const Font &font = FontSet::Get(14);
	font.Draw("SHIPS HERE (" + to_string(ships.size()) + ")", bounds.TopLeft() + Point(10, 9),
		Theme("ui/text secondary"));
	if(ships.size() > 1)
		DrawControl(Rectangle::FromCorner(Point(bounds.Right() - 46, bounds.Top() + 4), Point(42, 24)),
			"ALL", allShips && !customShipSelection,
			[this]() { allShips = !allShips; customShipSelection = false; customShips.clear(); Refresh(); });
	const Rectangle viewport = Rectangle::FromCorner(bounds.TopLeft() + Point(1, 34),
		Point(bounds.Width() - 2, bounds.Height() - 35));
	shipScroll.SetDisplaySize(viewport.Height());
	shipScroll.SetMaxValue(ships.size() * 32.);
	const vector<const Ship *> selectedShips = SelectedShipsForView();
	Clip(viewport);
	for(size_t i = 0; i < ships.size(); ++i)
	{
		const Ship *ship = ships[i];
		const Rectangle row = Rectangle::FromCorner(Point(bounds.Left() + 4,
			viewport.Top() + i * 32 - shipScroll.Value()), Point(bounds.Width() - 8, 30));
		if(row.Bottom() < viewport.Top() || row.Top() > viewport.Bottom())
			continue;
		const bool isSelected = find(selectedShips.begin(), selectedShips.end(), ship) != selectedShips.end();
		FillShader::Fill(row, Theme(isSelected ? "ui/selected" : "ui/raised"));
		if(isSelected)
			FillShader::Fill(Rectangle::FromCorner(row.TopLeft(), Point(3, row.Height())), Theme("ui/focus"));
		const string name = ship->GivenName().empty() ? ship->DisplayModelName() : ship->GivenName();
		font.Draw({name, {static_cast<int>(row.Width() - 40), Alignment::LEFT, Truncate::MIDDLE}},
			row.TopLeft() + Point(5, 8), Theme("ui/text primary"));
		if(row.Top() >= viewport.Top() && row.Bottom() <= viewport.Bottom())
		{
			shipZones.emplace_back(row, ship);
			if(ship != player.Flagship())
			{
				const Rectangle park = Rectangle::FromCorner(row.TopRight() + Point(-27, 3), Point(24, 24));
				DrawControl(park, ship->IsParked() ? "P" : "p", ship->IsParked(), [this, ship]() {
					player.ParkShip(ship, !ship->IsParked());
					previewDirty = true;
				});
			}
		}
	}
	EndClip();
}


void ModernOutfitterPanel::DrawCategories(const Rectangle &bounds)
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
			FillShader::Fill(Rectangle::FromCorner(row.TopLeft(), Point(3, row.Height())), Theme("ui/focus"));
		font.Draw({categories[i], {static_cast<int>(row.Width() - 12), Alignment::LEFT, Truncate::MIDDLE}},
			row.TopLeft() + Point(6, 8), Theme("ui/text primary"));
		if(row.Top() >= bounds.Top() + 34 && row.Bottom() <= bounds.Bottom())
			AddZone(row, [this, i]() { category = categories[i]; rowScroll.Set(0, 0); Refresh(); });
	}
	EndClip();
}


void ModernOutfitterPanel::DrawCatalog(const Rectangle &bounds)
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
			FillShader::Fill(Rectangle::FromCorner(row.TopLeft(), Point(3, row.Height())), Theme("ui/focus"));
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


void ModernOutfitterPanel::DrawDetailsPane(const Rectangle &bounds)
{
	FillShader::Fill(bounds, Theme("ui/panel"));
	Border(bounds, Theme("ui/divider"));
	if(!selected)
	{
		FontSet::Get(14).Draw("No outfit selected.", bounds.TopLeft() + Point(12, 48), Theme("ui/text secondary"));
		return;
	}
	const Font &font = FontSet::Get(14);
	const vector<const Ship *> viewShips = SelectedShipsForView();
	const Rectangle viewport = Rectangle::FromCorner(bounds.TopLeft() + Point(1, 40),
		Point(bounds.Width() - 2, bounds.Height() - 41));
	const double x = bounds.Left() + 12;
	const double width = bounds.Width() - 24;
	const double titleInset = compact && detailsPage ? 110. : 12.;
	FontSet::Get(18).Draw({selected->DisplayName(), {static_cast<int>(bounds.Width() - titleInset - 12), Alignment::LEFT, Truncate::MIDDLE}},
		bounds.TopLeft() + Point(titleInset, 10), Theme("ui/text primary"));
	const vector<PartStat> partStats = SelectedPartStats(*selected);
	const double leftWidth = width * .46;
	const double rightX = x + leftWidth + 16.;
	const double rightWidth = width - leftWidth - 16.;
	WrappedText description(font);
	description.SetAlignment(Alignment::LEFT);
	description.SetWrapWidth(max(100, static_cast<int>(leftWidth)));
	description.Wrap(selected->Description());
	const double heroHeight = max({250., 155. + description.Height(), 137. + 19. * partStats.size()});
	// Draw factual fitting data locally: OutfitInfoDisplay's requirements panel
	// also includes a depreciation-derived transaction price.
	vector<pair<string, string>> fittingData;
	for(const string &license : selected->Licenses())
		if(!player.HasLicense(license))
			fittingData.emplace_back("license needed:", license);
	if(selected->Mass())
		fittingData.emplace_back("mass:", Format::Number(selected->Mass()));
	if(selected->Get("required crew") > 0.)
		fittingData.emplace_back("required crew:", Format::Number(selected->Get("required crew")));
	for(const auto &[name, value] : *selected)
		if(value < 0. && name != "required crew" && !OutfitInfoDisplay::IsNotRequirement(name))
			fittingData.emplace_back(name + " needed:", Format::Number(-value));
	const double fittingHeight = fittingData.empty() ? 0. : 32. + 20. * fittingData.size();
	OutfitInfoDisplay info(*selected, player, false, false);
	auto effectHeight = [](const optional<TransferPlan> &plan, bool revealMappedNames) {
		if(!plan)
			return 0.;
		size_t visibleItems = count_if(plan->items.begin(), plan->items.end(), [&plan](const ItemEffect &item) {
			return ShowItemEffect(*plan, item);
		});
		size_t lines = plan->ships.size() + visibleItems + plan->allocation.size()
			+ plan->licensesAdded.size() + (revealMappedNames ? plan->mappedSystems.size()
				: !plan->mappedSystems.empty())
			+ (revealMappedNames ? plan->harvestedAdded.size() : !plan->harvestedAdded.empty()) + 3;
		lines += plan->cargoSizeBefore != plan->cargoSizeAfter
			|| plan->cargoFreeBefore != plan->cargoFreeAfter;
		for(const ShipEffect &effect : plan->ships)
		{
			for(const auto &[outfit, count] : effect.outfitsBefore)
			{
				auto after = effect.outfitsAfter.find(outfit);
				if(after == effect.outfitsAfter.end() || after->second != count)
					++lines;
			}
			for(const auto &[outfit, count] : effect.outfitsAfter)
				if(!effect.outfitsBefore.contains(outfit))
					++lines;
			lines += (effect.shieldsBefore != effect.shieldsAfter)
				+ (effect.hullBefore != effect.hullAfter)
				+ (effect.energyBefore != effect.energyAfter)
				+ (effect.fuelBefore != effect.fuelAfter)
				+ (effect.outfitSpaceBefore != effect.outfitSpaceAfter)
				+ (effect.weaponSpaceBefore != effect.weaponSpaceAfter)
				+ (effect.engineSpaceBefore != effect.engineSpaceAfter)
				+ (effect.cargoSpaceBefore != effect.cargoSpaceAfter)
				+ (effect.massBefore != effect.massAfter)
				+ (effect.accelerationBefore != effect.accelerationAfter)
				+ (effect.turnRateBefore != effect.turnRateAfter);
			for(const auto &[name, before] : effect.attributesBefore)
				if(name != "outfit space" && name != "weapon capacity" && name != "engine capacity"
						&& name != "cargo space" && before != AttributeValue(effect.attributesAfter, name))
					++lines;
			for(const auto &[name, after] : effect.attributesAfter)
				if(!effect.attributesBefore.contains(name) && after)
					++lines;
		}
		return 32. + 20. * lines;
	};
	const double contentHeight = heroHeight + (compact ? 110. : 0.) + fittingHeight + info.AttributesHeight() + 65.
		+ effectHeight(preview, false) + effectHeight(lastResult, true);
	detailScroll.SetDisplaySize(viewport.Height());
	detailScroll.SetMaxValue(contentHeight);
	const double offset = detailScroll.Value();
	Clip(viewport);
	const Sprite *sprite = selected->Thumbnail().GetSprite();
	if(sprite && sprite->IsLoaded())
		SpriteShader::Draw(sprite, Point(x + leftWidth * .5, viewport.Top() + 75 - offset),
			min(136. / max(sprite->Width(), sprite->Height()), 2.));
	font.Draw("BASE PRICE", Point(rightX, viewport.Top() + 10 - offset), Theme("ui/text secondary"));
	FontSet::Get(18).Draw(Format::CreditString(selected->Cost()),
		Point(rightX, viewport.Top() + 30 - offset), Theme("ui/text primary"));
	font.Draw("Installed " + to_string(Installed(selected)) + "    Cargo " +
		to_string(player.Cargo().Get(selected)) + "    Storage " + to_string(StorageCount(selected)),
		Point(rightX, viewport.Top() + 58 - offset), Theme("ui/text secondary"));
	font.Draw("Sold here: " + string(stock.Has(selected) ? "yes" : "no")
		+ "   Local stock: " + to_string(max(0, player.Stock(selected))),
		Point(rightX, viewport.Top() + 78 - offset), Theme("ui/text secondary"));
	FillShader::Fill(Rectangle::FromCorner(Point(rightX, viewport.Top() + 102 - offset),
		Point(rightWidth, 1)), Theme("ui/divider"));
	font.Draw("PART STATS", Point(rightX, viewport.Top() + 109 - offset), Theme("ui/text secondary"));
	for(size_t i = 0; i < partStats.size(); ++i)
	{
		const double rowY = viewport.Top() + 132 + 19. * i - offset;
		font.Draw({partStats[i].first, {static_cast<int>(rightWidth * .58), Alignment::LEFT, Truncate::MIDDLE}},
			Point(rightX, rowY), Theme("ui/text secondary"));
		font.Draw({partStats[i].second, {static_cast<int>(rightWidth * .40), Alignment::RIGHT, Truncate::MIDDLE}},
			Point(rightX + rightWidth * .60, rowY), Theme("ui/text primary"));
	}
	description.Draw(Point(x, viewport.Top() + 150 - offset), Theme("ui/text primary"));
	double y = viewport.Top() + heroHeight - offset;
	if(OwnedLicense(selected))
	{
		font.Draw("License held: yes", Point(x, y), Theme("ui/text secondary"));
		y += 22;
	}
	if(compact && !viewShips.empty())
	{
		const Ship *ship = viewShips.front();
		const string shipName = ship->GivenName().empty() ? ship->DisplayModelName() :
			ship->GivenName() + " (" + ship->DisplayModelName() + ")";
		font.Draw({viewShips.size() > 1 ? to_string(viewShips.size()) + " ships here selected" : shipName,
			{static_cast<int>(width), Alignment::LEFT, Truncate::MIDDLE}},
			Point(x, y), Theme("ui/text primary"));
		y += 22;
		auto space = [&viewShips](const char *name)
		{
			double low = viewShips.front()->Attributes().Get(name);
			double high = low;
			for(const Ship *candidate : viewShips)
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
	y += 12;
	if(!fittingData.empty())
	{
		font.Draw("FITTING REQUIREMENTS", Point(x, y), Theme("ui/text secondary"));
		y += 24;
		for(const auto &[label, value] : fittingData)
		{
			const double valueWidth = min(width * .46, static_cast<double>(font.Width(value) + 2));
			font.Draw({label, {static_cast<int>(width - valueWidth - 8), Alignment::LEFT, Truncate::BACK}},
				Point(x, y), Theme("ui/text secondary"));
			font.Draw({value, {static_cast<int>(valueWidth), Alignment::RIGHT, Truncate::MIDDLE}},
				Point(x + width - valueWidth, y), Theme("ui/text primary"));
			y += 20;
		}
		y += 8;
	}
	font.Draw("FULL NATIVE ATTRIBUTES", Point(x, y), Theme("ui/text secondary"));
	y += 22;
	info.DrawAttributes(Point(x, y));
	y += info.AttributesHeight() + 12;
	auto DrawEffects = [&](const TransferPlan &plan, const string &heading, bool revealMappedNames)
	{
		font.Draw(heading, Point(x, y), Theme("ui/text secondary"));
		y += 24;
		for(const ShipEffect &effect : plan.ships)
		{
			auto Count = [this](const map<const Outfit *, int> &outfits) {
				auto it = outfits.find(selected);
				return it == outfits.end() ? 0 : it->second;
			};
			const string name = effect.name.empty() ? "Ship " + to_string(effect.fleetIndex + 1) : effect.name;
			const string line = name + ": " + to_string(Count(effect.outfitsBefore)) + " -> "
				+ to_string(Count(effect.outfitsAfter)) + " installed; crew "
				+ to_string(effect.crewBefore) + " -> " + to_string(effect.crewAfter);
			font.Draw({line, {static_cast<int>(width), Alignment::LEFT, Truncate::BACK}},
				Point(x, y), Theme("ui/text primary"));
			y += 20;
			for(const auto &[outfit, before] : effect.outfitsBefore)
			{
				auto after = effect.outfitsAfter.find(outfit);
				const int count = after == effect.outfitsAfter.end() ? 0 : after->second;
				if(count != before)
				{
					const string change = "  " + outfit->DisplayName() + ": " + to_string(before)
						+ " -> " + to_string(count);
					font.Draw({change, {static_cast<int>(width), Alignment::LEFT, Truncate::MIDDLE}},
						Point(x, y), Theme("ui/text secondary"));
					y += 20;
				}
			}
			for(const auto &[outfit, count] : effect.outfitsAfter)
				if(!effect.outfitsBefore.contains(outfit))
				{
					const string change = "  " + outfit->DisplayName() + ": 0 -> " + to_string(count);
					font.Draw({change, {static_cast<int>(width), Alignment::LEFT, Truncate::MIDDLE}},
						Point(x, y), Theme("ui/text secondary"));
					y += 20;
				}
			auto Resource = [&](const char *name, double before, double after)
			{
				if(before == after)
					return;
				font.Draw(string("  ") + name + ": " + Format::Number(before) + " -> "
					+ Format::Number(after), Point(x, y), Theme("ui/text secondary"));
				y += 20;
			};
			Resource("shields", effect.shieldsBefore, effect.shieldsAfter);
			Resource("hull", effect.hullBefore, effect.hullAfter);
			Resource("energy", effect.energyBefore, effect.energyAfter);
			Resource("fuel", effect.fuelBefore, effect.fuelAfter);
			Resource("outfit space", effect.outfitSpaceBefore, effect.outfitSpaceAfter);
			Resource("weapon space", effect.weaponSpaceBefore, effect.weaponSpaceAfter);
			Resource("engine space", effect.engineSpaceBefore, effect.engineSpaceAfter);
			Resource("cargo space", effect.cargoSpaceBefore, effect.cargoSpaceAfter);
			Resource("ship mass", effect.massBefore, effect.massAfter);
			Resource("acceleration", 3600. * effect.accelerationBefore, 3600. * effect.accelerationAfter);
			Resource("turning rate", 60. * effect.turnRateBefore, 60. * effect.turnRateAfter);
			for(const auto &[name, before] : effect.attributesBefore)
				if(name != "outfit space" && name != "weapon capacity" && name != "engine capacity"
						&& name != "cargo space")
				{
					const double after = AttributeValue(effect.attributesAfter, name);
					if(before != after)
					{
						const string line = "  " + name + ": " + OutfitInfoDisplay::FormatAttribute(name, before)
							+ " -> " + OutfitInfoDisplay::FormatAttribute(name, after);
						font.Draw({line, {static_cast<int>(width), Alignment::LEFT, Truncate::MIDDLE}},
							Point(x, y), Theme("ui/text secondary"));
						y += 20;
					}
				}
			for(const auto &[name, after] : effect.attributesAfter)
				if(!effect.attributesBefore.contains(name) && after)
				{
					const string line = "  " + name + ": 0 -> " + OutfitInfoDisplay::FormatAttribute(name, after);
					font.Draw({line, {static_cast<int>(width), Alignment::LEFT, Truncate::MIDDLE}},
						Point(x, y), Theme("ui/text secondary"));
					y += 20;
				}
		}
		if(plan.cargoSizeBefore != plan.cargoSizeAfter || plan.cargoFreeBefore != plan.cargoFreeAfter)
		{
			const string line = "Fleet cargo: " + Format::Number(plan.cargoFreeBefore) + " / "
				+ Format::Number(plan.cargoSizeBefore) + " -> " + Format::Number(plan.cargoFreeAfter)
				+ " / " + Format::Number(plan.cargoSizeAfter);
			font.Draw({line, {static_cast<int>(width), Alignment::LEFT, Truncate::MIDDLE}},
				Point(x, y), Theme("ui/text secondary"));
			y += 20;
		}
		for(const ItemEffect &item : plan.items)
		{
			if(!ShowItemEffect(plan, item))
				continue;
			const string line = item.outfit->DisplayName() + ": cargo " + to_string(item.cargoBefore)
				+ " -> " + to_string(item.cargoAfter) + ", storage " + to_string(item.storageBefore)
				+ " -> " + to_string(item.storageAfter) + ", local stock "
				+ to_string(max(0, item.stockBefore)) + " -> " + to_string(max(0, item.stockAfter));
			font.Draw({line, {static_cast<int>(width), Alignment::LEFT, Truncate::BACK}},
				Point(x, y), Theme("ui/text secondary"));
			y += 20;
		}
		for(const string &license : plan.licensesAdded)
		{
			font.Draw("License gained: " + license, Point(x, y), Theme("ui/text primary"));
			y += 20;
		}
		if(!revealMappedNames && !plan.mappedSystems.empty())
		{
			font.Draw("Will reveal map coverage after purchase",
				Point(x, y), Theme("ui/text primary"));
			y += 20;
		}
		else if(revealMappedNames)
			for(const System *system : plan.mappedSystems)
			{
				font.Draw("Mapped: " + system->TrueName(), Point(x, y), Theme("ui/text primary"));
				y += 20;
			}
		if(!revealMappedNames && !plan.harvestedAdded.empty())
		{
			font.Draw("Will reveal resource locations after purchase",
				Point(x, y), Theme("ui/text primary"));
			y += 20;
		}
		else if(revealMappedNames)
			for(const auto &[system, outfit] : plan.harvestedAdded)
			{
				const string line = "Discovered: " + outfit->DisplayName() + " in " + system->TrueName();
				font.Draw({line, {static_cast<int>(width), Alignment::LEFT, Truncate::MIDDLE}},
					Point(x, y), Theme("ui/text primary"));
				y += 20;
			}
		if(!plan.allocation.empty())
		{
			font.Draw("ALLOCATION ORDER", Point(x, y), Theme("ui/text secondary"));
			y += 20;
			for(size_t i = 0; i < plan.allocation.size(); ++i)
			{
				const AllocationStep &step = plan.allocation[i];
				auto it = find_if(plan.ships.begin(), plan.ships.end(), [&step](const ShipEffect &effect) {
					return effect.fleetIndex == step.fleetIndex;
				});
				const string shipName = it == plan.ships.end() || it->name.empty()
					? "Ship " + to_string(step.fleetIndex + 1) : it->name;
				const string line = to_string(i + 1) + ". " + shipName + ": " + step.outfit->DisplayName()
					+ " " + RouteName(step.from) + " -> " + RouteName(step.to);
				font.Draw({line, {static_cast<int>(width), Alignment::LEFT, Truncate::MIDDLE}},
					Point(x, y), Theme("ui/text primary"));
				y += 20;
			}
		}
		y += 8;
	};
	if(preview)
		DrawEffects(*preview, "PLANNED SHIP AND HOLD CHANGES", false);
	if(lastResult)
		DrawEffects(*lastResult, "LAST RESULT", true);
	EndClip();
}


bool ModernOutfitterPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
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
	else if((key == SDLK_UP || key == SDLK_DOWN) && (mod & (KMOD_CTRL | KMOD_GUI)))
		ReorderSelectedShip(key == SDLK_UP ? -1 : 1);
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
	else if(key == SDLK_PAGEUP || key == SDLK_PAGEDOWN || key == SDLK_HOME || key == SDLK_END)
	{
		ScrollVar<double> *scroll = &rowScroll;
		if(compact && detailsPage)
			scroll = &detailScroll;
		else if(hoverPane == HoverPane::CATEGORY)
			scroll = &categoryScroll;
		else if(hoverPane == HoverPane::FLEET)
			scroll = &shipScroll;
		else if(hoverPane == HoverPane::DETAILS)
			scroll = &detailScroll;
		if(key == SDLK_HOME || key == SDLK_END)
			scroll->Set(key == SDLK_HOME ? 0. : scroll->MaxValue(), 0);
		else
			scroll->Scroll((key == SDLK_PAGEUP ? -1. : 1.) * scroll->DisplaySize() * .85, 0);
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
	else if(key == SDLK_v)
	{
		sort = sort == Sort::NAME ? Sort::PRICE : Sort::NAME;
		Refresh();
	}
	else if(key == SDLK_g)
	{
		const OutfitLocation locations[] = {OutfitLocation::Shop, OutfitLocation::Ship,
			OutfitLocation::Cargo, OutfitLocation::Storage};
		auto it = find(begin(locations), end(locations), actionFrom);
		actionFrom = locations[(it - begin(locations) + 1) % 4];
		if(actionTo == actionFrom)
			actionTo = actionFrom == OutfitLocation::Ship ? OutfitLocation::Shop : OutfitLocation::Ship;
		previewDirty = true;
		UpdatePreview();
	}
	else if(key == SDLK_t)
	{
		const OutfitLocation locations[] = {OutfitLocation::Shop, OutfitLocation::Ship,
			OutfitLocation::Cargo, OutfitLocation::Storage};
		auto it = find(begin(locations), end(locations), actionTo);
		int index = it - begin(locations);
		do
			index = (index + 1) % 4;
		while(locations[index] == actionFrom);
		actionTo = locations[index];
		previewDirty = true;
		UpdatePreview();
	}
	else if(key == SDLK_b || key == SDLK_s || key == SDLK_i || key == SDLK_u
			|| key == SDLK_c || key == SDLK_r)
		HandleNativeShortcut(key);
	else if(key == SDLK_h)
		CycleShip();
	else if(key == SDLK_a && ships.size() > 1)
	{
		allShips = !allShips;
		Refresh();
	}
	else if(key == SDLK_TAB || key == SDLK_SLASH || key == SDLK_f)
		search->SetFocus(true);
	else if(key == SDLK_RETURN && compact && selected && !detailsPage)
		detailsPage = true;
	else if(key == SDLK_RETURN && selected)
		CommitSelectedRoute();
	else if(command.Has(Command::HELP) || command.Has(Command::MAP)
			|| key == 'l' || key == 'd' || key == 'k'
			|| (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI)))
			|| (key == 'p' && (mod & KMOD_SHIFT)) || (key >= '0' && key <= '9'))
	{
		SyncNativeSelection();
		const bool handled = ShopPanel::KeyDown(key, mod, command, isNewPress);
		if(key == 'k' || (key == 'p' && (mod & KMOD_SHIFT)) || (key >= '0' && key <= '9'))
		{
			customShipSelection = true;
			customShips = playerShips;
			allShips = false;
			Refresh();
		}
		return handled;
	}
	else
		return true; // Consume unrelated game shortcuts while this panel is active.
	return true;
}


bool ModernOutfitterPanel::Click(int x, int y, MouseButton button, int clicks)
{
	if(button != MouseButton::LEFT || !fleetBounds.Contains(Point(x, y)))
		return false;
	const bool handled = ShopPanel::Click(x, y, button, clicks);
	if(handled)
	{
		customShipSelection = true;
		customShips = playerShips;
		allShips = false;
		Refresh();
	}
	return handled;
}



bool ModernOutfitterPanel::Drag(double dx, double dy)
{
	if(!dragShip)
		return false;
	const bool handled = ShopPanel::Drag(dx, dy);
	RefreshShips();
	return handled;
}



bool ModernOutfitterPanel::Release(int x, int y, MouseButton button)
{
	return ShopPanel::Release(x, y, button);
}


bool ModernOutfitterPanel::Scroll(double, double dy)
{
	if(hoverPane == HoverPane::CATEGORY)
		categoryScroll.Scroll(-dy * 30., 0);
	else if(hoverPane == HoverPane::FLEET)
		shipScroll.Scroll(-dy * 30., 0);
	else if(hoverPane == HoverPane::DETAILS || (compact && detailsPage))
		detailScroll.Scroll(-dy * 30., 0);
	else
		rowScroll.Scroll(-dy * 30., 0);
	return true;
}


bool ModernOutfitterPanel::Hover(int x, int y)
{
	const Point point(x, y);
	hoverPane = categoryBounds.Contains(point) ? HoverPane::CATEGORY :
		(fleetBounds.Contains(point) ? HoverPane::FLEET :
			(detailBounds.Contains(point) && (!compact || detailsPage) ? HoverPane::DETAILS : HoverPane::ROWS));
	return true;
}



void ModernOutfitterPanel::EndEditing()
{
	search->SetFocus(false);
	selectedQuantity->SetFocus(false);
}
