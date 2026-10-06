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
