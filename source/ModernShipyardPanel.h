/* ModernShipyardPanel.h
Copyright (c) 2026 by Daniel

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.
*/

#pragma once

#include "ShipyardPanel.h"
#include "ScrollVar.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

class Edit;

// Opt-in Shipyard presentation. ShipyardPanel still owns the native transactions,
// dialogs, mission checks, and fleet-capacity warning.
class ModernShipyardPanel final : public ShipyardPanel {
public:
	ModernShipyardPanel(PlayerInfo &player, const Sale<Ship> &stock);
	void Step() override;
	void Draw() override;

protected:
	bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	bool Click(int x, int y, MouseButton button, int clicks) override;
	bool Drag(double dx, double dy) override;
	bool Release(int x, int y, MouseButton button) override;
	bool Scroll(double dx, double dy) override;
	bool Hover(int x, int y) override;
	void EndEditing() override;

private:
	enum class Sort { NATIVE, NAME, PRICE };
	enum class Pane { CATEGORIES, CATALOG, FLEET, DETAILS };
	void RebuildCatalog();
	void Refresh();
	void RefreshFleet();
	void SelectRow(int index);
	void DrawControl(const Rectangle &bounds, const std::string &label, bool selected,
		const std::function<void()> &action, bool enabled = true);
	void DrawCategories(const Rectangle &bounds);
	void DrawCatalog(const Rectangle &bounds);
	void DrawFleet(const Rectangle &bounds);
	void DrawDetailsPane(const Rectangle &bounds);
	void DrawActions(const Rectangle &bounds);
	void NativeAction(SDL_Keycode key);
	void ReorderSelectedShip(int direction);

private:
	Sale<Ship> stock;
	std::shared_ptr<Edit> search;
	std::vector<std::string> categoryNames;
	std::vector<const Ship *> all;
	std::vector<const Ship *> rows;
	std::vector<const Ship *> fleet;
	std::string category;
	const Ship *selected = nullptr;
	Sort sort = Sort::NATIVE;
	bool queryDirty = false;
	bool compact = false;
	bool detailsPage = false;
	Pane hoverPane = Pane::CATALOG;
	ScrollVar<double> categoryScroll;
	ScrollVar<double> rowScroll;
	ScrollVar<double> fleetScroll;
	ScrollVar<double> detailScroll;
	Rectangle categoryBounds;
	Rectangle rowBounds;
	Rectangle fleetBounds;
	Rectangle detailBounds;
};
