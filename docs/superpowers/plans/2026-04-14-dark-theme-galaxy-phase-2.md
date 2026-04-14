# Dark Theme Galaxy Redesign Phase 2 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extend the new galaxy dark theme from the first-pass workspace cards into the remaining high-visibility UI surfaces: shell chrome, filters, theme toggle, notification toasts, and main modal/dialog surfaces.

**Architecture:** Keep the new dark token system from `theme.css` and align the remaining surfaces to it through targeted source-driven tests and focused style updates. Preserve modal depth where needed, but reduce gratuitous blur and move the visual emphasis toward controlled gradients, fine borders, and cleaner galaxy highlights.

**Tech Stack:** Vue 3, TypeScript, Pinia, Vue Test Utils, Vitest, Vite

---

### Task 1: Lock The Remaining Dark-Surface Rules In Tests

**Files:**
- Modify: `frontend/src/components/__tests__/ProjectFilters.spec.ts`
- Modify: `frontend/src/components/__tests__/GlobalNoticeLayer.spec.ts`
- Modify: `frontend/src/components/__tests__/ProjectFormDialog.spec.ts`
- Modify: `frontend/src/components/__tests__/SubtaskDialogs.spec.ts`
- Modify: `frontend/src/layouts/__tests__/AppShell.spec.ts`

- [ ] **Step 1: Add source-level expectations for galaxy surfaces and lighter blur usage**

```ts
expect(source).toContain('background: var(--meta-surface-bg), var(--meta-surface-glow), var(--card-sheen);')
expect(source).not.toContain('backdrop-filter: blur(16px);')
```

- [ ] **Step 2: Keep dialog tests aligned with the new modal surface tokens**

```ts
expect(source).toContain('background: var(--dialog-surface-glow), var(--dialog-surface-bg), var(--card-sheen);')
```

- [ ] **Step 3: Run the targeted tests to verify they fail**

Run: `npm test -- --run src/components/__tests__/ProjectFilters.spec.ts src/components/__tests__/GlobalNoticeLayer.spec.ts src/components/__tests__/ProjectFormDialog.spec.ts src/components/__tests__/SubtaskDialogs.spec.ts src/layouts/__tests__/AppShell.spec.ts`
Expected: FAIL because the remaining surfaces still use older blur-heavy or flatter dark styling.

### Task 2: Restyle Remaining Shell And Utility Surfaces

**Files:**
- Modify: `frontend/src/layouts/AppShell.vue`
- Modify: `frontend/src/components/ThemeToggle.vue`
- Modify: `frontend/src/components/GlobalNoticeLayer.vue`
- Modify: `frontend/src/components/projects/ProjectFilters.vue`
- Modify: `frontend/src/components/my-tasks/MyTaskFilters.vue`

- [ ] **Step 1: Restyle the shell/sidebar and theme toggle into the same cyan-violet galaxy language**

```css
background: var(--meta-surface-bg), var(--meta-surface-glow), var(--card-sheen);
box-shadow: var(--meta-surface-shadow);
```

- [ ] **Step 2: Align filters and notice surfaces with the new dark hierarchy**

```css
background: var(--meta-surface-bg), var(--meta-surface-glow), var(--card-sheen);
```

- [ ] **Step 3: Reduce direct hard-coded blur on these utility surfaces**

```css
/* delete blur(16px) usage and prefer token-based or no-blur treatments */
```

### Task 3: Restyle Main Modal/Dialog Surfaces

**Files:**
- Modify: `frontend/src/components/projects/ProjectFormDialog.vue`
- Modify: `frontend/src/components/my-tasks/MyTaskProgressDialog.vue`
- Modify: `frontend/src/components/subtasks/SubtaskFormDialog.vue`
- Modify: `frontend/src/components/subtasks/SubtaskProgressDialog.vue`

- [ ] **Step 1: Keep modal depth but switch them to the galaxy dialog surface stack**

```css
background: var(--dialog-surface-glow), var(--dialog-surface-bg), var(--card-sheen);
box-shadow: var(--dialog-surface-shadow);
```

- [ ] **Step 2: Tone down excessive auxiliary blur layers where possible without flattening the dialogs**

```css
filter: blur(12px);
```

- [ ] **Step 3: Run the targeted tests**

Run: `npm test -- --run src/components/__tests__/ProjectFilters.spec.ts src/components/__tests__/GlobalNoticeLayer.spec.ts src/components/__tests__/ProjectFormDialog.spec.ts src/components/__tests__/SubtaskDialogs.spec.ts src/layouts/__tests__/AppShell.spec.ts`
Expected: PASS

### Task 4: Verify Full Frontend Stability

**Files:**
- No new files

- [ ] **Step 1: Run the full frontend test suite**

Run: `npm test`
Expected: PASS

- [ ] **Step 2: Run the production build**

Run: `npm run build`
Expected: PASS

- [ ] **Step 3: Review the scoped diff**

Run: `git diff -- docs/superpowers/plans/2026-04-14-dark-theme-galaxy-phase-2.md frontend/src/layouts/AppShell.vue frontend/src/components/ThemeToggle.vue frontend/src/components/GlobalNoticeLayer.vue frontend/src/components/projects/ProjectFilters.vue frontend/src/components/my-tasks/MyTaskFilters.vue frontend/src/components/projects/ProjectFormDialog.vue frontend/src/components/my-tasks/MyTaskProgressDialog.vue frontend/src/components/subtasks/SubtaskFormDialog.vue frontend/src/components/subtasks/SubtaskProgressDialog.vue frontend/src/components/__tests__/ProjectFilters.spec.ts frontend/src/components/__tests__/GlobalNoticeLayer.spec.ts frontend/src/components/__tests__/ProjectFormDialog.spec.ts frontend/src/components/__tests__/SubtaskDialogs.spec.ts frontend/src/layouts/__tests__/AppShell.spec.ts`
Expected: Diff only covers phase-2 galaxy dark-theme surfaces.
