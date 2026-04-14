# Login Background Removal Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Remove the login page's bitmap background and leave the page on a plain theme-colored surface while preserving all login behavior.

**Architecture:** Keep the current login layout intact and confine the change to `LoginView.vue` plus its view test. Delete the image-specific setup, retain one stable background container, and let an existing solid theme token provide the background color in both light and dark modes.

**Tech Stack:** Vue 3, TypeScript, Pinia, Vue Testing Library, Vitest, Vite

---

### Task 1: Lock The New Login Background Behavior In Tests

**Files:**
- Modify: `frontend/src/views/__tests__/LoginView.spec.ts`

- [ ] **Step 1: Rewrite the source-level background test to assert there are no login image imports**

```ts
it('defines a plain login background surface without image asset imports or inline background-image binding', () => {
  const source = readFileSync(resolve(process.cwd(), 'src/views/LoginView.vue'), 'utf8')

  expect(source).not.toContain("import skyImage from '@/assets/login/sky.png'")
  expect(source).not.toContain("import cat3Image from '@/assets/login/cat3.png'")
  expect(source).not.toContain('new Image()')
  expect(source).not.toContain('backgroundImage: activeBackgroundImage')
  expect(source).toContain('data-testid="login-background"')
})
```

- [ ] **Step 2: Remove the theme-toggle image-switching test**

```ts
// Delete the test that asserts the background style changes from sky to cat3.
```

- [ ] **Step 3: Run the targeted login view test**

Run: `npm test -- --run src/views/__tests__/LoginView.spec.ts`
Expected: FAIL because `LoginView.vue` still imports image assets and binds `backgroundImage`.

- [ ] **Step 4: Commit**

```bash
git add frontend/src/views/__tests__/LoginView.spec.ts
git commit -m "test: update login view background expectations"
```

### Task 2: Remove Login Image Background Logic

**Files:**
- Modify: `frontend/src/views/LoginView.vue`
- Test: `frontend/src/views/__tests__/LoginView.spec.ts`

- [ ] **Step 1: Remove unused image and lifecycle imports**

```ts
import { ref } from 'vue'
```

- [ ] **Step 2: Delete the image assets, theme-computed background, and preload loop**

```ts
const username = ref('')
const password = ref('')
```

- [ ] **Step 3: Simplify the background markup to a single plain surface**

```vue
<section class="login-view">
  <div aria-hidden="true" class="login-view__background" data-testid="login-background" />

  <header class="login-view__toolbar">
    <p class="login-view__brand">Project Tracker</p>
    <ThemeToggle />
  </header>
```

- [ ] **Step 4: Replace image/scrim styling with a plain color fill**

```css
.login-view__background {
  position: absolute;
  inset: 0;
  pointer-events: none;
  z-index: 0;
  background: var(--panel-bg);
}
```

- [ ] **Step 5: Run the targeted login view test**

Run: `npm test -- --run src/views/__tests__/LoginView.spec.ts`
Expected: PASS

- [ ] **Step 6: Commit**

```bash
git add frontend/src/views/LoginView.vue frontend/src/views/__tests__/LoginView.spec.ts
git commit -m "feat: remove login image backgrounds"
```

### Task 3: Verify Frontend Stability For This Change

**Files:**
- No new files

- [ ] **Step 1: Run the frontend build**

Run: `npm run build`
Expected: PASS and output generated in `dist/`.

- [ ] **Step 2: Review the scoped diff**

Run: `git diff -- docs/superpowers/specs/2026-04-14-login-background-removal-design.md docs/superpowers/plans/2026-04-14-login-background-removal.md frontend/src/views/LoginView.vue frontend/src/views/__tests__/LoginView.spec.ts`
Expected: Diff only covers the approved login background simplification.

- [ ] **Step 3: Commit**

```bash
git add docs/superpowers/specs/2026-04-14-login-background-removal-design.md docs/superpowers/plans/2026-04-14-login-background-removal.md frontend/src/views/LoginView.vue frontend/src/views/__tests__/LoginView.spec.ts
git commit -m "docs: record login background removal plan"
```
