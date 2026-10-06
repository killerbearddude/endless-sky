# Endless Sky — UI/UX Modernization Roadmap

**Version:** 0.1  
**Date:** September 22, 2026  
**Status:** M0 baseline and implementation contract complete on September 22, 2026; M1–M9 planned. See `STATUS.md` for evidence and limits.  
**Scope:** Whole-game native UI/UX modernization.  
**Primary constraint:** Endless Sky remains Endless Sky. Preserve gameplay, content, progression, simulation, economy, information-access rules, and compatible saves/plugins.

## 1. Purpose and authority

Modernize how the player sees, understands, navigates, and operates Endless Sky. Use the Outfitter prototype as the initial visual and interaction reference, not as an alternative gameplay model and not as a universal screen layout.

The user has confirmed the whole-game scope and gameplay-preservation requirement. The architecture, milestone sequence, patch boundaries, and class names below are proposals. A detailed API is not approved merely because it appears in this roadmap.

The order of authority is: agreed scope and compatibility constraints; characterized native behavior; approved screen interaction contracts; the approved semantic design system and palette; prototype hierarchy and interaction examples; supplemental concept imagery for atmosphere and color balance only. A conflict between the prototype and native mechanics must be resolved in favor of preserved mechanics unless a separate change is explicitly approved. M1-A starts from current upstream `master` at `fa22e46ae92b382509b4076caa859e36e6c16fa7` by owner direction; the pinned M0 commit below remains the historical M0 reference.

### Repository baseline checked for this roadmap

| Item | Verified reference |
|---|---|
| Repository | `killerbearddude/endless-sky` |
| Native `master` | `902bca70c3fa8bbb57a9349c7d372cdfcf06adc4` |
| Design branch | `ui-prototype` |
| Prototype commit | `ed44a8a929e79651b147c8e49e5ab8fa2e32340e` |
| Design reference | `prototypes/outfitter/design/reference.png` at the prototype commit |
| Prototype documentation | `prototypes/outfitter/README.md` at the prototype commit |

Both branch references were rechecked on September 22, 2026. The prototype README explicitly identifies a separate browser application, representative catalog, illustrative ship, and incomplete transfer model. Its JavaScript is not the native gameplay oracle. [R1–R3]

The original roadmap review inspected source and test definitions without building or launching the native game. M0 subsequently built and launched the unchanged native reference, ran the registered baseline suites, and added documentation. The browser prototype was not rerun for M0. See `STATUS.md` for actual commands and results.

## 2. Non-negotiable boundaries

### What may change

Presentation, visual hierarchy, navigation, discovery, filtering, sorting, descriptions of existing effects, selection mechanisms, feedback, accessibility-related interaction, and workflow. Native UI code may be reorganized when the change preserves behavior and makes the redesign maintainable.

### What must remain equivalent

For an equivalent supported player action in equivalent game state, preserve the resulting gameplay state and relevant event sequence. This includes prices, fitting rules, requested-versus-actual quantity semantics, stock, licenses, linked equipment, crew, missions, travel, combat commands, information visibility, time advancement, and pause behavior.

Gameplay parity is broader than a matching credit balance or save-file comparison. A new screen must not trigger a mission twice, suppress an offer, consume random numbers during rendering, unpause combat unexpectedly, or expose undiscovered information.

The current native outfitter can execute partial transfers, adjust crew, buy required licenses, handle maps specially, and remove linked outfits. Preserve those behaviors rather than simplifying them to match the prototype. [R6]

### Explicit exclusions

No new economy, balancing, AI, combat, progression, routing algorithm, content rewrite, automatic optimization, or new gameplay automation. No transaction undo, staged shopping cart, or atomic all-or-nothing transfer model unless separately approved. Browser reset/undo controls are prototype aids, not production requirements. No save-format migration as a prerequisite for the UI.

Existing gameplay defects must be recorded and evaluated separately. Parity does not mean preserving crashes or corruption indefinitely; it means not concealing unrelated fixes inside UI work. Malformed input must never be silently converted into a different player action.

## 3. Delivery strategy

Make one adjustment to the earlier outline: do not finish a general widget framework or rewrite all transfer logic before producing visible progress.

Build a small native foundation and a real, read-only Outfitter slice early. Establish baseline characterization before extracting any mechanics. Add working transactions through existing handlers; make exact previews depend on a tested shared evaluation path. Complete Outfitter before expanding the production migration. Use Shipyard as the second screen to validate reuse.

Keep one implementation slice active and, at most, the next screen family in detailed design. Later phases have scope and gates now; their exact patches are refined after earlier phases provide evidence.

## 4. Architectural direction

### Retain the runtime and native ownership model

The pinned project uses C++20, SDL2, and OpenGL-related build paths. Keep those as the production baseline. React/Vite remain design tools, not native runtime dependencies. No embedded browser or replacement engine is justified by this roadmap. [R4]

Keep `UI`/`Panel` integration and existing game-owned state. Evaluate and adapt `Edit`, `Dropdown`, `Table`, scrolling, tooltips, and drawing helpers rather than automatically replacing them. `ShopPanel` exposes useful shared state and virtual entry points, but its internal layout/navigation helpers also include private implementation details. An Outfitter override is possible; it is not a zero-effort reskin. [R5]

### Preserve data-defined interfaces

`Interface` already loads UI definitions from game data and renders them using `Information`. Upstream documentation also describes named interface overrides in plugins. A new hardcoded C++ layer must not simply bypass this extension surface. [R7, R10, R11]

Use namespaced, additive theme values first. Preserve existing interface identifiers, named points/boxes, color overrides, and fallback behavior where relied upon. Audit actual plugin-dependent hooks before changing them. Content-plugin compatibility and UI-override compatibility are separate test categories; neither is guaranteed by merely loading the plugin.

### Keep four responsibilities distinct

1. **Presentation:** native widgets, bounds, drawing, focus, scrolling, and feedback.
2. **View state:** query, selection, sort, visible columns, expanded sections, and scroll offsets.
3. **Game-data adapters:** read the current permitted data and provide correctly formatted values.
4. **Gameplay operations:** validate and execute supported game actions through authoritative native rules.

View state is not a second persistent copy of the game. Rendering/filtering/hover must not mutate gameplay state. A scratch preview context must isolate mutable data and shared references; a shallow copy of `PlayerInfo` or `Ship` is not assumed safe.

### Build components on demand

Initial scope: theme access, basic layout helpers, bounds/clipping conventions, list/table selection, input/focus integration, and the controls required by Outfitter. Avoid a general layout language, new event bus, replacement focus manager, universal transaction framework, or mandatory component class for every visual group.

Extract a screen-specific pattern into a reusable component when another real screen needs it. New code should document ownership, invalidation, invariants, units, and non-obvious compatibility decisions.

## 5. Milestone overview

M0 is complete as a baseline and implementation contract; M1–M9 remain planned. Completion means the M0 handoff criteria were met, not that later transaction or visual parity has been proven.

| ID | Milestone | Main result | Dependency | Main risk |
|---|---|---|---|---|
| M0 | Baseline and implementation contract | Reproducible legacy baseline, behavior inventory, design rules | None | Missing behavior or unverified environment |
| M1 | Minimal foundation and native visual slice | Read-only Outfitter using real game data | M0 | Input, clipping, typography, data visibility |
| M2 | Production Outfitter | Full legacy capability plus verified modern presentation/previews | M1; characterization before extraction | Transfer parity and fleet behavior |
| M3 | Shipyard and reuse validation | Second complete screen using shared pieces | M2 | Ownership/sale side effects; premature abstraction |
| M4 | Landed services and navigation | Coherent port, trading, bank, and hiring workflows | M3 | Mission hooks and navigation lifecycle |
| M5 | Information and narrative | Ship/fleet, player, missions, logs, conversations, help | M3; coordinate shell with M4 | Hidden information, callbacks, text volume |
| M6 | Map and navigation presentation | Consistent map modes and side panels | M4–M5 patterns stable | Route/knowledge semantics; performance |
| M7 | Menus and application lifecycle | Main menu, start/load, settings, loading surfaces | Shared controls stable | Save/input/settings compatibility |
| M8 | Flight HUD and contextual encounters | Modern live-flight presentation and contextual panels | Map and modal/focus contracts proven | Command leakage, pause behavior, frame budget |
| M9 | Whole-game release and cleanup | Consistent, compatible modernization release | All screen families accepted | Mixed-state regressions and premature deletion |

M4/M5 and M6/M7 are not rigidly serial when dependencies permit. With one implementer, finish one bounded deliverable before starting another.

## 6. M0 — Baseline and implementation contract

### M0-A: Establish the reference build

Confirm the actual development OS, compiler, toolchain, and available test hardware from the working environment. Do not infer them from a chat client. Record the native baseline commit and actual build commands.

Build the unchanged game, enumerate tests, and run the applicable existing unit/integration suites. Use an isolated configuration directory and disposable pilots. Record failures as pre-existing only when reproduced against the unchanged baseline. A configured but unexecuted test suite is not a pass.

The repository already defines a Catch2 unit target, CTest integration, and integration scenarios. Reuse this infrastructure rather than creating a disconnected test runner. Existing tests include shop-triggered missions, saving/loading, ship sales, and storage-related behavior; they are starting points, not proof of complete outfitter coverage. [R8, R9]

**Exit:** reproducible build/test evidence, environment details, and known baseline failures.

### M0-B: Inventory screens and behavior

For each surface record its source files, entry and exit points, actions, shortcuts, state mutations, mission/condition hooks, pause behavior, save interactions, data-defined interface hooks, and existing tests.

Include primary screens and secondary surfaces: confirmations, tutorials/help, mission conversations, hails, boarding, notifications, launch warnings, overlays, settings pages, and embedded plugin-related controls where present. Inventory UI surfaces, not merely classes named `Panel`.

Create representative fixtures: early pilot, developed single ship, mixed fleet, large catalog, limited stock, license-required equipment, linked outfits, full cargo, unusual capacities, unavailable services, and plugin-defined categories/attributes.

**Exit:** every known surface assigned to a migration milestone; important behavior has an evidence/test strategy.

### M0-C: Define design and screen contracts

Extract semantic colors, typography roles, spacing, borders, focus/selection, table density, alerts, motion, and responsive priorities from the prototype. Do not promise exact font-family support before testing the native font path.

Define wide/standard/compact behavior using effective logical width **and height**, UI zoom, content length, and font metrics—not browser pixel breakpoints alone. Final supported dimensions and performance budgets are recorded after the reference build is measured.

Resolve Outfitter-specific contracts: multi-ship selection; quantity semantics; fleet cargo versus selected-ship capacity; availability/ownership; explicit routes versus legacy shortcut fallbacks; category-specific columns; full descriptions; and what happens when no ships or items are selected. Source tabs must not silently remove useful combined ownership visibility.

**Exit:** a small implementable design-system v0.1 and Outfitter acceptance checklist. Do not prototype every future screen before proceeding.

## 7. M1 — Minimal foundation and a real native slice

### M1-A: Theme, geometry, and input proof

Introduce additive theme helpers and only the layout operations needed for this screen. Keep legacy screens unchanged while testing the new appearance.

Use common rectangles for drawing, clipping, hit testing, scrolling, and focus indicators. Verify scale changes, independent scroll regions, popup placement, keyboard focus restoration, and outside-click behavior. Inspect existing edit/dropdown limitations before reuse.

Test that typing shortcut letters into search/quantity never buys equipment, opens the map, or sends commands underneath the screen. Resolve Tab, Enter, Escape, key-repeat, selection, and modal focus behavior without globally remapping gameplay controls.

### M1-B: Read-only Outfitter catalog

Display the actual permitted catalog, ordered categories, thumbnails, ownership/stock context, selected-ship data, item details, search, and sorting. Start with a small column set; use category-relevant metrics rather than forcing reactor columns onto all equipment.

Selection must survive sorting/filtering where valid. Empty results must clear stale details and financial previews. Unknown or inapplicable metrics display as unavailable—not fabricated zeros. Custom categories receive a useful fallback rather than disappearing.

Keep this an opt-in development view with no new transaction execution. Preserve the existing Outfitter for gameplay. Capture screenshots from the native game, not only the browser prototype.

**M1 acceptance:** the visual concept works with real data, supported viewport/zoom cases, keyboard/mouse navigation, and long/custom content; browsing itself does not mutate gameplay state.

## 8. M2 — Complete the Outfitter

### M2-A: Connect existing operations

Connect the redesigned controls to the authoritative native transaction paths. Preserve every supported route and the meaning of legacy shortcuts, modifier quantities, ship selection/grouping, park/reorder actions, refill/help flows, and leaving the panel. Map each item to the M0 behavior inventory rather than assuming all functionality lives in the visible purchase buttons.

Where explicit source/destination controls differ from legacy shortcut fallback behavior, make the distinction visible. Do not let a preview indicate one source while a shortcut silently chooses another.

### M2-B: Characterize transactions before extracting them

Add fixture-based tests for shop/ship/cargo/storage routes, partial fulfillment, finite/unlimited stock, prices/depreciation, licenses/maps, linked outfits, crew and recharge effects, precise capacity boundaries, and single/multi-ship allocation.

Record old results before modifying the execution path. Compare meaningful gameplay fields and relevant event traces; do not rely only on rendered numbers. Guard integer parsing, overflow, invalid quantities, stale objects, and empty selections without silently changing valid quantity semantics.

### M2-C: Add an authoritative preview boundary

Proposed concepts are `OutfitTransferRequest`, `OutfitTransferPlan`, and an execution result. These are responsibilities to implement, not a finalized class schema.

A request represents the selected outfit, route, quantity interpretation, and selected ships. The evaluator produces the actual supported result using native rules, including ordered partial fulfillment and automatic side effects. Preview and commit share that rules implementation rather than separately reimplementing it.

The plan must represent actual quantity, affected ships, exact credit changes, license purchases, cargo/storage destinations, stock changes, linked removals, and warnings/failures. An execution result should report what actually happened, not only a boolean.

Before commitment, validate relevant state/preconditions. If missions, selection, stock, credits, inventory, or other prerequisites changed, refresh the preview rather than committing a materially different result invisibly. Never run the real operation and undo it to generate a preview.

The planner must not invent atomicity: if the baseline buys the subset that fits or redirects later removals into storage, the evaluator reports that same behavior. Unsupported preview cases may remain explicitly unavailable during development, with preserved legacy operations; the new screen is not production-complete until its required cases are covered.

### M2-D: Fleet and inventory presentation

Retain multi-selection, actual allocation order, group behavior, absent/parked restrictions, and supported no-ship operations. Separate selected-ship statistics from fleet cargo and local planetary storage.

For multiple ships show the selected count, eligible/affected count, requested quantity meaning, total operation quantity/cost, and inspectable per-ship changes. Aggregate ranges can supplement—not replace—the exact result when it matters.

### M2-E: Trustworthy statistics

Show direct capacity, inventory, and financial changes first. Use native numeric/formatting conventions and existing ship calculations where possible. Keep underlying calculations precise and round for display only.

Energy and heat figures must state the modeled operating scenario. Generated heat is not temperature. A positive energy balance under selected assumptions is not a guarantee of flight/combat viability. Reuse existing checks and do not add a second flight simulator for previewing outfits.

For every displayed metric record its native source, units, permitted visibility, scenario assumptions, invalidation triggers, and handling of missing/zero denominators.

### M2-F: Stabilize and accept

Finish compact layouts, full descriptions, keyboard-only flows, failure feedback, column persistence, large-content performance, and modal interactions. Use an opt-in setting or equivalent development switch until parity is demonstrated.

**M2 acceptance:** all supported legacy capabilities remain reachable; baseline comparisons pass; preview/commit agree; no new save schema or plugin semantic change; tested fallback exists; native visuals and interaction are accepted. This is the first production-quality modernized screen.

## 9. M3 — Shipyard: prove reuse

Apply the same visual language and interaction conventions to Shipyard without forcing it into Outfitter-specific rules. Reuse catalog/list mechanics, search, selected-item details, fleet selection presentation, theme, and layout only where they genuinely fit.

Preserve purchase/sale pricing, ownership transfer, flagship behavior, cargo/crew consequences, model variants, licenses, mission hooks, and other characterized shipyard operations. Do not force ship sales into an outfit-transfer abstraction.

Refine shared code only after seeing the requirements of both screens. Add no abstraction solely because a future screen might use it.

**Acceptance:** both Outfitter and Shipyard pass regression tests; shared changes improve reuse without converting `ShopPanel` into a framework rewrite.

## 10. M4 — Landed services and navigation

Convert Trading, Bank, Hiring, the landed/planet shell, and spaceport-service navigation in small complete slices. Align service availability, headings, back behavior, notices, and shared transaction presentation.

Introduce common confirmation/dialog styling as soon as migrated screens require it. Do not postpone basic modal consistency until the late menu phase, and do not globally replace every dialog at once.

Preserve mission-offer hooks, service restrictions, day/time behavior, takeoff checks, refills, cargo handling, and callbacks. A new tab must not create extra service-entry events or bypass the native service lifecycle. Shop mission-trigger integration tests are directly relevant here. [R9]

**Acceptance:** the complete land → service → trade/outfit → return → depart loop preserves gameplay outcomes and event ordering.

## 11. M5 — Information and narrative

Modernize ship/fleet information, player information, cargo displays, mission/job lists and detail views, logs/messages, conversations, tutorials, and help. Use lists/tables for comparison and readable text layouts for narrative; visual consistency does not require identical layouts.

Preserve all actions and condition-driven choices. Sorting/filtering must not change the mission being accepted or abandoned. Long descriptions must remain fully reachable. Do not editorially rewrite story text or expose hidden choices, undiscovered content, or internal conditions.

**Acceptance:** large fleets, long logs, and branch-heavy conversations are usable; mission callbacks/choices and information availability match baseline; keyboard/mouse users can reach the full content.

## 12. M6 — Map and navigation presentation

Prototype and migrate map chrome separately from the map canvas: search/filters, side-panel information, route summary, then overlays/modes. Preserve existing route calculations and knowledge rules.

Validate panning, zoom, selection, waypoint/route interaction, mission links, market/outfitter/shipyard views, and returning to the correct previous screen. Hovering or browsing must not mark systems visited or expose unavailable data.

Use visible-region drawing and appropriate cache invalidation for dense maps. Establish measured performance against the same baseline scenario.

**Acceptance:** equivalent destination/route actions yield the same route and relevant gameplay state; undiscovered information stays hidden; all existing map modes remain usable.

## 13. M7 — Menus and application lifecycle

Convert main menu, starting conditions, loading/continue flow, preferences, control binding, gamerule presentation, and applicable plugin/settings surfaces identified by M0. This milestone finishes application-level consistency; it is not the first time common dialogs are modernized.

Preserve existing save discovery/load behavior, selected pilot identity, settings meanings, binding defaults, start semantics, and error recovery. UI preferences must not alter gameplay saves or require a migration. Preserve existing persisted UI state where required; any new preference serialization needs a compatibility test.

**Acceptance:** new and existing pilots, interrupted/cancelled flows, settings round trips, remapping, and malformed/missing-file cases behave as documented without destructive side effects.

## 14. M8 — Flight HUD and contextual encounters

Use a dedicated HUD design, not a menu overlaid on space. Modernize flight instruments, target/weapon/escort presentation, radar/minimap, messages, and applicable hail/boarding/contextual surfaces.

Do not change targeting, firing, AI, capture rules, routing, or simulation timing. Preserve input capture, pause/unpause semantics, time acceleration, key-repeat behavior, and priority of critical warnings. New transitions must not delay a gameplay command or obscure an urgent state change.

Validate command/event traces for equivalent actions, then test real-flight and combat scenarios at the agreed performance/viewport targets. Protect the render/simulation boundary; a cosmetic animation must not drive the game clock.

**Acceptance:** command behavior and game state remain equivalent; no leaked/dropped controls; critical data remains recognizable; frame/input performance stays inside the measured budget.

## 15. M9 — Whole-game stabilization and release

Run complete journeys using representative pilots and plugins: start/load, land, manage ship/fleet, buy/sell/install, accept and complete missions, navigate, fly/fight, save, quit, and reload.

Perform cross-platform testing on the supported release targets, native screenshot review across screen families, performance comparisons, information-visibility checks, input/accessibility review, and compatibility round trips against the selected baseline.

Publish a screen-coverage report and known limitations. Remove duplicated UI paths only after there is acceptance evidence and a recoverable release/commit. Do not delete legacy data-defined interface contracts merely because the default appearance changed.

**Acceptance:** every inventoried surface is modernized or has an explicitly approved disposition; no unexplained gameplay regression remains; release packages contain the required assets and do not require Node/React; saves and agreed plugin classes pass their compatibility tests.

## 16. Definition of done for every patch

| Gate | Required evidence |
|---|---|
| Behavior | Characterized native operations still produce equivalent supported outcomes and relevant events. |
| Interaction | Mouse and keyboard flows work; focus, repeats, modal escape/back, and pointer bounds are tested. |
| Information | Values, units, visibility/knowledge restrictions, partial results, and warnings are truthful. |
| Layout | Agreed width/height/zoom cases and long/custom content work without inaccessible controls. |
| Compatibility | Save semantics, content plugins, and UI overrides affected by the patch are checked. |
| Performance | Same-scenario timing/memory measurements meet the agreed budget; expensive work is invalidated appropriately. |
| Maintainability | New files are registered in build/test manifests; ownership/invariants and limitations are documented. |
| Delivery | Commit/ref, changed files, actual test output, screenshots when relevant, failures, and fallback are recorded. |

Blocked or unrun tests are reported explicitly. A screenshot cannot establish transaction parity; a passing unit test cannot establish usability. Native screen-reader integration and certification are not assumed: core keyboard, contrast, scaling, and non-color feedback requirements are part of the work, while platform-assistive integration requires a separate verified capability assessment.

## 17. Work packaging and change control

Keep the prototype commit pinned as a reference. Use short-lived feature branches from the current validated fork baseline; illustrative names are `ui/m0-baseline`, `ui/theme-layout`, and `ui/outfitter-catalog`. Merge tested increments to the fork's existing integration/default branch; do not rename branches or create remotes as an undocumented side effect.

Keep upstream syncs separate from UI patches. At each sync record the old/new baseline, review upstream behavior changes, and rerun affected characterization tests. Do not compare gameplay against a moving target without explaining the change.

Keep production and legacy rendering switchable at a safe screen boundary during migration where practical. Both should use the same authoritative gameplay path after extraction; retain pinned pre-refactor results to prevent a shared mistake from making both sides appear correct. Never switch UI modes mid-transaction.

A compact repository planning set is sufficient:

- `docs/ui/ROADMAP.md`: scope, milestones, gates, and significant decisions.
- `docs/ui/DESIGN-SYSTEM.md`: implemented visual and interaction rules.
- `docs/ui/PARITY.md`: behavior matrix, fixtures, compatibility and test coverage.
- `docs/ui/STATUS.md`: verified commit, current patch, evidence, blockers, next action.

These four documentation files were created in the detached native-baseline M0 worktree and copied to `docs/ui` in the `ui-prototype` checkout at the user's request. Keep patch-specific handoffs short and reference these sources rather than maintaining competing versions of the plan.

Each handoff states: exact starting commit, goal, non-goals, allowed change area, dependencies, acceptance tests, expected evidence, and stop/report conditions. It may not authorize the next milestone implicitly.

## 18. Decisions and unknowns to close before their implementation gate

| Question | Resolve by | Default direction |
|---|---|---|
| Primary development/test platform and hardware | M0-A | Inspect actual environment; preserve existing cross-platform design. |
| Native minimum window and zoom/font targets | M0-C/M1 | Measure baseline, then define width-and-height fixtures. |
| Typography/icon asset pipeline | M1-A | Reuse supported native paths; preserve provenance; avoid a font-engine rewrite. |
| UI-override plugin hooks | M0-B, then each affected screen | Audit named interface/color/box contracts; test overrides independently. |
| Exact evaluator/executor API and scratch-state strategy | M2-B/C | Shared native rules with isolated preview state; no double implementation. |
| Multi-ship preview presentation | M0-C; validate M2-D | Preserve allocation semantics; expose total and per-ship effects. |
| Preference persistence | Before M2-F | Use compatible UI preference storage; no new gameplay-save schema. |
| Release budget and cadence | After M2/M3 evidence | Estimate from actual delivery/review data, not invented dates. |

No implementation duration is committed here. M2, M6, and M8 deserve the largest contingency because their main risks are behavior, information access, and input/performance respectively.

## 19. Immediate execution boundary

M0 is complete. The next implementation package is **M1-A: additive theme/layout/input proof**, followed by **M1-B: a real read-only Outfitter catalog**. Do not start with a general UI framework or a complete transaction rewrite.

The first visible native target is **M1: a real, read-only Outfitter catalog**. The first production feature is **M2: a fully functional Outfitter with verified gameplay parity**. The second production feature is **M3: Shipyard**, which validates reuse before the whole-game migration accelerates.

The program is successful when the player experiences a coherent modern interface while the game underneath still behaves as Endless Sky.

## Source references

Repository facts above are tied to the inspected revisions, not an assumed latest upstream version. Source references are evidence; future implementation must reread the exact approved starting commit.

- **R1 — Native branch check:** `https://api.github.com/repos/killerbearddude/endless-sky/branches/master` (checked September 22, 2026; resolved to `902bca70c3fa8bbb57a9349c7d372cdfcf06adc4`).
- **R2 — Prototype branch check:** `https://api.github.com/repos/killerbearddude/endless-sky/git/ref/heads/ui-prototype` (checked September 22, 2026; resolved to `ed44a8a929e79651b147c8e49e5ab8fa2e32340e`).
- **R3 — Prototype scope:** `https://github.com/killerbearddude/endless-sky/blob/ed44a8a929e79651b147c8e49e5ab8fa2e32340e/prototypes/outfitter/README.md`.
- **R4 — Native build/toolchain:** `https://github.com/killerbearddude/endless-sky/blob/902bca70c3fa8bbb57a9349c7d372cdfcf06adc4/CMakeLists.txt`, especially language standard, dependencies, and CTest setup.
- **R5 — Shared shop UI:** `https://github.com/killerbearddude/endless-sky/blob/902bca70c3fa8bbb57a9349c7d372cdfcf06adc4/source/ShopPanel.h#L45-L245`.
- **R6 — Native transfer behavior:** `https://github.com/killerbearddude/endless-sky/blob/902bca70c3fa8bbb57a9349c7d372cdfcf06adc4/source/OutfitterPanel.cpp#L709-L984`.
- **R7 — Data-defined UI:** `https://github.com/killerbearddude/endless-sky/blob/902bca70c3fa8bbb57a9349c7d372cdfcf06adc4/source/Interface.h#L40-L215`.
- **R8 — Existing test registration:** `https://github.com/killerbearddude/endless-sky/blob/902bca70c3fa8bbb57a9349c7d372cdfcf06adc4/tests/CMakeLists.txt`.
- **R9 — Shop mission lifecycle tests:** `https://github.com/killerbearddude/endless-sky/blob/902bca70c3fa8bbb57a9349c7d372cdfcf06adc4/tests/integration/config/plugins/integration-tests/data/tests/tests_shipyard_outfitter_missions.txt`.
- **R10 — Official interface documentation:** `https://github.com/endless-sky/endless-sky/wiki/CreatingInterfaces/1bd733ec2b3ee1ba246e30565012846e7991d2c5` (named-interface replacement and anchored points).
- **R11 — Official plugin documentation:** `https://github.com/endless-sky/endless-sky/wiki/CreatingPlugins` (consulted September 22, 2026; lists interface/color definitions and plugin data surfaces).
- **R12 — Screen/source inventory starting point:** `https://github.com/killerbearddude/endless-sky/blob/902bca70c3fa8bbb57a9349c7d372cdfcf06adc4/source/CMakeLists.txt`.
