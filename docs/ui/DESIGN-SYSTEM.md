# Native UI design and interaction contract v0.1

Status: initial M0 contract with an M1-A additive native palette and opt-in proof screen. The approved palette specification defines the semantic sRGB roles below. The approved browser prototype at `ed44a8a` supplies hierarchy and interaction direction; its illustrative ship, static catalog, base prices, undo/reset and browser layout do not define native gameplay. Supplemental color concept imagery informs atmosphere and balance only, not layout, controls, or mechanics.

## Color roles

| Role | Reference | Use |
| --- | --- | --- |
| Background / panel / raised | `#0C0F11` / `#14191D` / `#1D252B` | Quiet structure |
| Hover / divider / control border | `#27333B` / `#39464F` / `#71818C` | Hover, boundaries, inactive controls |
| Primary / secondary / muted text | `#E8EEF2` / `#A8B5BF` / `#7F909C` | Hierarchy; muted text must not carry required actions |
| Focus / selected surface | `#69D2E7` / `#18333C` | Keyboard focus, active selection, primary actions |
| Positive / caution / danger | `#87C99A` / `#E6B566` / `#EE8282` | Outcome, capacity pressure, blocked action |

These are reference sRGB values from the approved palette. M1-A adds them as normalized, namespaced `ui/` colors in `data/_ui/interfaces.txt`; it does not change legacy names or plugin override order. The opt-in proof renders all roles in the native game and labels focus, selected surfaces, and statuses in text as well as color. This is a native visual check of the proof surface, not acceptance of an Outfitter design or a measured contrast audit.

## Layout and type

- Roles: screen title, section heading, row label, numeric value, supporting description, warning, and disabled reason. Tabular figures align by decimal/right edge with units visible. Missing or inapplicable metrics show an unavailable marker; never substitute fabricated zero.
- Start with existing `FontSet`, `Font`, `Table`, `Tooltip`, sprites and drawing helpers. Exact Ubuntu font and prototype icon treatment are unresolved in native assets; check glyph coverage and provenance before adding assets.
- Use a restrained spacing scale as an implementation starting point: 4, 8, 12, 16, 24 logical units. This is proposed, not a measured baseline. Keep hit boxes, clipping, focus outline and drawn bounds derived from the same rectangles.
- Wide view may show catalog, selected-ship context and full details side by side. Standard view prioritizes catalog plus reachable details. Compact view prioritizes selection, source/ownership, actual action and complete description with independent scrolling or a deliberate details transition. Do not hide fleet selection or action reasons.
- Determine mode from effective **logical width and height**, UI zoom, measured font dimensions, long names/descriptions and category count. Test 1024x768 and 1920x1080 physical windows as starting fixtures, then record actual effective dimensions at each supported zoom. Minimum native sizes and performance budgets remain open until Outfitter and comparison-screen measurements are taken.

## Outfitter presentation rules

- Show active selected ship and selected fleet count; a multi-ship action must distinguish eligible/affected ships and per-ship allocation. Preserve ship groups, parking, ordering and keyboard selection from `ShopPanel`.
- Show current owned/installed/cargo/local-storage quantities together with shop availability. Filters or tabs may narrow the list but may not make ownership invisible or erase custom categories.
- Label quantity as **per selected ship** when a ship is the source or destination, otherwise as a **total across the hold/shop**. Show actual fulfilled quantity and partial result. Do not show an exact preview until it shares verified native rules with execution.
- Distinguish selected-ship fitting capacity from fleet cargo and planetary storage. Explicit routes must name their source and destination; preserve legacy shortcut fallback order and make that order discoverable.
- Use category-relevant columns, with a generic attribute fallback for plugin categories. Keep full outfit descriptions reachable, including at compact sizes. Metrics need source, units, scenario and visibility rules before showing derived energy/heat claims.
- Empty selection/results clear stale detail and financial previews. Disabled actions display a native validation reason. Search/quantity fields must consume typed shortcut letters and keep keyboard focus predictable after dialogs.

## Native capability inventory

| Capability | Direction | Source and remaining check |
| --- | --- | --- |
| Panel stack, event trap, keyboard focus and zones | Reuse/adapt | `Panel.h`, `UI.cpp`; verify underlying command leakage and modal focus |
| Data-defined interface, named points/boxes and colors | Reuse | `Interface.cpp`, `GameData.cpp`, `data/_ui/interfaces.txt`; audit plugin overrides before changing identifiers |
| Text input, dropdown, table, scroll, tooltip | Adapt | `Edit`, `Dropdown`, `text/Table`, `ScrollArea`, `ScrollBar`, `Tooltip`; inspect focus, clipping and popup bounds in M1-A |
| Font and image paths | Reuse/unknown | `text/FontSet`, `text/Font`, `image/SpriteSet`; verify font/icon suitability at zoom |
| Shared shop selection, navigation and transaction buttons | Adapt | `ShopPanel.cpp`; changing its geometry affects Shipyard |
| New layout primitives | New only where needed | M1-A may add a small rectangle/layout helper after the native proof; no general widget framework |

`Interface::Load` clears and reloads named definitions, and `Interface` resolves named points/boxes. `data/_ui/interfaces.txt` contains `planet`, small-screen landed services, `hud`, map, menu and other contracts. `ShopPanel` currently draws much of Outfitter geometry directly in C++; replacing it does not imply those data-defined contracts can be ignored elsewhere. Content plugins and interface/color override plugins require separate compatibility fixtures.

## Verified native control behavior and M1-A implications

| Existing mechanism | Source finding at `902bca70c` | M1-A decision |
| --- | --- | --- |
| `UI` / `Panel` | `UI::Handle` converts SDL pointer positions by `Screen::Zoom`, visits top panels first and stops when `TrapAllEvents()` is true. `Panel::DoKeyDown` sends a key to the focused child without also calling the parent; `Panel::SetFocus` clears other focus in that panel tree. `Panel::DoDraw` draws children after the parent. | Reuse stack, child focus and event trap. Test SDL key and text events separately, plus dialog close/focus restoration. |
| `Interface` / `Information` | `Interface::Load` clears elements, points, values and lists when a named definition is reloaded. Buttons create `Panel` zones from their rendered bounds; `visible`/`active` conditions control following elements. Missing named point/box/value returns zero/empty, so missing override entries can silently collapse geometry. | Preserve names and fallback behavior. Add only namespaced theme values; check complete named interface replacements and both small-screen variants. |
| `Edit` | Text is delivered by `TextInput` only when focused; `KeyDown` handles caret movement, Tab/Shift+Tab focus, clipboard/history, Backspace/Delete and Escape focus release. `Panel::DoKeyDown` prevents a focused child key from reaching the parent within that tree. | Reuse for search only after checking its focus path under the new panel and under popup stack. Keep transaction shortcuts inactive while editing. |
| `Dropdown` | Extends `Edit`. Its drop zone opens a `DroppedPanel` child that traps events; it draws above or below based on screen bottom, and selection/dismissal differ for click versus long drag. Popup size is option count times row height. | Adapt or constrain for quantity/sort; test long lists, screen top/bottom, outside click, keyboard focus and return focus. Do not assume virtualized options or automatic screen clipping. |
| `ScrollArea` / `ScrollBar` / `ScrollVar` | `ScrollArea` renders content into a `RenderBuffer`, invalidates on rectangle/content changes, and owns scroll and scrollbar behavior. `ShopPanel` instead has three hand-managed scroll regions and typed zones. | Reuse the primitives where content fits; derive hit boxes and clipping from one rectangle set. Do not change shared ShopPanel geometry without Shipyard coverage. |
| `text/Table` / `TableArea` | `TableArea` is a scrolling text table whose columns have offsets/layout and whose rows invalidate its render buffer. It formats numeric cells through `Format::Number`; it is not an editable universal grid. | Adapt for read-only tabular content if measured width and selection behavior fit; keep item selection in Outfitter view state. |
| `Tooltip`, `FontSet`, `SpriteSet` | Existing panels create tooltips with named colors and update activation/text on preference changes. `FontSet::Get(size)` returns loaded bitmap fonts; `GameData` resolves default font image paths through enabled sources, so plugins may override the 14/18 assets. Sprite sources likewise come from base plus enabled plugins. | Reuse drawing and tooltips; inspect glyphs, font metrics and source order before committing typography/icons. No browser font or icon pipeline is assumed native. |

The `Edit` finding is limited to a **focused child of the same panel tree**. A different stacked popup, shortcut handling before focus is assigned, and key-repeat behavior still require M1-A input tests. Native clipping, tooltip placement and long-content fit are not proven by the menu screenshot.

## Data-defined override contract

`GameData::LoadSources` starts with base resources, then enabled global and local plugins; `UniverseObjects::Load` reads data from those sources. `UniverseObjects::LoadFile` loads named `color` values into `GameData::Colors()` and named `interface` values into `GameData::Interfaces()`. `Interface::Load` replaces a definition's elements, points, values and lists on re-load; optional `overwrite` also resets the object first. Thus a plugin-supplied definition of an existing interface can change drawing **and** the geometry that C++ reads. `GameData` also permits plugin-supplied image/fonts/shaders. These are native compatibility surfaces, not only appearance files.

| Named contract family in `data/_ui/interfaces.txt` | C++ use that must remain compatible |
| --- | --- |
| `planet`, `spaceport`, `news`, `hiring`, `trade`, `bank` and `(small screen)` variants | Landed services choose variants around logical width 1280 and read content boxes/column values; `PlanetPanel`, `SpaceportPanel`, `TradingPanel`, `BankPanel`, `HiringPanel`. |
| `map`, `map buttons`, `map buttons (small screen)`, `map detail panel`, `map planet card`, `map: sales key`, `map: mission view: key` | Map zoom bounds/duration, button bounds, sales list and detail-card geometry; `MapPanel`, `MapDetailPanel`, `MapSalesPanel`, `MissionPanel`. |
| `menu background`, `main menu`, `load menu`, `start conditions menu`, `controls`, `settings`, `plugins`, `audio`, `preferences`, `gamerules`, `gamerules presets` | Menu/Load/Preferences/Gamerules panels read named boxes and values such as `pilots`, `snapshots`, plugin list/description and volume bars. |
| `hud`, `main view`, `starfield`, `escort element`, `boarding`, `hail panel`, `message log`, `info panel` | Live HUD positions, view zoom list, contextual panel/message boxes and information layout. |
| Named colors including `active`, `inactive`, `hover`, `panel background`, `shop side panel background`, `outfitter difference highlight` | Shared button states, shop sidebars and comparison highlighting; new semantic colors must not repurpose these names silently. |

M1 compatibility fixtures should separately load: (1) a content plugin with custom outfit category/attribute and no interface override; (2) an interface/color override plugin that changes a known box, point, value and color while keeping the required names. Verify both wide and small-screen variants, plus the native default fallback. A content plugin test cannot stand in for the UI-override test. M0 establishes these contracts and fixtures; actual compatibility regression runs belong to the affected implementation patches.

## M1-B implemented catalog behavior

The opt-in native catalog uses `ui/` semantic colors for panels, controls, text, selection, and focus. A separate color-override plugin changed `ui/focus` in a native capture; existing named interface definitions and legacy color names remain unchanged. Category order follows the native outfit `CategoryList`, with visible custom categories appended. Rows show thumbnail, name, base cost, and concise availability/ownership counts. A selected outfit keeps pointer identity across source/sort changes while visible; an empty result clears details.

At standard and wide logical widths, catalog and details share the screen. Below the current 1040-logical-unit layout threshold, selecting a row opens a detail page with a Back control; search and source controls remain in the header. The detail pane clips and scrolls full descriptions and native requirements/attributes. The view displays keyboard hints for Tab search, Left/Right categories, bracket source changes, S sort, H ship cycling, and A all-ships selection; Up/Down move by displayed row order. Three actual native cases were captured: 1280×800, 1800×980, and 1280×800 at 120% UI zoom, plus a separate 960×680 compact case. The numeric threshold is an implementation choice for M1-B, not a universal UI breakpoint. Human review, other font/plugin combinations, and a measured accessibility contrast audit remain open.

## M2 implemented transaction presentation

The opt-in modern Outfitter (Ctrl+Alt+O) uses the M1-B semantic `ui/` colors and catalog hierarchy and the loaded native 14-point font for transaction controls. The footer stays visible under the detail pane at wide sizes and on the compact detail page. Current credits and fleet cargo capacity, selected/eligible/affected ships, source, destination, requested quantity meaning, actual fulfillment, and credit delta have text labels. Local stock is displayed as a nonnegative owned count; the native negative accounting value for purchases from an unlimited shop is kept in the transaction state, not presented as available inventory. The catalog's price remains explicitly labeled **base price**; the evaluator supplies the transfer-specific credit delta.

Four source buttons and three destination buttons expose the twelve directed routes. G and T cycle the route by keyboard, Enter commits it, and B/S/I/U/C/R preserve their native shortcut fallback paths. V sorts the catalog so Shift+S remains a quantity-modified Sell. F or Tab focuses search; search focus consumes action letters until released. PageUp/PageDown/Home/End scroll the visible catalog, category, fleet, or detail pane. When a route cannot proceed, its native reason is available in a full dialog. After an action, a partial result is labeled, and the details scroll shows exact per-ship and hold changes, fitting/cargo capacity and mass changes, licenses, mapped systems, and allocation order. Unchanged, absent linked outfits are omitted from the visible preview. Before buying a map, the preview says it will reveal coverage without giving a count or names of undiscovered systems or minable resources; the result names those systems and resources once the player has discovered them. The modern fleet list uses the native ShopPanel mouse selection and drag handlers; K, digit groups, and Ctrl+Up/Down expose parking, grouping, and reordering by keyboard. Compact selection opens a detail page with Back and persistent transaction controls.

A precise quote exists only for a request evaluated by the shared native `MoveOutfit` rules on isolated state. A commit refreshes the quote if its preconditions changed. Named `Interface` definitions and legacy colors were not replaced; an installed `ui/focus` override changed focus/selection rendering in a native capture. M2 did not add energy/heat viability estimates, transaction undo, a shopping cart, or a save preference.
