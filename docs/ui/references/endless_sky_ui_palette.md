# Endless Sky UI Color Palette

Palette and usage guide for a minimalist, futuristic UI direction inspired by the Endless Sky style goals and the outfitter prototype concept.

**Purpose.** This document defines a restrained dark interface palette for redesigning Endless Sky screens while keeping attention on ship data, outfit decisions, and gameplay consequences. The style should feel technical and quiet: mostly monochrome, with cyan for interaction and a few warm status colors for decisions that need immediate attention.

## Core Palette

| Role | Name | Hex | Use |
| --- | --- | --- | --- |
| Background | Near black | `#0C0F11` | App background, empty space, inactive shell |
| Panel | Charcoal | `#14191D` | Primary panels and window interiors |
| Raised surface | Slate charcoal | `#1D252B` | Cards, rows, tab wells, dropdowns |
| Hover | Cool slate | `#27333B` | Hover rows and soft pressed states |
| Divider | Steel gray | `#39464F` | Hairlines, brackets, subtle HUD geometry |
| Control border | Medium gray | `#71818C` | Inputs, selectable rows, inactive buttons |
| Primary text | Cool white | `#E8EEF2` | Primary labels, titles, values |
| Secondary text | Silver | `#A8B5BF` | Descriptions, stat names, less critical values |
| Muted text | Gray | `#7F909C` | Disabled, unavailable, explanatory microcopy |
| Accent focus | Soft cyan | `#69D2E7` | Focus rings, selected edge, primary actions |
| Selected surface | Deep cyan slate | `#18333C` | Selected row or active tab background |
| Positive | Soft green | `#87C99A` | Valid improvement, installed, affordable |
| Caution | Warm amber | `#E6B566` | Capacity pressure, warning, tradeoff |
| Danger | Soft coral | `#EE8282` | Invalid purchase, blocked state, damage |

## Quick Visual Read

| Background | Panel | Surface | Divider | Accent | Caution | Danger |
| --- | --- | --- | --- | --- | --- | --- |
| `#0C0F11` | `#14191D` | `#1D252B` | `#39464F` | `#69D2E7` | `#E6B566` | `#EE8282` |

The interface should read as dark neutral structure first, with cyan appearing only where the player can act or where selection needs to be unmistakable.

## Usage Rules

- **Neutral first.** Most of the interface should use near black, charcoal, slate, and gray. The UI should feel quiet, like a spacecraft instrument panel.
- **Cyan means interaction.** Use cyan for focus, selection, active tabs, and primary actions. It should be a thin edge, glow, or button fill rather than a full-screen wash.
- **Status colors stay scarce.** Use green, amber, and coral only when the player must compare outcomes or understand a blocked action.
- **Decoration stays faint.** Circuit traces, brackets, numerals, and small geometric details should sit in divider gray or lower-opacity cyan.
- **Readable before atmospheric.** Primary text should stay cool white on dark panels. Muted text is for secondary information, never for required actions.

## Component Guidance

| Component | Treatment |
| --- | --- |
| Primary button | Soft cyan fill with near-black text. Use for Buy, Install, Accept, or Launch. |
| Secondary button | Raised slate surface, medium-gray border, cool-white text. Cyan border only on focus. |
| Selected row | Deep cyan slate surface, one cyan edge or bracket, cool-white primary label. |
| Inactive tab | Transparent or panel charcoal with silver text and a steel-gray underline. |
| Active tab | Charcoal or selected surface with cyan top or bottom edge. |
| Capacity meter | Silver under normal use, amber near limits, coral when a preview would exceed limits. |
| Disabled item | Charcoal surface with muted gray text and a clear reason nearby. |

## Recommended Screen Balance

A typical screen should be roughly 80 percent neutral dark surfaces and text, 15 percent dividers and inactive control borders, and 5 percent color accents. If cyan starts appearing on every border, icon, and label, selection will stop feeling meaningful.

## Implementation Note

Name tokens by role rather than by color. For example:

```css
--ui-background: #0C0F11;
--ui-panel: #14191D;
--ui-surface-raised: #1D252B;
--ui-hover: #27333B;
--ui-divider: #39464F;
--ui-border-control: #71818C;
--ui-text-primary: #E8EEF2;
--ui-text-secondary: #A8B5BF;
--ui-text-muted: #7F909C;
--ui-accent-focus: #69D2E7;
--ui-selected-surface: #18333C;
--status-positive: #87C99A;
--status-caution: #E6B566;
--status-danger: #EE8282;
```

Token names should describe the role in the interface. That makes it easier to tune the palette later without rewriting component code.
