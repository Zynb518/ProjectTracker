# Dark Theme Meteor Overlay Design

**Date:** 2026-04-14

**Scope:** Add a lightweight global meteor overlay for the `dark` theme so the night-sky background feels alive, while keeping the effect behind the application content and within a low-performance budget.

## Goal

Make the dark theme feel like a living galaxy workspace by adding occasional meteors that pass across the sky background without distracting from cards, tables, dialogs, or navigation.

This change must:

- keep `light` theme behavior unchanged
- show meteors only when the application is in `dark` mode
- keep the effect behind all foreground UI
- feel noticeable but restrained, matching the user's approved `B` frequency choice
- avoid reintroducing the stutter/performance issues the user already called out

## Current Context

The global dark atmosphere is currently split across:

- `frontend/src/styles/theme.css`
  - owns the global `dark` theme background image and dark overlay stack
- `frontend/src/App.vue`
  - owns the application root and is the safest place to mount a global visual layer without threading it through page-specific layouts

The current dark background already uses:

- `frontend/src/assets/backgrounds/night-sky.jpg`
- a dark overlay for readability
- no dedicated animated sky layer

That means the meteors should be treated as a separate visual layer, not as a replacement for the existing global background image.

## Approved Direction

The approved implementation direction is:

- use a dedicated global overlay layer instead of drawing meteors directly into `body::before` or `body::after`
- mount that overlay near the app root so it is always present across project, overview, and workspace pages
- only render/show the overlay in `dark` theme
- let meteors pass through the background, including the middle portion of the screen, but always behind foreground content
- target a medium visibility level: clearly perceptible, but not constant and not theatrical

## Architecture

Add a small dedicated component for background-only effects, for example:

- `frontend/src/components/backgrounds/MeteorSkyOverlay.vue`

Mount it inside the application root in:

- `frontend/src/App.vue`

Responsibilities should be split like this:

- `theme.css`
  - continues to own the static full-page dark background image and overlay tints
- `MeteorSkyOverlay.vue`
  - owns the fixed-position meteor effect layer
  - owns individual meteor element markup
  - owns meteor animation timing and placement
- existing page/layout components
  - remain unchanged unless needed for stacking safety

This keeps the animation isolated from the shared token file and avoids overloading `body::before` / `body::after` with both image support and animated streak logic.

## Visual Behavior

### Theme Rules

- `light` theme: no meteor layer
- `dark` theme: meteor layer visible

### Meteor Density

The user's chosen mood is:

- medium frequency
- clearly noticeable movement
- still subordinate to the content layer

Practical interpretation:

- 3 to 4 meteor elements total
- only 1 meteor should be visually prominent at a time
- each meteor should have a longer cycle with staggered delays so the page does not feel busy

### Motion Language

Each meteor should:

- start near the upper or upper-middle part of the viewport
- travel diagonally across the sky
- fade in quickly, travel, then fade out cleanly
- use a narrow streak with a gentle tail rather than a thick glowing beam

The effect should read as:

- occasional sky motion
- soft blue-violet cold light
- background ambience

not:

- arcade particles
- fireworks
- repeated scanning lines
- fast constant streak spam

## Layering Strategy

The overlay must remain behind application content.

Expected layering:

1. `html.dark body` background image and dark base tint
2. meteor overlay
3. application shells, cards, tables, dialogs, notices

Implementation constraints:

- meteor layer must use `pointer-events: none`
- it must not intercept clicks, selection, hover, or scrolling
- it should be fixed to the viewport rather than tied to any page panel

## Performance Constraints

This design is explicitly performance-sensitive.

To avoid the dark theme feeling heavy:

- animate only `transform` and `opacity`
- avoid JavaScript-driven frame loops
- avoid canvas for this phase
- avoid animating large full-screen filters or continuously shifting the background image
- keep the number of simultaneous moving elements very small

Accessibility support:

- add a `prefers-reduced-motion: reduce` fallback that disables the meteor animation entirely

## Styling Direction

The meteor visuals should fit the galaxy redesign already in progress:

- cooler cyan-to-violet tint, not warm yellow-orange
- subtle glow concentrated near the meteor head
- thin tail with rapid falloff
- no oversized bloom that washes out the `night-sky.jpg` details

## File Changes

Expected file changes:

- `frontend/src/App.vue`
  - mount the new global meteor overlay component near the app root
- `frontend/src/components/backgrounds/MeteorSkyOverlay.vue`
  - add the dedicated dark-theme meteor layer and animations
- `frontend/src/styles/__tests__/themeTypography.spec.ts`
  - add source-level assertions for the presence of the meteor layer integration if needed
- optional new component test
  - verify the overlay is dark-theme aware and non-interactive if test coverage benefits from it

## Testing Strategy

This change needs verification for:

- the app root including the overlay component
- the overlay remaining non-interactive
- reduced-motion support existing in source
- meteor animation being scoped to `dark` mode instead of `light`
- full frontend test and build stability

## Out Of Scope

The following are not part of this change:

- replacing the existing background image
- adding twinkling stars across the whole page
- adding parallax camera movement
- canvas or WebGL particle systems
- route-specific meteor behavior
- meteor-triggered sound, flashes, or UI reactions

## Self-Review

Checked:

- the scope stays limited to one global animated overlay
- the layering order is explicit and keeps content above the effect
- the performance constraints are concrete enough to guide implementation
- the design cleanly separates static background ownership from animated overlay ownership
