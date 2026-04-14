# Dark Theme Galaxy Redesign Design

**Date:** 2026-04-14

**Scope:** Redesign the frontend dark theme into a lighter-weight "micro-glow galaxy" system. Prioritize project/workspace card experiences first, starting from global theme semantics and then applying them to the highest-frequency workspace surfaces.

## Goal

Replace the current heavy cyber-neon dark theme with a calmer night-sky galaxy theme that:

- keeps the "black night / galaxy / stars" atmosphere
- borrows color inspiration from `frontend/src/assets/sky.jpg` without using the image directly
- improves perceived performance by removing the heaviest animated and blurred background effects
- makes project/workspace cards feel more intentional and readable

## Approved Direction

The approved visual direction is:

- galaxy style: `B / 平衡银河`
- image usage: `sky.jpg` is inspiration only, not a full-screen theme background image
- priority: `card-oriented workspace pages first`
- rollout order: `overall system first, local page surfaces second`

## Current Problems

The current dark theme has the right ambition but is too heavy in the wrong places.

Primary issues:

- animated scanline background on `html.dark body::before`
- large blurred atmospheric layer on `html.dark body::after`
- broad use of `backdrop-filter` across main cards and layout containers
- glow and gradient density that pushes the look toward cyber UI instead of calm galaxy UI
- card surfaces that rely too much on blur rather than on deliberate surface hierarchy

## New Theme System

### Background Structure

The new dark theme uses a static layered background:

- deep navy-to-midnight base gradient
- sparse star points
- low-opacity cyan / violet nebula glows at edges and corners
- no scanline motion
- no full-screen heavy blur filter

This should feel like a quiet night sky rather than a holographic console.

### Surface Hierarchy

Dark surfaces are split into clear semantic layers:

- `app background`
  - night-sky page backdrop
- `primary workspace surface`
  - large overview cards, workspace shells, hero panels
- `secondary surface`
  - metric blocks, nested info regions, filter groups
- `dialog surface`
  - overlays and edit forms
- `interactive highlight`
  - buttons, selected chips, active pills

The system should communicate depth mostly through contrast, border light, and shadow structure, not through thick blur.

### Color Direction

The new dark palette should read as:

- base: deep blue-black
- support: muted slate blue
- glow: cold cyan with restrained violet accents
- text: cooler near-white with softer secondary slate text

The theme should avoid overusing hot magenta or bright neon pink.

### Performance Constraints

The redesign must explicitly reduce heavy effects:

- remove `scanline-drift` background animation
- remove dark-theme full-screen `filter: blur(...)` atmosphere layers
- reduce reliance on `backdrop-filter` for main layout surfaces and cards
- keep any remaining blur limited to truly modal/overlay contexts if needed
- prefer static gradients, subtle stars, and thin luminous borders

## High-Priority Surfaces

The following surfaces are the first-class targets for the redesign:

- `frontend/src/styles/theme.css`
  - rebuild dark tokens and background layers
- `frontend/src/views/ProjectListView.vue`
  - top hero shell, pagination, state cards
- `frontend/src/components/projects/ProjectGrid.vue`
  - project cards
- `frontend/src/views/ProjectDetailView.vue`
  - view switch shell, workspace shell, state blocks, placeholder
- `frontend/src/components/workspace/ProjectHero.vue`
  - overview card and nested metrics/meta surfaces
- `frontend/src/views/MyTasksView.vue`
  - hero/state shells
- `frontend/src/components/my-tasks/MyTaskBoard.vue`
  - task cards

## Lower-Priority Surfaces

The redesign should keep compatibility with existing dialog tokens so later passes can continue the same galaxy system across:

- project dialogs
- subtask dialogs
- notice toasts
- menus and drawers

These can inherit new tokens immediately even if their local CSS is not deeply rewritten in the first pass.

## Testing Strategy

This redesign needs verification for:

- new dark-token values and removal of scanline-based background styling
- retention of semantic status tokens
- project and task cards still using semantic theme variables rather than ad-hoc color mixes
- workspace hero cards and key view shells no longer depending on heavy `backdrop-filter` usage
- full frontend test and build stability

## Out Of Scope

The following are not part of this pass:

- changing the light theme direction
- using `sky.jpg` directly as a global background
- redesigning every single component one by one
- backend or API changes

## Self-Review

Checked:

- scope stays centered on the dark theme and its highest-frequency surfaces
- the approved direction aligns with user choices: `B`, `inspiration-only image`, `workspace cards first`
- performance reduction is explicit rather than implied
- the work remains implementable as one coherent frontend change
