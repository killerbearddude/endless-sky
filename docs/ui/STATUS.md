# UI modernization: M0 status

Updated 2026-09-22. **M0 complete as a baseline and implementation contract.** The unchanged native reference builds, its applicable baseline tests pass, every known screen family is assigned and source-inventoried, and the initial Outfitter parity and native design contracts are recorded. Patch-specific characterization and visual acceptance remain for M1 and later milestones.

## Reference and checkout

| Item | Observed |
| --- | --- |
| Original checkout | `/home/daniel/Dev/endless-sky`, clean `ui-prototype` at `ed44a8a929e79651b147c8e49e5ab8fa2e32340e` |
| Native reference | local `master` at `902bca70c3fa8bbb57a9349c7d372cdfcf06adc4` |
| M0 worktree | `/tmp/endless-sky-m0-902bca7`, detached at the exact native reference; original checkout untouched |
| Remotes | `origin` is `killerbearddude/endless-sky`; `upstream` is `endless-sky/endless-sky` |
| End state / changes | At M0 closure the original checkout was clean on `ui-prototype` at `ed44a8a929e79651b147c8e49e5ab8fa2e32340e`; the detached M0 worktree remained at `902bca70c3fa8bbb57a9349c7d372cdfcf06adc4`. The four `docs/ui` files are now also saved as untracked files in `/home/daniel/Dev/endless-sky/docs/ui` on `ui-prototype` at the user's request. No production C++, gameplay data, save, dependency manifest, CI or plugin-loading change; no commit made. |

The supplied handoff is an execution brief. The supplied roadmap is preserved in `ROADMAP.md` with only evidence-based M0 status reconciliation. At M0, the approved palette specification had not yet been rendered natively; M1-A below implements its semantic roles while retaining existing interface definitions.

## Native environment and build

Linux Mint 22.1, kernel 6.8.0-139-generic, x86_64; GCC 13.3.0, CMake 3.28.3, Ninja 1.11.1, 12 logical CPUs. The machine has an X display and NVIDIA GeForce RTX 2070 SUPER with OpenGL 4.6 (`glxinfo -B`). The repository uses C++20 and SDL2. `README.md` points to `docs/readme-developer.md` for package/build instructions.

The first configure attempt stopped at `CMakeLists.txt:69`: `find_package(libavif REQUIRED)` could not find a libavif package config. Because sudo required a password, approved missing packages were downloaded from the configured Ubuntu Noble repositories and unpacked under `/tmp/endless-sky-m0-deps/usr`, without changing system packages. The local set comprises development packages `libavif-dev`, `libflac++-dev`, `libflac-dev`, `libminizip-dev`, `libopenal-dev`, `libmad0-dev`, `catch2`, plus their needed runtime packages `libavif16`, `libmad0`, `libminizip1t64`, `libflac++10`, `libflac12t64`, `libopenal1`, `libgav1-1`, and `libyuv0`. The local `.pc` files had their `prefix` changed from `/usr` to the extracted `/tmp` prefix. Other listed development dependencies were already installed.

The local package preparation actually used `apt-get download` in `/tmp/endless-sky-m0-debs` for the packages listed above, then `dpkg-deb -x` for each package into `/tmp/endless-sky-m0-deps`. The extracted `*.pc` files under `usr/lib/x86_64-linux-gnu/pkgconfig` had only their `prefix=/usr` line changed to `prefix=/tmp/endless-sky-m0-deps/usr`; no repository file or system package was edited.

The successful unchanged-source configuration and build used:

```sh
# From /tmp/endless-sky-m0-902bca7:
mkdir -p build  # CMake unconditionally writes an ignored build/compile_commands.json symlink.
PKG_CONFIG_PATH=/tmp/endless-sky-m0-deps/usr/lib/x86_64-linux-gnu \
  cmake -S . -B /tmp/endless-sky-m0-build-local -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DES_USE_VCPKG=OFF \
  -DES_USE_SYSTEM_LIBRARIES=ON -DBUILD_TESTING=ON -DES_USE_OFFSCREEN=ON \
  -DCMAKE_PREFIX_PATH=/tmp/endless-sky-m0-deps/usr
LD_LIBRARY_PATH=/tmp/endless-sky-m0-deps/usr/lib/x86_64-linux-gnu \
LIBRARY_PATH=/tmp/endless-sky-m0-deps/usr/lib/x86_64-linux-gnu \
  cmake --build /tmp/endless-sky-m0-build-local -j 4
```

Result: CMake configured and linked `endless-sky` and `endless-sky-tests`. The first link attempt needed `LIBRARY_PATH` for the locally unpacked `-lminizip`; the next needed the local runtime `libgav1` and `libyuv` dependencies of libavif. After adding those, the full build completed. CMake emitted only a deprecation warning from the packaged OpenAL config on the successful configure.

## Test registration, execution, and isolation

`tests/CMakeLists.txt` registers Catch2 `unit`, Catch2 `[!benchmark]`, and integration tests discovered by running the game with `--tests`. There are 34 files under `tests/unit/src` and 22 integration data files in its CMake manifest. Successful discovery registered **92 CTest entries**: 45 normal integration scenarios, their 45 `[debug]` counterparts, one unit target, and one benchmark target.

The out-of-source binary could not initially auto-find `credits.txt`/resources during integration discovery, so the generated list was empty. Symlinks for `credits.txt`, `data`, `images`, and `sounds` were placed only in `/tmp/endless-sky-m0-build-local`, then `IntegrationTests` was regenerated with `LD_LIBRARY_PATH` pointing to the local library prefix. These are disposable build-tree links, not source changes. Normal integration execution itself passes `--resources` pointing to the native source tree. `source/Files.cpp::Init` supports `--config`; `tests/integration/RunIntegrationTest.cmake` copies the checked-in fixture to a per-test config directory under the build tree and invokes the game with that directory. No live pilot/save directory was used.

| Group | Actual command (with local `LD_LIBRARY_PATH`) | Result |
| --- | --- | --- |
| Unit | `ctest --test-dir /tmp/endless-sky-m0-build-local --output-on-failure -L '^unit$'` | 1/1 passed, 0.42 s |
| Normal integration | `ctest --test-dir /tmp/endless-sky-m0-build-local --output-on-failure -L '^integration$' -j 1` | 45/45 passed, 1014.39 s |
| Benchmark | `ctest --test-dir /tmp/endless-sky-m0-build-local --output-on-failure -L '^benchmark$'` | 1/1 passed, 46.60 s |
| Debug integration | Registered with `integration-debug` label | 0/45 run; these are interactive/debug variants, not the normal baseline suite |

An initial `-j 4` integration attempt produced fixture-copy errors: the checked-in runner copies via a shared `integration_configs/config` path before renaming it, so concurrent runs collide. That attempt was stopped; the disposable integration config tree was cleared, and the full normal group then passed serially. The parallel errors are runner contention, not reproduced gameplay failures. Do not run this integration target in parallel without first changing its fixture setup in a separate authorized patch.

## Native visual evidence

A normal native launch was made on an isolated Xvfb display (1280x800) with `--resources /tmp/endless-sky-m0-902bca7` and disposable `--config /tmp/endless-sky-m0-visual-config`. The captured menu screenshot at `/tmp/endless-sky-m0-menu.png` visibly shows the main menu with no pilot loaded. The process was stopped after capture. The virtual display logged unsupported VSync/swap interval and libpng profile warnings; the menu rendered. This does not verify the Outfitter, other window sizes, zoom levels, or keyboard/mouse workflows.

## M0 acceptance and downstream limits

| Handoff criterion | M0 evidence |
| --- | --- |
| Exact reference/environment; executed build/test outcomes separated from unrun work | Native SHA and toolchain above; successful build, 1 unit, 45 normal integration and 1 benchmark CTest target passed; 45 debug integration variants registered but unrun. |
| Live saves protected | `--config` points to copied per-test configurations or the disposable visual configuration; no live pilot/save path was opened. |
| Screen, action, mission/navigation and UI-extension scope inventoried | `PARITY.md` maps every known surface to M2–M9 and records entry/exit, controls/modifiers, state/mission hooks, save/time boundary and existing test evidence. `DESIGN-SYSTEM.md` identifies named interface/color/point/box and font/sprite override paths. |
| Initial Outfitter parity and fixtures | `PARITY.md` records all supported source/destination routes, quantity/fallback/allocation rules, non-transaction capabilities, major edge cases, and fixture assertions. New fixtures are specified, not claimed as executed. |
| Small native M1 contract | `DESIGN-SYSTEM.md` specifies semantic roles, focus/selection, compact priorities, ownership/quantity/metric presentation, and reuse/adapt/new capability decisions without introducing a general framework. |
| No unauthorized production changes | Only four documentation files were added in the detached native worktree; no production or gameplay file changed. |

**Known baseline result:** no failure in the completed serial baseline suite. The initial missing-libavif configure, out-of-source integration discovery, and parallel fixture-copy errors were environment/runner setup failures; their resolutions are recorded above. The 1280×800 screenshot proves only the native main menu. The normal suite does not prove every transfer route, event sequence, UI override, viewport, or input workflow.

**Recorded unknowns for later gates:** exact Outfitter transaction event/save traces and partial allocation outcomes; the strict cargo exact-fit branch and quantity parsing defect candidates; per-screen visual, focus and pause/tick measurements; custom-category/content-plugin and named UI-override-plugin regression outcomes. The matrix identifies each owner milestone. These are explicit M1/M2-and-later validation tasks, not evidence silently assumed at M0 closure.

**Smallest next patch:** **M1-A: additive theme/layout/input proof** at the native screen boundary, retaining legacy gameplay and interface names. Then **M1-B: real read-only Outfitter catalog**. Neither was executed in M0. Before any M1 source-changing patch, choose a native-baseline branch and carry the reviewed `docs/ui` files from the current `ui-prototype` checkout or detached M0 worktree into it. The present documentation location was requested by the user; no commit or branch switch was made.

## M1-A implementation and validation — October 6, 2026

The owner selected current `endless-sky/endless-sky:master` after it advanced 56 commits beyond the M0 reference. M1-A starts at `fa22e46ae92b382509b4076caa859e36e6c16fa7` on `codex/m1a-theme-layout-input-proof`; the older SHA above remains the M0 historical reference. The production checkout on `ui-prototype` and its untracked M0 docs were left in place; these four docs were copied into an isolated task worktree before editing.

**Implemented:** `data/_ui/interfaces.txt` adds 14 namespaced `ui/` palette colors at normalized reference sRGB values without changing legacy colors. `UIProofPanel` is an opt-in native proof surface entered with `--ui-proof`. It uses existing `Rectangle`, `Panel`, `Edit`, `UI`, `FillShader`, `LineShader`, and `FontSet` behavior. The same rectangles define button fill, border, and clickable zone, and the same `Edit::Position()` rectangles define field fill, pointer hit test, and focus border. `GameLoadingPanel` places the proof over the normal menu only for that option; integration runs ignore the option. The proof has two text fields, a labeled focus indicator, a parent B shortcut counter, a trapped popup, and labeled semantic swatches/statuses. It adds no catalog, transaction, gameplay, save, or plugin schema behavior.

**Build and automated checks:** Configured Debug with CMake/Ninja using locally unpacked Ubuntu packages under `/tmp/endless-sky-m1a-deps`, `ES_USE_VCPKG=OFF`, `ES_USE_SYSTEM_LIBRARIES=ON`, `BUILD_TESTING=ON`, and `ES_USE_OFFSCREEN=ON`. The game and unit executable built successfully. The first sandboxed unit CTest run executed all 78,615 assertions in 96 cases successfully but CTest failed when LeakSanitizer could not run under tracing; the unsandboxed `ctest --test-dir /tmp/endless-sky-m1a-build --output-on-failure -L '^unit$'` passed 1/1. After adding disposable resource symlinks in the build tree, discovery registered 92 tests. Serial integration checks `Outfitter Mission Test`, `Plugin Installed Autocondition`, and `Shipyard Mission Test` passed 3/3. The full 45-scenario normal integration suite and 45 debug variants were not run; this patch does not change shared `UI`, `Panel`, `ShopPanel`, or gameplay code. No focused automated test was added because the proof is exercised through the running native game.

**Native checks:** On a disposable Xvfb display and config directory, the proof rendered at 1280×800, at 1800×980, and at 1280×800 with 120% UI zoom. Screenshots are in `docs/ui/reference/`. At 1280×800, typing shortcut letters `bm` into the first focused field left the parent B count at zero. Tab moved focus to the second field and Shift+Tab returned it. Escape released field focus, then B incremented the parent count once. Clicking the drawn popup button opened the child panel; B was trapped while it was open, and Escape restored focus to the first field. On the final rebuilt binary, a click inside the popup left it open; a click outside closed it and restored first-field focus. With Xvfb key repeat enabled at 200 ms / 20 Hz, one held B generated 21 observed X11 keypress events and incremented the parent proof action only once. A long text entry stopped at the field's right edge with no overlap. The proof's selection/focus and status meanings have text or numbered indicators as well as color. Native captures verify this proof surface only, not Outfitter parity or an accessibility contrast audit.

**Limits:** The content-plugin integration scenario passed, but it does not exercise a custom Outfitter category on this proof surface. A dedicated UI/color override fixture was not run; the new names use the existing plugin-overridable color map, and no existing named interface or legacy color was changed. The native popup and input proof are not a replacement for M1-B's real data, scrolling, and transaction boundary checks. Xvfb reported unsupported VSync and existing image-profile warnings; rendering and input continued. The first native screenshot taken with a centered Xvfb window showed an OpenGL/root-capture offset; moving the disposable window to display origin produced correctly bounded screenshots. A separate production display and human review remain useful before accepting the palette.

## M1-A R1 correction and validation — October 6, 2026

R1 starts from reviewed M1-A commit `584f475b17ac27e4255ebdf6fb7ff44aeaaed92e` on `codex/m1a-theme-layout-input-proof`. The correction is local to `UIProofPanel.cpp`: a proof-only `Edit` subclass draws its control/focus border after `Edit::Draw()` has filled the field. Its `Position()` is the single field rectangle for the Edit surface, pointer hit test, and final border. The parent retains the numbered focus indicator, so focus is not conveyed by color alone. Shared `Edit`, `Panel`, `UI`, startup, ShopPanel, gameplay, palette, and plugin code are unchanged by R1.

The corrected Debug build passed with:

```sh
LD_LIBRARY_PATH=/tmp/endless-sky-m1a-deps/usr/lib/x86_64-linux-gnu \
LIBRARY_PATH=/tmp/endless-sky-m1a-deps/usr/lib/x86_64-linux-gnu \
  cmake --build /tmp/endless-sky-m1a-build -j 4
```

The unit target passed **1/1** using `LD_LIBRARY_PATH=/tmp/endless-sky-m1a-deps/usr/lib/x86_64-linux-gnu ctest --test-dir /tmp/endless-sky-m1a-build --output-on-failure -L '^unit$'`. The current corrected build discovered **45 normal integration scenarios**. The complete group passed **45/45, 0 failed** in 1022.27 seconds with:

```sh
LD_LIBRARY_PATH=/tmp/endless-sky-m1a-deps/usr/lib/x86_64-linux-gnu \
  ctest --test-dir /tmp/endless-sky-m1a-build --output-on-failure -L '^integration$' -j 1
```

The native proof was relaunched with `--ui-proof`, source resources, and a disposable config. Replacement captures under `docs/ui/reference/` show the final focus border and inactive border at 1280×800, 1800×980, and 1280×800 with 120% UI zoom. The borders stay aligned with their fields; center-pixel samples from all 14 palette swatches match the prior 1280×800 capture. At 1280×800, clicks just inside and outside the first field's left edge respectively set and released focus. Typing `bm` in the focused field did not increment the parent B count; Tab, Shift+Tab, Escape, popup B trapping and dismissal, and focus restoration passed. With Xvfb repeat at 200 ms / 20 Hz, one held B generated 21 observed X11 keypress events and exactly one additional parent proof action. These checks demonstrate the opt-in proof surface, not human palette acceptance or gameplay parity.

No R1-specific automated test was added. The 45 debug/interactive integration variants and benchmark target were not run for R1. M0 results above remain historical evidence from the older baseline; R1 does not reinterpret them. The existing M1-B and later-milestone unknowns remain separate from this correction.

## M1-B read-only Outfitter catalog — October 6, 2026

M1-B starts from accepted M1-A commit `00f25fad67d49b6eba0cbc4ec2308bcfe981b86c` on the isolated `codex/m1b-readonly-outfitter-catalog` branch. The original `ui-prototype` checkout and its untracked `docs/ui` files were not edited. While landed at a planet with an Outfitter, Ctrl+Shift+O opens `ReadOnlyOutfitterPanel`; ordinary O continues to open the legacy transactional Outfitter. The M1-B panel never enters `ShopPanel`, `OutfitterPanel`, their mission/refill paths, or transaction handlers.

The panel reads the planet's merged outfitter sale set, player stock, fleet cargo, the existing current-planet storage entry, outfits installed on ships at the current planet, and already-held license outfits that the legacy visibility rule exposes. It uses the native outfit category precedence, then adds visible custom categories by name. The view owns source/category/search/sort/selected-outfit/selected-ship/scroll state. Name and base-cost sorts preserve selection by outfit pointer; empty results clear details. The detail pane uses the native `OutfitInfoDisplay` for requirements and attributes, with an explicitly labeled base price. A local “all here” selection shows selected-ship count and capacity ranges where ships differ; cargo and storage remain separate. The panel provides wide/standard catalog plus details and a compact catalog-to-details transition. It does not show a transaction price or Current → After preview.

**Executed validation:** Debug CMake/Ninja build passed using the same locally unpacked dependencies as M1-A. The unit target passed 1/1 on final source outside sandbox tracing; 78,613 assertions passed. The new `M1B Read Only Outfitter` integration scenario passed 1/1 on final source, then passed again within the serial normal integration batches recorded below. Initial sandboxed CTest attempts executed assertions but could not complete LeakSanitizer under tracing; unsandboxed runs are the reported test results.

**Native checks:** An isolated Xvfb display and disposable pilots produced captures in `docs/ui/reference/` at 1280×800, 1800×980, 1280×800 with 120% UI zoom, and compact 960×680. Mouse category and row selection, keyboard Up/Down through a ten-row compact Power catalog with the last row kept visible, Left/Right, source brackets, S price sorting with Hyperdrive selection retained, focused `bm` search, clearing search, empty-state detail clearing, compact detail transition, and detail scrolling were inspected in the running game. The first live-search capture exposed a callback-order bug; it was fixed and retested on the rebuilt binary. The compact Back button initially covered the item title; final-binary compact evidence shows both. Search focus consumes shortcut letters; Tab exits search focus, the first Escape releases it, and the second closes M1-B. Ordinary O was exercised after closing M1-B and opened the unchanged legacy Outfitter. The focused integration scenario checks Ctrl+Shift+O, Up/Down, Left/Right, source brackets, S/H, B/M, Escape, and unchanged credits, flagship crew, ship count, flagship Hyperdrive, cargo Hyperdrive, and storage Hyperdrive.

**Plugin fixtures:** `docs/ui/fixtures/m1b-content` adds `M1B Survey Resonator`, a 188-character second outfit name, `M1B Field Equipment`, and a nonstandard field-resonance attribute to a local outfitter. Native 120% captures show the custom category, both items, middle-truncated long row/title without price or Back overlap, thumbnail, description, and scrolled generic attribute. Searching `m1b` retained both plugin outfits; switching to price sort reordered them while the selected long outfit kept its identity. A separate disposable pilot with an owned, locally unsold Navy license showed the Licenses category, “LICENSE HELD” row, and held-license detail label. Another disposable pilot showed Hyperdrive counts of 1 installed, 2 in fleet cargo, 3 in current-planet storage, and 4 in player stock; the Cargo filter retained all four quantities in the details pane. The separate `m1b-ui-override` fixture overrides `ui/focus`; its native capture shows the changed selection and focus outlines. The M1-B patch does not change existing named interfaces or legacy colors. Fixture directories were copied into separate disposable configs with each `recent.txt` pointing to its own save; the final native compatibility captures used those isolated configs.

**Read-only evidence and limits:** The new panel holds `const PlayerInfo &` and reads local storage from `PlanetaryStorage()` rather than the mutable `Storage()` accessor. The focused integration test asserts the state listed above. Comparing a fresh disposable pilot save before and after native browsing showed only the normal `playtime` field changing; equipment, cargo, stock, credits, conditions, ship order/parking, and other serialized fields did not differ in that inspected fixture. This does not prove every possible mission, license, map, or multi-ship state. No transaction execution was added. Native Xvfb captures are automated visual evidence; production-display and human usability review remain open. The 46 debug/interactive integration variants and benchmark target were not run.

**Final-source commands and results:**

```sh
mkdir -p build
PKG_CONFIG_PATH=/tmp/endless-sky-m1a-deps/usr/lib/x86_64-linux-gnu \
  cmake -S . -B /tmp/endless-sky-m1b-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DES_USE_VCPKG=OFF \
  -DES_USE_SYSTEM_LIBRARIES=ON -DBUILD_TESTING=ON \
  -DES_USE_OFFSCREEN=ON -DCMAKE_PREFIX_PATH=/tmp/endless-sky-m1a-deps/usr
LD_LIBRARY_PATH=/tmp/endless-sky-m1a-deps/usr/lib/x86_64-linux-gnu \
LIBRARY_PATH=/tmp/endless-sky-m1a-deps/usr/lib/x86_64-linux-gnu \
  cmake --build /tmp/endless-sky-m1b-build -j 4
LD_LIBRARY_PATH=/tmp/endless-sky-m1a-deps/usr/lib/x86_64-linux-gnu \
  ctest --test-dir /tmp/endless-sky-m1b-build --output-on-failure -L '^unit$'
export LD_LIBRARY_PATH=/tmp/endless-sky-m1a-deps/usr/lib/x86_64-linux-gnu
# Run each normal integration batch serially:
ctest --test-dir /tmp/endless-sky-m1b-build --output-on-failure -L '^integration$' -I 1,12 -j 1
ctest --test-dir /tmp/endless-sky-m1b-build --output-on-failure -L '^integration$' -I 13,24 -j 1
ctest --test-dir /tmp/endless-sky-m1b-build --output-on-failure -L '^integration$' -I 25,36 -j 1
ctest --test-dir /tmp/endless-sky-m1b-build --output-on-failure -L '^integration$' -I 37,46 -j 1
```

The final Debug game and unit executables linked successfully. The final unit target passed 1/1 (78,613 assertions). The four final-source serial normal integration batches passed **12/12, 12/12, 12/12, and 10/10: 46/46 total, zero failed**. These batches include `M1B Read Only Outfitter`, `Outfitter Mission Test`, `Shipyard Mission Test`, and `Plugin Installed Autocondition`. One earlier unbounded serial run was terminated with exit 143 after 20 passes and no reported test failure; it is not counted as the final result. The 46 `[debug]` interactive variants and benchmark target were registered but not run. The native captures and disposable save audit above are automated/Xvfb evidence, not production-display or human usability acceptance.

## M1-B R1 pricing and focus correction — October 6, 2026

R1 starts at reviewed M1-B commit `b13cf8313e4e5220d0f69e9552bb7c10c8b87a97` on `codex/m1b-readonly-outfitter-catalog`. The only C++ edit is in `ReadOnlyOutfitterPanel.cpp`. Its detail pane still labels `Outfit::Cost()` as **Base price**, but no longer calls `OutfitInfoDisplay::DrawRequirements()`, whose rows include a price calculated from `PlayerInfo::StockDepreciation()`. M1-B instead draws missing licenses, mass, required crew, and signed negative outfit attributes from native data using `Format::Number` / `OutfitInfoDisplay::FormatAttribute`; it continues to use `OutfitInfoDisplay::DrawAttributes()` for native attribute and weapon formatting. The fitting rows are included in the detail scroll extent. Shared `OutfitInfoDisplay`, depreciation, transaction code, `ShopPanel`, the legacy Outfitter, and the launch hook are unchanged.

**Depreciation check:** A disposable Ruin pilot had one locally stocked Hyperdrive with a stock-depreciation record dated `1000000`. In the running legacy Outfitter, selecting it displayed `cost (25%): 12,500`. In the R1 read-only panel, the same outfit displayed `Base price: 50,000 credits`; scrolling revealed factual mass, outfit-space, and native attributes with no second `cost` row. The legacy screen was inspected by selection only; no transaction was executed. The R1 captures `m1b-r1-depreciated-base-1280x800.png` and `m1b-r1-depreciated-details-1280x800.png` record the read-only result.

**Shift+Tab check:** In the native R1 panel, Tab focused search and showed its cyan focus border. Shift+Tab released search focus, returned keyboard handling to the read-only catalog, and left M1-B open. Pressing B and M afterward left M1-B on top; no underlying action screen opened. `m1b-r1-shift-tab-1280x800.png` records the unfocused field. This is native Xvfb input evidence, not a human usability or accessibility audit.

**Validation:** The Debug build completed. Outside sandbox tracing, `LD_LIBRARY_PATH=/tmp/endless-sky-m1a-deps/usr/lib/x86_64-linux-gnu ctest --test-dir /tmp/endless-sky-m1b-build --output-on-failure -R '^(unit|M1B Read Only Outfitter|Outfitter Mission Test|Shipyard Mission Test|Plugin Installed Autocondition)$' -j 1` passed **5/5**: unit 1/1 (78,613 assertions) and the four requested focused integration scenarios 4/4. No shared infrastructure changed, so the prior 46/46 normal integration result remains historical M1-B evidence; the full suite was not rerun for R1. The 46 debug/interactive variants, benchmark target, production-display review, and human usability review were not run. Twelve existing detail-bearing native captures in `docs/ui/reference/` were refreshed for R1, with three R1-specific captures added. Unaffected catalog-only and empty-result captures remain from M1-B.

## M1-B R2 native requirement classification — October 7, 2026

R2 starts from `f96172b29af64322b5bc323a5fb3bc7c5fb31dd3` on `codex/m1b-readonly-outfitter-catalog`. `OutfitInfoDisplay::IsNotRequirement` is now a public static query with its existing implementation unchanged; the native display's requirement and attribute paths still call that same classifier. `ReadOnlyOutfitterPanel` uses it before adding a negative outfit attribute to local fitting data, renders accepted requirements as positive `needed` values with native `Format::Number`, and retains the native special case in which positive `required crew` is a requirement. **Base price** and the R1 exclusion of `OutfitInfoDisplay::DrawRequirements()` remain in place. No transaction, depreciation, `ShopPanel`, or legacy Outfitter behavior changed.

The disposable `m1b-content` fixture now gives `M1B Survey Resonator` a negative `energy generation` attribute. At 1280×800 and 120% UI zoom, the native R2 detail view showed **energy generation: -120** in the native-formatted attribute section, while fitting data contained mass but not energy generation. A separate Hyperdrive view showed **outfit space needed: 20** under fitting data with no transaction-derived cost row. `m1b-plugin-details-scrolled-zoom120.png` was refreshed, and `m1b-r2-requirement-1280x800.png` records the positive requirement.

The Debug rebuild passed. The unit target passed **1/1**, and `M1B Read Only Outfitter` passed **1/1**. Because the classifier was exposed from shared `OutfitInfoDisplay`, the normal integration suite was run serially in four bounded batches: **12/12, 12/12, 12/12, and 10/10; 46/46 total, zero failed**. Existing display behavior was unchanged by the classifier move; the wider run validates that assumption for the covered scenarios. The 46 debug/interactive variants, benchmark target, production-display review, and human usability review were not run. No M2 work began.

## M2 native characterization work — October 7, 2026

The isolated `codex/m2-production-outfitter` worktree starts at the exact accepted M1-B commit `37177375016d551d83167fc78a068eab03fa0355`; the M1-B branch and the `ui-prototype` checkout were left unchanged. Remote `origin` was fetched before branch creation. This is **M2 in progress**, not a production Outfitter acceptance. The legacy Outfitter and the opt-in M1-B catalog are still the only Outfitter screens.

Headless integration skips drawing, while legacy item selection zones are built during drawing. A narrow integration-test hook now selects the specified outfit, quantity and optionally all local ships on the native `OutfitterPanel`; keyboard action inputs then reach its existing `CanMoveOutfit` / `MoveOutfit` path. No transaction, mission, save, or depreciation implementation has been changed. The disposable tests cover 12 single-ship transfer routes, multi-ship quantity and mixed eligibility, finite stock, capacity and credit limits, the exact-fit cargo boundary, crew and linked outfit changes, maps, licenses, owned-hold refill, save/reload, and mission entry/return. The initial 16 focused normal integration scenarios passed **16/16** outside sandbox tracing. Four further native scenarios for held licenses, unequal allocation, and depreciated stock passed **4/4** on the unchanged transaction binary. `PARITY.md` records each observed result and the remaining gaps.

The exact-fit fixture reproduces the native ship-to-cargo discrepancy: at precisely enough free cargo mass, `u` moves the outfit to storage. It is recorded separately and has not been fixed in M2. No modern transaction controls, preview evaluator, multi-ship preview, transaction statistics, or M2 native visual/input acceptance has been implemented. Because the test hook touches shared panel/input infrastructure, the sanitizer-enabled Debug build's full normal suite ran serially outside sandbox tracing: **63/63 passed** (62 integration scenarios and the unit target), with zero reported failures. The four additional scenarios above were added after that full run and passed separately. The debug/interactive variants, benchmark, production-display review, and human usability review were not run. This result applies to the characterization build before the preview-state copy was added.

## M2 production Outfitter implementation — awaiting owner review — October 7, 2026

`PlanetPanel` now opens `ModernOutfitterPanel` with Ctrl+Alt+O at a usable Outfitter. Ordinary O still opens the legacy transactional screen; Ctrl+Shift+O still opens the accepted read-only M1-B catalog. The modern panel retains native `OutfitterPanel::Step` for refill, help, mission offers, and selected-ship validation. It uses the M1-B catalog/search/sort/detail hierarchy with a persistent transaction footer, a fleet list, compact detail transition, and no browser runtime or save-schema change. The shop sale set remains the native snapshot taken on entry; visible ownership is rebuilt after transfers and modal return.

Explicit source and destination controls expose all twelve supported routes. G and T cycle those controls; Enter commits the selected route. B/S/I/U/C/R still execute the original native shortcut fallback order with one keypress. V sorts the catalog so Shift+S remains a quantity-modified Sell. The fleet list sends mouse click, Ctrl/Shift selection, double-click grouping, and drag reorder to native `ShopPanel` handlers; K and digit groups also reach native handlers. The modern panel records the actual route used by a fallback and shows requested quantity, fulfilled quantity, credit delta, selected/eligible/affected ships, per-ship changes, local hold/stock effects, licenses, maps, resource changes, and ordered primary outfit allocation. Disabled explicit routes expose the full native reason in a dialog. Partial results are labeled in the explicit action feedback.

`PlayerInfo::CloneForOutfitterPreview` copies only transaction-relevant mutable state and separate ships; it does not load a save or replay global universe changes. `OutfitterPanel::PreviewMoveOutfit` and `PreviewShortcut` execute the existing `MoveOutfit` / `HandleShortcuts` rules on that isolated state. `CommitMoveOutfit` recomputes the plan against current state, rejects a stale plan, then compares the live result and allocation trace with the preview. Focused scenarios on the final source passed for all twelve explicit routes, all twelve shortcut routes, multi-ship purchase and escort-group selection, disabled purchase, modern save/reload, modern mission entry/return, a real weapon and its ammunition, capacity/generation recharge, cargo-capacity and depreciation effects, map and minable-map discovery, and stale-plan rejection. The full normal suite passed **87/87** outside sandbox tracing: 86 integration scenarios (40 M2 and 46 prior) plus the unit target, serially with zero reported failures.

**Native Xvfb checks:** At 1280×800 and 960×680, the opt-in screen rendered its catalog, persistent route controls, quantity field, compact Back/detail page, and scrollable per-ship preview/result. A mouse transfer changed installed count and credits by the previewed amount. In a two-ship disposable pilot, a mouse click selected an escort, Ctrl-click selected both (the preview showed two affected ships), drag changed serialized ship order, and parking reduced fleet cargo capacity from 100 to 50; the saved pilot records the new order and parked flag. A separate `ui/focus` color-override plugin rendered magenta focus/selection without changing named interface definitions. Custom-category outfits were searchable and price-sortable. With a disposable generated plugin adding 1,000 outfits, the native category rendered, 75 cumulative wheel steps remained responsive on repeat, search found outfit 0999, sorting retained the selected detail, and a mouse purchase of that last item updated credits and installed count. Holding Shift showed quantity 5; Shift+S sold three installed units, credited 3,000, and returned quantity to 1 on release. Evidence captures are in `docs/ui/reference/m2-*.png`. These are automated native-display and input checks, not owner or human usability acceptance.

One earlier disposable Xvfb process exited normally during a 50-wheel-event burst with no sanitizer or application error reported; the immediate smaller and 75-step repeat did not reproduce it. The cause remains unknown. The existing exact-fit ship-to-cargo behavior remains a separate native gameplay defect candidate; M2 did not change it. The older `stoi` quantity concern was not reproduced. Debug/interactive integration variants, benchmark, production-display review, and human usability/contrast assessment have not been run.

**Final input repair:** An early compact Xvfb build tried to draw a 12-point footer label without a loaded native font shader and exited with an AddressSanitizer null-shader trace. The footer now uses the loaded 14-point font; rebuilt wide and compact captures completed without that fault. F focuses the modern search instead of the legacy Find dialog, and a focused integration scenario confirms action letters do not transact while editing. On the final native build, PageDown moved the 1,000-row catalog, End exposed outfits 0990–0999, Home returned to outfit 0000, and the compact detail scroll exposed planned ship changes and allocation order above its persistent footer.

**Map-information repair:** The first modern detail implementation named future map discoveries before purchase. It now gives no count or name for undiscovered systems before purchase, while retaining the exact set internally for plan agreement. A disposable native Xvfb check showed `Will reveal map coverage after purchase` before buying and `Mapped: Over the Rainbow` after a 200-credit purchase. `m2-map-preview-private.png` and `m2-map-result-after-purchase.png` record the two states. This source correction was made before the final full-suite result reported below.

**Minable-map coverage:** A disposable save adds an Iron minable payload to Terra Incognita. The focused modern purchase asserts the preview plan contains a harvested addition, then verifies its live result; the saved game records `harvested / Terra Incognita / Iron`. Native captures `m2-minable-map-preview-private.png` and `m2-minable-map-result.png` show generic system/resource coverage before purchase and the newly known system and Iron location afterward. The M2 plan now compares harvested-resource additions as well as visited systems on commit.

**Outcome-comparison coverage:** The final preview boundary compares serialized fleet/stock depreciation records, fleet cargo size/free space and largest hold, plus each affected ship's mass and outfit/weapon/engine/cargo capacities, in addition to credits, outfits, crew, resources, licenses, mapped systems, and harvested minables. `M2 Modern Cargo Capacity Preview` raised a fixture flagship's cargo space from 50 to 60 and passed preview/live agreement. The depreciated-stock, exact-fit, weapon/ammo, explicit-route, and minable-map cases passed again after the expanded comparison. These are native state comparisons, not UI-derived fitting or price estimates.

**Final validation:** The Debug CMake/Ninja build completed on the final source. With the locally unpacked M1-A dependencies on `LD_LIBRARY_PATH`, `ctest --test-dir /tmp/endless-sky-m2-build --output-on-failure -E '^\[debug\]|^benchmark$' -j 1` passed **87/87** in 1,833.72 seconds: 86 normal integration scenarios and one unit target. No sanitizer failure was reported. Earlier full-suite attempts were intentionally interrupted after source or fixture changes and are not counted as passing final validation. The 87/87 result applies to the fixed implementation and fixture set documented here. Native Xvfb input/captures are Codex-host evidence; production-display and human/owner acceptance remain not run. M3 has not started.

The wide 1,000-item color-override capture and the compact transaction/effect captures were refreshed from the exact 87/87 build after the depreciation and capacity comparison changes. They are `m2-wide-1000-outfits-ui-override.png`, `m2-compact-transactions-1000.png`, and `m2-compact-preview-effects-1000.png` under `docs/ui/reference/`. The compact effect capture shows previewed outfit-space and ship-mass changes and allocation order above the persistent transaction controls.

The M2 implementation and focused tests are in reviewable commits `91e2c09bc` and `33ec9a666`, following native-characterization commits `1f1aced02` and `f921b7d13`. Only the M2 branch was used for this implementation; the accepted M1-B branch and the prototype checkout were not edited.
