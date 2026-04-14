# Dark Theme Global Sky Background Design

**Date:** 2026-04-14

**Scope:** Replace the current synthetic CSS-only dark-theme page background with a real global background image based on `sky.jpg`. Rename and relocate the asset into a dedicated shared background directory, and keep the rest of the galaxy workspace system readable through a dark overlay.

## Goal

Make the dark theme visibly read as a starry night sky across the whole application by using the provided bitmap image directly as the global dark background.

This change must:

- keep `light` theme behavior unchanged
- make `dark` theme clearly show the sky image behind the app
- preserve readability for cards, tables, dialogs, and text by keeping a dark overlay above the image
- move the image into a clearer long-term asset location with a more descriptive filename

## Current Context

The current global dark background in `frontend/src/styles/theme.css` is generated entirely through CSS:

- `html.dark body` uses layered gradients
- `html.dark body::before` renders synthetic star points
- `html.dark body::after` renders extra nebula-like glows

This keeps the page atmospheric, but it does not show the actual sky image the user wants to use.

The current image file exists at:

- `frontend/src/assets/sky.jpg`

There are also login-specific assets in `frontend/src/assets/login/`, but those are a separate concern and should not be mixed into the global app background system.

## Approved Direction

The approved implementation direction is:

- rename `frontend/src/assets/sky.jpg`
- move it to `frontend/src/assets/backgrounds/night-sky.jpg`
- use that image as the `dark` theme's global `body` background
- keep a dark gradient overlay above the image so foreground UI remains legible
- reduce or remove the old synthetic star/nebula pseudo-layers so they do not fight the real image

## Asset Organization

The asset should move from:

- `frontend/src/assets/sky.jpg`

to:

- `frontend/src/assets/backgrounds/night-sky.jpg`

Reasoning:

- the image is no longer an ad-hoc loose asset
- the filename describes its role in the application
- the location supports future shared global backgrounds without mixing them with page-specific login assets

## Background Behavior

### Light Theme

No changes.

`html.light`, `:root`, and the general light-theme page background behavior stay exactly as they are today.

### Dark Theme

The dark theme background should become a layered composition:

1. base dark overlay gradient
2. the `night-sky.jpg` image
3. optional very subtle extra dark tint for contrast stabilization

The image should be configured to behave like a true full-screen page background:

- `background-position: center center`
- `background-size: cover`
- `background-repeat: no-repeat`
- fixed, non-moving appearance unless the current browser behavior makes `background-attachment: fixed` risky, in which case use a stable non-fixed fallback

## Readability Constraints

The image must not overpower the product UI.

To keep the workbench readable:

- maintain a dark overlay above the image
- avoid brightening foreground surfaces to compensate
- keep current card/dialog galaxy tokens intact
- make sure text contrast still reads against the new page backdrop

The global background image is the atmosphere layer, not the primary content surface.

## Pseudo-Layer Strategy

The current `html.dark body::before` and `html.dark body::after` effects should be reduced so the real image remains visible.

Expected direction:

- remove the synthetic starfield if it duplicates visible stars from the image
- remove or sharply reduce the current extra glow fog
- keep only minimal supporting tint if needed for depth

The final result should feel like:

- real sky photo first
- application overlay second
- micro-glow UI surfaces third

not:

- photo hidden behind a competing CSS star generator

## File Changes

Expected file changes:

- `frontend/src/styles/theme.css`
  - replace the dark-theme global background composition
  - reference the renamed background asset
  - simplify or reduce the old `body::before` and `body::after` dark pseudo-layers
- `frontend/src/styles/__tests__/themeTypography.spec.ts`
  - update source-level assertions for the new dark background implementation if needed
- filesystem move
  - `frontend/src/assets/sky.jpg`
  - `frontend/src/assets/backgrounds/night-sky.jpg`

## Testing Strategy

This change needs verification for:

- dark theme source using the relocated `night-sky.jpg`
- dark theme no longer relying on the old synthetic-only background stack
- light theme remaining untouched
- full frontend test and build stability after the asset move

## Out Of Scope

The following are not part of this change:

- redesigning workspace cards, tables, or dialogs again
- changing the login page background behavior
- changing the light theme
- introducing multiple rotating background images
- adding animation to the global page background

## Self-Review

Checked:

- scope is limited to the requested global dark background image change
- asset rename and destination are explicit
- the readability strategy is concrete rather than implied
- the design does not conflict with the current galaxy surface system
