# Dark Theme Galaxy Redesign Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rebuild the dark theme into a calmer galaxy-night system and apply it to the main workspace/project/task surfaces while reducing the heaviest blur and animation costs.

**Architecture:** Update the global dark-theme token system first in `theme.css`, then align the highest-frequency workspace and card components to those semantics. Use tests to lock the new palette/performance rules before implementation, then verify the full frontend suite and build after the visual refactor lands.

**Tech Stack:** Vue 3, TypeScript, Pinia, Vue Testing Library, Vue Test Utils, Vitest, Vite

---

### Task 1: Lock The New Dark Theme Constraints In Tests

**Files:**
- Modify: `frontend/src/styles/__tests__/themeTypography.spec.ts`
- Modify: `frontend/src/components/workspace/__tests__/ProjectHero.spec.ts`
- Modify: `frontend/src/components/__tests__/ProjectGrid.spec.ts`
- Modify: `frontend/src/components/__tests__/MyTaskBoard.spec.ts`
- Modify: `frontend/src/views/__tests__/ProjectListView.spec.ts`
- Modify: `frontend/src/views/__tests__/ProjectDetailView.spec.ts`
- Modify: `frontend/src/views/__tests__/MyTasksView.spec.ts`

- [ ] **Step 1: Update theme tests for the new dark palette and lighter background system**

```ts
expect(themeSource).toContain('--app-bg: #060916;')
expect(themeSource).toContain('--panel-bg: #0d1323;')
expect(themeSource).toContain('--text-main: #ecf3ff;')
expect(themeSource).toContain('--accent-start: #7cd3ff;')
expect(themeSource).toContain('--accent-end: #8d78ff;')
expect(themeSource).not.toContain('repeating-linear-gradient(')
expect(themeSource).not.toContain('animation: scanline-drift')
```

- [ ] **Step 2: Add source-level expectations that the main workspace/card surfaces no longer rely on heavy backdrop blur**

```ts
expect(source).not.toContain('backdrop-filter: var(--backdrop-blur);')
```

- [ ] **Step 3: Run targeted tests and confirm they fail for the right reasons**

Run: `npm test -- --run src/styles/__tests__/themeTypography.spec.ts src/components/workspace/__tests__/ProjectHero.spec.ts src/components/__tests__/ProjectGrid.spec.ts src/components/__tests__/MyTaskBoard.spec.ts src/views/__tests__/ProjectListView.spec.ts src/views/__tests__/ProjectDetailView.spec.ts src/views/__tests__/MyTasksView.spec.ts`
Expected: FAIL because the current theme still uses scanline styling, old dark tokens, and blur-heavy surfaces.

- [ ] **Step 4: Commit**

```bash
git add frontend/src/styles/__tests__/themeTypography.spec.ts frontend/src/components/workspace/__tests__/ProjectHero.spec.ts frontend/src/components/__tests__/ProjectGrid.spec.ts frontend/src/components/__tests__/MyTaskBoard.spec.ts frontend/src/views/__tests__/ProjectListView.spec.ts frontend/src/views/__tests__/ProjectDetailView.spec.ts frontend/src/views/__tests__/MyTasksView.spec.ts
git commit -m "test: lock galaxy dark theme constraints"
```

### Task 2: Rebuild The Global Dark Theme Tokens

**Files:**
- Modify: `frontend/src/styles/theme.css`

- [ ] **Step 1: Replace the current dark token palette with the new galaxy-night values**

```css
html.dark {
  --app-bg: #060916;
  --panel-bg: #0d1323;
  --text-main: #ecf3ff;
  --accent-start: #7cd3ff;
  --accent-end: #8d78ff;
}
```

- [ ] **Step 2: Replace scanline and full-screen blur effects with static starfield / nebula layers**

```css
html.dark body {
  background:
    radial-gradient(circle at 18% 16%, rgba(124, 211, 255, 0.14), transparent 20%),
    radial-gradient(circle at 82% 18%, rgba(141, 120, 255, 0.16), transparent 22%),
    linear-gradient(180deg, #060916 0%, #0a1020 52%, #0d1426 100%);
}
```

- [ ] **Step 3: Keep dialog-related tokens compatible while reducing heavy blur defaults**

```css
--backdrop-blur: blur(10px);
```

- [ ] **Step 4: Run the theme test**

Run: `npm test -- --run src/styles/__tests__/themeTypography.spec.ts`
Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add frontend/src/styles/theme.css frontend/src/styles/__tests__/themeTypography.spec.ts
git commit -m "feat: rebuild dark galaxy theme tokens"
```

### Task 3: Apply The New Surface Language To Workspace And Card Views

**Files:**
- Modify: `frontend/src/views/ProjectListView.vue`
- Modify: `frontend/src/components/projects/ProjectGrid.vue`
- Modify: `frontend/src/views/ProjectDetailView.vue`
- Modify: `frontend/src/components/workspace/ProjectHero.vue`
- Modify: `frontend/src/views/MyTasksView.vue`
- Modify: `frontend/src/components/my-tasks/MyTaskBoard.vue`

- [ ] **Step 1: Restyle list/detail/task hero shells and state surfaces to use the new primary/secondary galaxy surfaces**

```css
background: var(--gradient-surface), var(--project-card-glow);
box-shadow: var(--shadow-panel);
```

- [ ] **Step 2: Remove heavy blur from primary cards and workspace shells**

```css
/* delete backdrop-filter from the main project/task/workspace shells */
```

- [ ] **Step 3: Tune hover borders, shadows, and nested meta surfaces to match the calmer cyan-violet galaxy system**

```css
border-color: color-mix(in srgb, var(--accent-line) 72%, transparent);
box-shadow: 0 20px 40px rgba(4, 8, 20, 0.42), 0 0 0 1px rgba(124, 211, 255, 0.08);
```

- [ ] **Step 4: Run the targeted component/view tests**

Run: `npm test -- --run src/components/workspace/__tests__/ProjectHero.spec.ts src/components/__tests__/ProjectGrid.spec.ts src/components/__tests__/MyTaskBoard.spec.ts src/views/__tests__/ProjectListView.spec.ts src/views/__tests__/ProjectDetailView.spec.ts src/views/__tests__/MyTasksView.spec.ts`
Expected: PASS

- [ ] **Step 5: Commit**

```bash
git add frontend/src/views/ProjectListView.vue frontend/src/components/projects/ProjectGrid.vue frontend/src/views/ProjectDetailView.vue frontend/src/components/workspace/ProjectHero.vue frontend/src/views/MyTasksView.vue frontend/src/components/my-tasks/MyTaskBoard.vue frontend/src/components/workspace/__tests__/ProjectHero.spec.ts frontend/src/components/__tests__/ProjectGrid.spec.ts frontend/src/components/__tests__/MyTaskBoard.spec.ts frontend/src/views/__tests__/ProjectListView.spec.ts frontend/src/views/__tests__/ProjectDetailView.spec.ts frontend/src/views/__tests__/MyTasksView.spec.ts
git commit -m "feat: apply galaxy dark theme to workspace surfaces"
```

### Task 4: Verify Full Frontend Stability

**Files:**
- No new files

- [ ] **Step 1: Run the full frontend test suite**

Run: `npm test`
Expected: PASS

- [ ] **Step 2: Run the production build**

Run: `npm run build`
Expected: PASS and output generated in `dist/`.

- [ ] **Step 3: Review the scoped diff**

Run: `git diff -- docs/superpowers/specs/2026-04-14-dark-theme-galaxy-redesign-design.md docs/superpowers/plans/2026-04-14-dark-theme-galaxy-redesign.md frontend/src/styles/theme.css frontend/src/views/ProjectListView.vue frontend/src/components/projects/ProjectGrid.vue frontend/src/views/ProjectDetailView.vue frontend/src/components/workspace/ProjectHero.vue frontend/src/views/MyTasksView.vue frontend/src/components/my-tasks/MyTaskBoard.vue frontend/src/styles/__tests__/themeTypography.spec.ts frontend/src/components/workspace/__tests__/ProjectHero.spec.ts frontend/src/components/__tests__/ProjectGrid.spec.ts frontend/src/components/__tests__/MyTaskBoard.spec.ts frontend/src/views/__tests__/ProjectListView.spec.ts frontend/src/views/__tests__/ProjectDetailView.spec.ts frontend/src/views/__tests__/MyTasksView.spec.ts`
Expected: Diff covers only the approved galaxy dark-theme redesign scope.

- [ ] **Step 4: Commit**

```bash
git add docs/superpowers/specs/2026-04-14-dark-theme-galaxy-redesign-design.md docs/superpowers/plans/2026-04-14-dark-theme-galaxy-redesign.md frontend/src/styles/theme.css frontend/src/views/ProjectListView.vue frontend/src/components/projects/ProjectGrid.vue frontend/src/views/ProjectDetailView.vue frontend/src/components/workspace/ProjectHero.vue frontend/src/views/MyTasksView.vue frontend/src/components/my-tasks/MyTaskBoard.vue frontend/src/styles/__tests__/themeTypography.spec.ts frontend/src/components/workspace/__tests__/ProjectHero.spec.ts frontend/src/components/__tests__/ProjectGrid.spec.ts frontend/src/components/__tests__/MyTaskBoard.spec.ts frontend/src/views/__tests__/ProjectListView.spec.ts frontend/src/views/__tests__/ProjectDetailView.spec.ts frontend/src/views/__tests__/MyTasksView.spec.ts
git commit -m "test: verify galaxy dark theme redesign"
```
