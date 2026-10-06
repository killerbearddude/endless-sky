/* UIProofPanel.h
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

#include "Panel.h"

#include <memory>

class Edit;



// Opt-in native screen for checking the M1-A visual and input contracts.
// It never reads or changes player or game state.
class UIProofPanel final : public Panel {
public:
	UIProofPanel();

	void Step() override;
	void Draw() override;

protected:
	bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	bool Hover(int x, int y) override;

private:
	void OpenPopup();
	void RestoreFocus();

	std::shared_ptr<Edit> first;
	std::shared_ptr<Edit> second;
	Rectangle popupButton;
	bool initialFocusSet = false;
	bool hoveringButton = false;
	int shortcutCount = 0;
};
