/* ModernOutfitterPanel.h
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

#pragma once

#include "OutfitterPanel.h"
#include "Sale.h"
#include "ScrollVar.h"

#include <memory>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <vector>

class Edit;
class Outfit;
class PlayerInfo;
class Ship;

// Opt-in modern Outfitter presentation using the native OutfitterPanel rules.
class ModernOutfitterPanel final : public OutfitterPanel {
public:
	ModernOutfitterPanel(PlayerInfo &player, const Sale<Outfit> &stock);
	bool SelectOutfitForTest(const std::string &name, int quantity, bool allShips) override;
	bool SelectOutfitRouteForTest(const std::string &from, const std::string &to) override;
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
	enum class Source { ALL, SHOP, INSTALLED, CARGO, STORAGE };
	enum class Sort { NAME, PRICE };
	void Refresh();
	void RebuildCatalog();
	bool VisibleInSource(const Outfit *outfit) const;
	int Installed(const Outfit *outfit) const;
	int StorageCount(const Outfit *outfit) const;
	bool OwnedLicense(const Outfit *outfit) const;
	void SelectRow(int index);
	void CycleShip();
	void DrawDetailsPane(const Rectangle &bounds);
	void DrawCategories(const Rectangle &bounds);
	void DrawFleet(const Rectangle &bounds);
	void DrawCatalog(const Rectangle &bounds);
	void DrawShipContext(const Rectangle &bounds);
	void DrawPreviewComparison(const Rectangle &bounds);
	void DrawControl(const Rectangle &bounds, const std::string &text, bool selected,
		const std::function<void()> &action);
	void DrawActions(const Rectangle &bounds);
	void SyncNativeSelection();
	void RefreshShips();
	void UpdatePreview();
	void CommitSelectedRoute();
	void HandleNativeShortcut(SDL_Keycode key);
	void ReorderSelectedShip(int direction);
	std::vector<const Ship *> SelectedShipsForView() const;

private:
	Sale<Outfit> stock;
	std::shared_ptr<Edit> search;
	std::vector<std::string> categories;
	std::vector<const Outfit *> all;
	std::vector<const Outfit *> rows;
	std::vector<const Ship *> ships;
	std::string category;
	const Outfit *selected = nullptr;
	Source source = Source::ALL;
	Sort sort = Sort::NAME;
	size_t shipIndex = 0;
	bool allShips = false;
	bool customShipSelection = false;
	std::set<Ship *> customShips;
	bool detailsPage = false;
	bool queryDirty = false;
	OutfitLocation actionFrom = OutfitLocation::Shop;
	OutfitLocation actionTo = OutfitLocation::Ship;
	std::optional<TransferPlan> preview;
	std::optional<TransferPlan> lastResult;
	std::string feedback;
	std::string lastQuantity;
	bool previewDirty = true;
	bool wasTop = false;
	ScrollVar<double> categoryScroll;
	ScrollVar<double> shipScroll;
	ScrollVar<double> rowScroll;
	ScrollVar<double> detailScroll;
	Rectangle categoryBounds;
	Rectangle fleetBounds;
	Rectangle rowBounds;
	Rectangle detailBounds;
	bool compact = false;
	enum class HoverPane { CATEGORY, FLEET, ROWS, DETAILS } hoverPane = HoverPane::ROWS;
};
