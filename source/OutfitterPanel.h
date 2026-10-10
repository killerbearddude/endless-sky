/* OutfitterPanel.h
Copyright (c) 2014 by Michael Zahniser

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

#include "ShopPanel.h"

#include "Sale.h"

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

class Outfit;
class PlayerInfo;
class Point;
class Ship;
class System;



// Class representing the Outfitter UI panel, which allows you to buy new
// outfits to install in your ship or to sell the ones you own. Any outfit you
// sell is available to be bought again until you close this panel, even if it
// is not normally sold here. You can also directly install any outfit that you
// have plundered from another ship and are storing in your cargo bay. This
// panel makes an attempt to ensure that you do not leave with a ship that is
// configured in such a way that it cannot fly (e.g. no engines or steering).
class OutfitterPanel : public ShopPanel {
public:
	// Define locations which items may move to and from within the outfitter.
	enum class OutfitLocation {
		Ship,
		Shop,
		Cargo,
		Storage,
	};

	struct ItemEffect {
		const Outfit *outfit = nullptr;
		int cargoBefore = 0;
		int cargoAfter = 0;
		int storageBefore = 0;
		int storageAfter = 0;
		int stockBefore = 0;
		int stockAfter = 0;
		bool operator==(const ItemEffect &) const = default;
	};

	struct ShipEffect {
		size_t fleetIndex = 0;
		std::string name;
		std::map<const Outfit *, int> outfitsBefore;
		std::map<const Outfit *, int> outfitsAfter;
		int crewBefore = 0;
		int crewAfter = 0;
		double shieldsBefore = 0.;
		double shieldsAfter = 0.;
		double hullBefore = 0.;
		double hullAfter = 0.;
		double energyBefore = 0.;
		double energyAfter = 0.;
		double fuelBefore = 0.;
		double fuelAfter = 0.;
		double outfitSpaceBefore = 0.;
		double outfitSpaceAfter = 0.;
		double weaponSpaceBefore = 0.;
		double weaponSpaceAfter = 0.;
		double engineSpaceBefore = 0.;
		double engineSpaceAfter = 0.;
		double cargoSpaceBefore = 0.;
		double cargoSpaceAfter = 0.;
		double massBefore = 0.;
		double massAfter = 0.;
		double accelerationBefore = 0.;
		double accelerationAfter = 0.;
		double turnRateBefore = 0.;
		double turnRateAfter = 0.;
		std::map<std::string, double> attributesBefore;
		std::map<std::string, double> attributesAfter;
		bool operator==(const ShipEffect &) const = default;
	};

	struct AllocationStep {
		size_t fleetIndex = 0;
		const Outfit *outfit = nullptr;
		int quantity = 0;
		OutfitLocation from = OutfitLocation::Shop;
		OutfitLocation to = OutfitLocation::Ship;
		bool operator==(const AllocationStep &) const = default;
	};

	struct TransferPlan {
		OutfitLocation from = OutfitLocation::Shop;
		OutfitLocation to = OutfitLocation::Ship;
		SDL_Keycode shortcutKey = 0;
		const Outfit *outfit = nullptr;
		int requestedQuantity = 1;
		int selectedShipCount = 0;
		int eligibleShipCount = 0;
		int fulfilledQuantity = 0;
		bool quantityPerShip = false;
		bool success = false;
		bool hasEffects = false;
		std::string reason;
		int64_t creditsBefore = 0;
		int64_t creditsAfter = 0;
		int cargoSizeBefore = 0;
		int cargoSizeAfter = 0;
		double cargoFreeBefore = 0.;
		double cargoFreeAfter = 0.;
		int largestHoldBefore = 0;
		int largestHoldAfter = 0;
		std::string fleetDepreciationAfter;
		std::string stockDepreciationAfter;
		std::vector<ItemEffect> items;
		std::vector<ShipEffect> ships;
		std::vector<AllocationStep> allocation;
		std::vector<std::string> licensesAdded;
		std::vector<const System *> mappedSystems;
		std::vector<std::pair<const System *, const Outfit *>> harvestedAdded;
		std::string precondition;
		bool operator==(const TransferPlan &) const = default;
	};

	struct CommitResult {
		bool committed = false;
		bool stale = false;
		bool matched = false;
		TransferPlan plan;
	};


public:
	OutfitterPanel(PlayerInfo &player, const Sale<Outfit> &stock);
	bool SelectOutfitForTest(const std::string &name, int quantity, bool allShips) override;
	bool PreviewOutfitForTest(const std::string &from, const std::string &to) override;
	bool VerifyOutfitPreviewForTest() const override;
	bool PreviewHasHarvestedForTest() const override;
	bool CommitOutfitPreviewForTest(bool expectStale) override;
	TransferPlan PreviewMoveOutfit(OutfitLocation from, OutfitLocation to,
		const std::string &actionName = "no action specified", SDL_Keycode shortcutKey = 0) const;
	TransferPlan PreviewShortcut(SDL_Keycode key) const;
	CommitResult CommitMoveOutfit(const TransferPlan &preview,
		const std::string &actionName = "no action specified");

	virtual void Step() override;


protected:
	virtual int TileSize() const override;
	virtual int VisibilityCheckboxesSize() const override;
	virtual bool HasItem(const std::string &name) const override;
	virtual void DrawItem(const std::string &name, const Point &point) override;
	virtual double DrawDetails(const Point &center) override;
	TransactionResult CanMoveOutfit(OutfitLocation fromLocation, OutfitLocation toLocation,
		const std::string &actionName = "no action specified") const;
	TransactionResult MoveOutfit(OutfitLocation fromLocation, OutfitLocation toLocation,
		const std::string &actionName = "no action specified") const;
	bool ButtonActive(char key, bool shipRelatedOnly = false);
	virtual bool ShouldHighlight(const Ship *ship) override;
	virtual void DrawKey() override;
	virtual std::optional<Rectangle> KeyArea() const override;

	// Toggles for the display filters.
	void ToggleForSale();
	void ToggleInstalled();
	void ToggleStorage();
	void ToggleCargo();

	virtual int FindItem(const std::string &text) const override;

	virtual double ButtonPanelHeight() const override;
	virtual void DrawButtons() override;
	virtual TransactionResult HandleShortcuts(SDL_Keycode key) override;


private:
	struct TransferEvent {
		Ship *ship = nullptr;
		const Outfit *outfit = nullptr;
		int quantity = 0;
		OutfitLocation from = OutfitLocation::Shop;
		OutfitLocation to = OutfitLocation::Ship;
	};
	void RecordTransfer(Ship *ship, const Outfit *outfit, int quantity,
		OutfitLocation from, OutfitLocation to) const;
	mutable std::vector<TransferEvent> *transferEvents = nullptr;
	mutable std::optional<std::pair<OutfitLocation, OutfitLocation>> lastRoute;
	std::string TransferPrecondition() const;
	bool MatchesTransferOutcome(const TransferPlan &plan) const;
	std::optional<TransferPlan> testPreview;
	std::vector<Ship *> transferOrder;
	static bool ShipCanAdd(const Ship *ship, const Outfit *outfit);
	static bool ShipCanRemove(const Ship *ship, const Outfit *outfit);
	void DrawOutfit(const Outfit &outfit, const Point &center, bool isSelected, bool isOwned) const;
	bool HasLicense(const std::string &name) const;
	void CheckRefill();
	void Refill();
	// Shared code for reducing the selected ships to those that have the
	// same quantity of the selected outfit.
	const std::vector<Ship *> GetShipsToOutfit(bool isInstall = false) const;


private:
	// Record whether we've checked if the player needs ammo refilled.
	bool checkedRefill = false;
	// Allow toggling whether outfits that are for sale are shown.
	bool showForSale = true;
	// Allow toggling whether installed outfits are shown.
	bool showInstalled = true;
	// Allow toggling whether stored outfits are shown.
	bool showStorage = true;
	// Allow toggling whether outfits in cargo are shown.
	bool showCargo = true;

	Sale<Outfit> outfitter;

	// Keep track of whether the outfitter help screens have been shown.
	bool checkedHelp = false;

	int shipsHere = 0;
};
