# AppShell Light Sidebar Sync Design

## Context

The light-theme refresh has already landed on the login page and the project list page. The remaining mismatch is the left sidebar in `frontend/src/layouts/AppShell.vue`.

`AppShell.vue` currently contains two competing light-theme treatments:

1. The scoped component styles already use the newer light-theme token system built on layered cool surfaces.
2. The trailing global `html.light` and `html:not(.dark)` overrides force the sidebar, navigation, account card, logout button, and theme toggle back into a darker glass treatment that no longer matches the refreshed project cards.

This produces a split visual language in light mode: the main workspace uses pale layered panels while the left rail still reads like a dark translucent overlay.

## Goal

Make the entire left sidebar outer shell in light theme feel like the same surface family as the refreshed project list cards.

## Non-Goals

- Do not change sidebar structure, routing, copy, or interaction behavior.
- Do not redesign the internal information hierarchy inside the sidebar.
- Do not change dark-theme styling.
- Do not change sticky behavior, responsive layout behavior, or workspace background composition outside the existing light sky treatment.

## Design Summary

### 1. Sync the sidebar shell to the project-card surface language

In light theme, `.app-shell__sidebar` should read like a tall project card rather than a dark glass slab.

The sidebar shell should therefore use the same base surface family as `frontend/src/components/projects/ProjectGrid.vue`:

- `--project-card-bg` as the primary surface
- `--project-card-glow` as the cool highlight layer
- a soft border derived from the normal light token system instead of the darker glass border
- a panel shadow aligned with the project-card elevation

Implementation detail:

- Keep the existing layout, radius, spacing, and sticky positioning.
- Keep the static `--card-sheen` layer on the sidebar shell so the chrome still has a subtle reflective finish without needing the project-card hover overlay pattern.

Expected result:

- The left rail becomes part of the same light-theme family as the project list cards.
- The shell feels cooler, brighter, and less like a dark overlay sitting on top of the page.

### 2. Remove the obsolete light-theme dark-glass overrides

The global light-theme overrides at the end of `AppShell.vue` were added for the earlier glass-heavy sidebar treatment and now conflict with the scoped token-based light styles.

For this sync pass:

- Keep the global light-theme sky background rule for `.app-shell::before`.
- Remove or neutralize the light-only overrides that hardcode dark translucent surfaces or near-white text for:
  - `.app-shell__sidebar`
  - `.app-shell__eyebrow`
  - `.app-shell__brand h1`
  - `.app-shell__brand-copy`
  - `.app-shell__foot-label`
  - `.app-shell__account-note`
  - `.app-shell__sidebar-foot`
  - `.app-shell__link`
  - `.app-shell__link-icon`
  - `.app-shell__account-card`
  - `.app-shell__account-copy strong`
  - `.app-shell__account-copy span`
  - `.app-shell__logout`
  - `.app-shell__theme-toggle`
  - `.theme-toggle__track`
  - `.theme-toggle__thumb`
  - `.theme-toggle__label`

After cleanup, the component should fall back to its scoped light-theme token usage for these elements.

Reasoning:

- The scoped styles already provide the desired second-level hierarchy with `meta-surface` and control tokens.
- Removing the dark-glass light overrides is the smallest change that restores consistency without redesigning the sidebar internals.

### 3. Preserve the current visual hierarchy inside the sidebar

This pass only changes the shell language and removes incompatible light-only overrides.

Internal hierarchy should stay as follows:

- Sidebar shell: project-card family surface
- Navigation links, account card, logout button, theme toggle: existing scoped token-driven secondary surfaces
- Text and icon colors: current scoped token-driven readable light-theme values

This keeps the sidebar aligned with the project list page while preserving the existing content emphasis and click targets.

### 4. Testing boundary

Update `frontend/src/layouts/__tests__/AppShell.spec.ts` so the source-level style assertions verify the new boundary:

- The sidebar shell uses project-card surface tokens instead of the old meta-surface treatment.
- The light-theme sky background rule remains.
- The file no longer contains a dedicated light-theme sidebar override that paints the shell as a dark translucent slab.
- The file no longer depends on light-theme hardcoded white heading text that only made sense on the dark-glass shell.

No behavior tests need to change because routing, auth rendering, sticky layout, and theme toggling behavior are not part of this visual sync.

## Acceptance Criteria

- In light theme, the left sidebar shell visually matches the refreshed project card family rather than the older dark-glass shell.
- The light-theme sky background remains present behind the authenticated layout.
- Sidebar navigation, account section, logout button, and theme toggle still render with clear hierarchy and readable contrast.
- Dark-theme styling remains unchanged.
- Existing AppShell behavior tests still pass after updating the style assertions for the new visual direction.

## Risks and Constraints

- If any inner element still relies on the removed light-theme overrides for contrast, it must be rebalanced using existing theme tokens rather than adding new hardcoded light overrides.
- This design intentionally avoids introducing new `shell-*` theme tokens because the goal is visual alignment with existing project cards, not a parallel token family.

## Implementation Notes

- Preferred touch points:
  - `frontend/src/layouts/AppShell.vue`
  - `frontend/src/layouts/__tests__/AppShell.spec.ts`
- Avoid theme-token expansion unless implementation exposes a clear gap that cannot be solved with the current `project-card`, `meta-surface`, and control tokens.
