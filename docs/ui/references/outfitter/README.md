# Endless Sky UI — Canonical Outfitter References

These files define the approved visual direction for the native UI modernization. They are design references, not gameplay specifications. Existing C++ game logic remains authoritative for native gameplay behavior.

## Reference priority

When references disagree, use this order:

1. Native gameplay behavior
2. [reference.png](reference.png)
3. [desktop-final.png](desktop-final.png)
4. [cargo-installed.png](cargo-installed.png)
5. [compact-1280.png](compact-1280.png)
6. [endless_sky_ui_palette.md](../endless_sky_ui_palette.md)

The palette defines semantic color usage; it does not override layout or composition.

## reference.png — primary visual and layout target

Use this image as the strongest reference for screen composition, visual hierarchy, information grouping, spacing, panel proportions, major navigation placement, selected-item presentation, and the relationship between catalog, ship details, and transaction preview. It is the primary answer to what the modern Endless Sky UI should look like. Its role extends beyond color.

## desktop-final.png — implemented interaction and layout reference

Use this image to understand the searchable catalog, source and inventory tabs, equipment browsing, ship inspection, current-to-after preview, control grouping, and how the design worked in an interactive browser prototype. For exact visual composition, `reference.png` takes priority.

## cargo-installed.png — transaction-state and feedback reference

Use this image for owned inventory presentation, installed and cargo state, preview feedback, blocked-action explanations, and transaction-state messages. Its illustrative values are not native gameplay truth.

## compact-1280.png — compact-layout reference

Use this image when adapting the design to smaller supported layouts. It demonstrates retained hierarchy under reduced space, persistent transaction controls, scrollable content, and reduced-density presentation. It is not the default wide-layout target.

## Color reference

[endless_sky_ui_palette.md](../endless_sky_ui_palette.md) defines semantic color meaning, interaction accents, neutral structure, warning and status colors, and text hierarchy. It does not define overall layout. The intended style uses dark neutral structure, restrained cyan interaction accents, sparse status color, and a quiet spacecraft-instrument-panel character. Readability takes priority over decoration.

## Critical rule

Modernizing the native UI means following the approved composition and hierarchy while preserving gameplay. Applying a dark background and cyan borders to legacy geometry alone does not meet the visual direction.

## Historical note

These images are copies of assets on the `ui-prototype` branch. The originals remain there for historical continuity; the copies give native implementation branches direct access to the approved references.
