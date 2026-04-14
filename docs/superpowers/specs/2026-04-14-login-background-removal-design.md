# Login Background Removal Design

**Date:** 2026-04-14

**Scope:** Remove the login page's image background and replace it with a plain color surface. Keep the existing login card, toolbar, form flow, and theme toggle behavior unchanged.

## Goal

Temporarily simplify the login page so it no longer depends on `sky.png` or `cat3.png`, making the page easier to restyle later without carrying image-specific logic.

## Current Context

The current login page in `frontend/src/views/LoginView.vue`:

- imports `sky.png` and `cat3.png`
- preloads both assets on mount
- switches the visible full-screen background image based on the current theme
- overlays a gradient scrim above the image

This means the login page has its own dedicated visual background system on top of the application's global theme background.

## Approved Direction

The login page will use a plain color background only.

Specific decisions:

- remove all login-page image imports and image preloading logic
- remove theme-driven image switching for the login background
- keep one full-screen background layer in the template so layout structure stays stable
- make that layer use an existing theme color token instead of a bitmap image
- keep the login card, spacing, form markup, and submit behavior unchanged
- remove the extra login-page scrim so the background reads as a clean solid surface

## Theme Behavior

The page should still respect the existing light and dark themes by reusing current tokens from `frontend/src/styles/theme.css`.

The login-specific background layer should use `var(--panel-bg)` so:

- light theme stays clean and neutral
- dark theme stays consistent with the current dark palette
- no new theme tokens are required for this temporary change

## Component Boundaries

Expected file changes:

- `frontend/src/views/LoginView.vue`
  - remove image imports, preloading, and inline background-image binding
  - simplify the background markup and CSS to a plain color surface
- `frontend/src/views/__tests__/LoginView.spec.ts`
  - replace image-specific assertions with checks that the login page no longer depends on image assets or background-image style binding

## Data Flow

No backend, store, router, or API behavior changes are required.

The page continues to:

- collect username and password input
- call `authStore.login`
- redirect to `/projects` on success
- show existing notification behavior on failure

## Error Handling

No new error states are introduced.

Removing the image preload logic also removes any possibility of background-asset loading failure affecting the login view.

## Testing Strategy

This change needs coverage for:

- the login page still rendering a single-card layout
- the login view source no longer importing `sky.png` or `cat3.png`
- the login background no longer using an inline `backgroundImage` style
- the login page still submitting credentials and redirecting correctly

## Out Of Scope

The following are intentionally not part of this change:

- redesigning the login card itself
- changing typography, spacing, or button styling
- modifying global theme tokens
- deleting image files from the repository
- broader login-page visual redesign

## Self-Review

Checked:

- scope is limited to removing login-page image background behavior
- no placeholders remain
- test expectations align with the approved plain-background direction
- the implementation can be completed in one focused frontend change
