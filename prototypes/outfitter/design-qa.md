# Outfitter prototype design QA

final result: passed

## Comparison target and evidence

- Approved source: `design/reference.png`.
- Original local verification URL: `http://127.0.0.1:4176/`. This URL serves the prototype only while its local development server is running.
- Final browser capture: `evidence/desktop-final.png`.
- Source and implementation: 1672 × 941 pixels; browser viewport 1672 × 941 CSS pixels, devicePixelRatio 1. No density resizing was applied to either image.
- State: Power category; Shop source; RT-I Radiothermal selected; quantity 1; destination Peregrine; initial illustrative ship and credits; all columns visible; no filters or notifications.
- Full comparison, with source and implementation in one image: `evidence/comparison-final.png`.
- Focused comparison inspected for readable statistics, numeric alignment, copy, and preview: `evidence/detail-comparison-final.png`. Additional catalog comparison: `evidence/catalog-comparison-final.png`.
- Responsive captures: `evidence/laptop-1440.png` (1440 × 900), `evidence/compact-1280.png` (1280 × 720), and `evidence/mobile-390.png` (390 CSS pixels wide, full document). Viewport changes were allowed to settle before accepted captures; the viewport override was reset after testing.
- Transaction evidence: `evidence/cargo-installed.png`.

## Findings and fixes

No actionable P0, P1, or P2 findings remain within this Outfitter prototype's scope.

### Iteration history

1. **Initial desktop comparison — blocked.** `evidence/desktop-v1.png` and the combined `evidence/comparison-v1.png` showed the last catalog row partly hidden and lower ship statistics clipped at the reference size (P2). Header and row heights were reduced to 34 and 52 pixels; ship statistics spacing was tightened; the zero-cargo row is omitted until cargo is held. After these changes, all eight rows and all initial ship statistics are visible at the reference size. Browser measurements confirmed matching client/scroll heights in both regions. Smaller windows intentionally provide scrollable catalog/statistic regions above persistent transaction controls.
2. **Semantic color and control treatment — fixed.** The generated-heat change was white rather than amber because of CSS specificity, and the native checkbox had a bright default theme (P2). Applied the amber rule at the correct specificity and dark native control treatment. Verified in `evidence/desktop-v2.png` and the final combined comparison.
3. **Compact column headers — fixed.** At 1280 pixels, rate-unit headers touched adjacent column labels (P2). Header buttons now wrap labels and units within their own column. Verified in `evidence/compact-1280.png`; browser measurements show each header's scroll width equals its client width.
4. **Interaction review — fixed before handoff.** Source changes clear the fit filter so installed equipment remains available for uninstalling. Arrow navigation moves keyboard focus with selection. Empty search results have no stale financial projection. Secondary destination shortcuts now select a destination for review; the primary action commits exactly the displayed preview. All four paths were exercised again in the browser.

## Required fidelity surfaces

- **Fonts and typography:** locally bundled Ubuntu and Ubuntu Mono; readable numeric alignment, differentiated headings, restrained wordmark, clear units. Original data descriptions are longer than the mock's editorial descriptions; they are deliberately clamped with the full description available as a tooltip. Small optical differences from generated typography remain P3.
- **Spacing and layout rhythm:** preserved the 224-pixel category rail, flexible catalog, 420-pixel inspector, 64-pixel header and 48-pixel footer at the reference viewport. Persistent transaction actions remain reachable. Narrow layouts reflow into a vertical document and retain horizontal table scrolling rather than compressing numeric columns beyond readability.
- **Colors and tokens:** navy surfaces, thin blue-gray separators, cyan selection/actions, green positive states, amber constraints/heat. Visible words accompany semantic colors. Checked hover, focus, selection, disabled and empty states.
- **Images and icons:** all 20 equipment thumbnails and the Falcon sprite are original game assets with preserved aspect ratios and transparency; Phosphor supplies standard interface icons. The canonical game artwork intentionally replaces the image generator's altered renditions. No CSS/handmade SVG substitutes were used for game artwork. Low-resolution source thumbnails are a P3 limitation for large displays.
- **Copy and content:** verified catalog values and base prices; separate fit and heat messages; explicit source/destination; current/after values; in-app disclosure of illustrative ship state and limited simulation. “To cargo” and “Preview sale” select a destination first, a deliberate functional refinement so a committed action always matches its preview. Added session Undo/Reset controls for reviewing the prototype.

## Browser interactions exercised

- Initial RT-I install: credits 32,390,000 → 32,090,000; free outfit space 57 → 12 t; mass 993 → 1,038 t; installed quantity 0 → 1.
- A second install is blocked with a visible capacity explanation.
- Uninstall to storage restores ship capacity without charging credits; selling stored equipment restores base-price credits.
- Quantity 2 cannot be installed, but its cargo preview correctly shows 90 t used. Buying to cargo changes credits and cargo only. Moving one owned item from cargo to the ship charges no additional credits.
- Fits-only filtering, searching, no-results state, cost sorting, hiding/restoring columns, Guns/Engines category switching, and hardpoint restrictions.
- Keyboard B purchase, arrow selection followed by Enter, and matching focus/selection.
- Undo verified by a fresh accessibility snapshot: credits and ship state return to their pre-purchase values. Reset restores the initial state.
- A purchase was exercised at the narrow mobile viewport; transaction controls remain reachable by scrolling.
- Browser error log checked after interactions and at handoff: no errors returned.

## Executed checks

- Repository packaging: repeated `npm ci`, the production build, and all 12 transaction tests successfully from `prototypes/outfitter`. Runtime source was preserved except for trailing-whitespace cleanup. Assets, package manifest, and lockfile were copied unchanged from the browser-verified prototype. Documentation paths were made portable and the approved reference was included at `design/reference.png`.
- JavaScript syntax checks for catalog and model: passed.
- Transaction model: 12 test groups passed (`node tests/model.test.mjs`). Covers rejected quantities, insufficient funds, inventory ownership, every capacity, exact fits, reversibility, immutable previews, and independent transaction composition.
- Final production build: passed (`npm run build`).
- Native game repository status remained clean. No native-game build, save-file integration, or full flight simulation was performed.

## Scope, open questions, and follow-up polish

This is a browser prototype of the approved Outfitter, with a representative 20-item catalog. It is not a native-game patch. Port, Trade and Shipyard display a scope explanation rather than opening unbuilt screens. Transactions are in-memory only. Full cooling, temperature, repair, licenses, depreciation, and travel are outside the model; these limits are disclosed in the prototype.

No blocking design questions remain for this prototype. P3 follow-ups are matching the generated typeface's precise optical proportions and providing higher-resolution canonical equipment artwork where available. These do not prevent assessing the chosen layout and workflow.

## Implementation checklist

- [x] Recreate the approved visual hierarchy and density.
- [x] Use real equipment data and original game artwork.
- [x] Implement and verify the fitting/transfer workflow.
- [x] Fix P0/P1/P2 findings and inspect combined visual evidence.
- [x] Check desktop, compact desktop and narrow layouts.
- [x] Keep the original verified local preview available for review; a fresh checkout can start its own preview using the README instructions.
