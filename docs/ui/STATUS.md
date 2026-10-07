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
