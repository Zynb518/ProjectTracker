# Project Detail Top Card Redesign Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the split project detail summary (`ProjectHero + WorkspaceStats`) with one merged top card that shows project overview, member count, dual circular progress, and a right-side combined metadata panel.

**Architecture:** Keep `ProjectDetailView` data flow unchanged and refactor the top summary rendering into a single focused workspace component. Reuse existing `ProjectDetail` fields, remove the separate stats strip from the detail page, and verify the new structure through view-level and component-level tests before styling polish.

**Tech Stack:** Vue 3, TypeScript, Pinia, Vue Testing Library, Vitest, Vite

---

### Task 1: Lock The New Top Card Structure In Tests

**Files:**
- Modify: `frontend/src/views/__tests__/ProjectDetailView.spec.ts`
- Create: `frontend/src/components/workspace/__tests__/ProjectHero.spec.ts`

- [ ] **Step 1: Write the failing view-level test for the merged top card**

```ts
it('renders a single merged top overview card with member, node, subtask, and metadata summaries', async () => {
  const screen = render(ProjectDetailView, {
    global: {
      plugins: [createPinia()],
    },
  })

  expect(await screen.findByTestId('project-overview-card')).toBeTruthy()
  expect(screen.queryByText('Project Workspace')).toBeNull()
  expect(screen.queryByText('成员')).toBeTruthy()
  expect(screen.queryByText('阶段节点')).toBeTruthy()
  expect(screen.queryByText('子任务')).toBeTruthy()
  expect(screen.queryByText('状态')).toBeTruthy()
  expect(screen.queryByText('负责人')).toBeTruthy()
  expect(screen.queryByText('周期')).toBeTruthy()
})
```

- [ ] **Step 2: Run the detail view test to verify it fails**

Run: `npm test -- --run src/views/__tests__/ProjectDetailView.spec.ts`
Expected: FAIL because the merged overview card structure and test ids do not exist yet.

- [ ] **Step 3: Write the failing component-level test for dual circular progress and merged metadata**

```ts
it('renders member summary, two circular progress indicators, and one grouped metadata panel', () => {
  const screen = render(ProjectHero, {
    props: {
      project: sampleProject,
    },
  })

  expect(screen.getByTestId('project-overview-card')).toBeTruthy()
  expect(screen.getByTestId('member-summary')).toHaveTextContent('5')
  expect(screen.getByTestId('node-progress-ring')).toHaveTextContent('1 / 3')
  expect(screen.getByTestId('subtask-progress-ring')).toHaveTextContent('4 / 12')
  expect(screen.getByTestId('project-meta-panel')).toBeTruthy()
})
```

- [ ] **Step 4: Run the component test to verify it fails**

Run: `npm test -- --run src/components/workspace/__tests__/ProjectHero.spec.ts`
Expected: FAIL because the component test file or target markup does not exist yet.

- [ ] **Step 5: Commit**

```bash
git add frontend/src/views/__tests__/ProjectDetailView.spec.ts frontend/src/components/workspace/__tests__/ProjectHero.spec.ts
git commit -m "test: cover merged project detail overview card"
```

### Task 2: Implement The Merged Overview Card

**Files:**
- Modify: `frontend/src/views/ProjectDetailView.vue`
- Modify: `frontend/src/components/workspace/ProjectHero.vue`
- Optionally modify: `frontend/src/components/workspace/WorkspaceStats.vue`

- [ ] **Step 1: Refactor the detail view template to use only the merged top card**

```vue
<template>
  <section class="project-detail">
    <p v-if="isLoading && !project" class="project-detail__state loading-panel">工作台加载中...</p>

    <template v-else-if="project">
      <p v-if="isLoading" aria-live="polite" class="project-detail__refreshing">工作台刷新中...</p>

      <ProjectHero :project="project" />

      <div class="project-detail__grid">
        <!-- existing node rail / subtask / member layout stays unchanged -->
      </div>
    </template>
  </section>
</template>
```

- [ ] **Step 2: Implement the new merged `ProjectHero` markup with explicit test hooks**

```vue
<template>
  <section class="project-overview-card" data-testid="project-overview-card">
    <div class="project-overview-card__content">
      <div class="project-overview-card__lead">
        <p class="project-overview-card__eyebrow">Project Overview</p>
        <h2>{{ project.name }}</h2>
        <p class="project-overview-card__description">
          {{ project.description || '当前项目未填写描述。' }}
        </p>
      </div>

      <div class="project-overview-card__metrics">
        <article class="project-overview-card__metric" data-testid="member-summary">
          <span>成员</span>
          <strong>{{ project.member_count }}</strong>
        </article>

        <article class="project-overview-card__progress" data-testid="node-progress-ring">
          <span>阶段节点</span>
          <strong>{{ project.completed_node_count }} / {{ project.node_count }}</strong>
        </article>

        <article class="project-overview-card__progress" data-testid="subtask-progress-ring">
          <span>子任务</span>
          <strong>{{ project.completed_sub_task_count }} / {{ project.sub_task_count }}</strong>
        </article>
      </div>
    </div>

    <aside class="project-overview-card__meta" data-testid="project-meta-panel">
      <!-- merged status / owner / period rows -->
    </aside>
  </section>
</template>
```

- [ ] **Step 3: Add the circular progress calculation helpers**

```ts
function progressValue(completed: number, total: number) {
  if (total <= 0) {
    return 0
  }

  return Math.max(0, Math.min(100, Math.round((completed / total) * 100)))
}
```

- [ ] **Step 4: Style the merged top card with two-column layout and dashboard-like rings**

```css
.project-overview-card {
  display: grid;
  grid-template-columns: minmax(0, 1.2fr) minmax(240px, 280px);
}

.project-overview-card__ring {
  background: conic-gradient(var(--accent-end) calc(var(--progress) * 1%), rgba(255, 255, 255, 0.08) 0);
}
```

- [ ] **Step 5: Run the targeted tests to verify the implementation passes**

Run: `npm test -- --run src/components/workspace/__tests__/ProjectHero.spec.ts src/views/__tests__/ProjectDetailView.spec.ts`
Expected: PASS

- [ ] **Step 6: Commit**

```bash
git add frontend/src/views/ProjectDetailView.vue frontend/src/components/workspace/ProjectHero.vue frontend/src/components/workspace/WorkspaceStats.vue
git commit -m "feat: redesign project detail top overview card"
```

### Task 3: Verify Full Frontend Stability

**Files:**
- No new files

- [ ] **Step 1: Run the full frontend test suite**

Run: `npm test`
Expected: PASS with all tests green.

- [ ] **Step 2: Run the production build**

Run: `npm run build`
Expected: PASS and output generated in `dist/`.

- [ ] **Step 3: Review the diff for scope**

Run: `git diff -- frontend/src/views/ProjectDetailView.vue frontend/src/components/workspace/ProjectHero.vue frontend/src/components/workspace/WorkspaceStats.vue frontend/src/views/__tests__/ProjectDetailView.spec.ts frontend/src/components/workspace/__tests__/ProjectHero.spec.ts`
Expected: Diff only covers the approved top-card redesign scope.

- [ ] **Step 4: Commit**

```bash
git add frontend/src/views/ProjectDetailView.vue frontend/src/components/workspace/ProjectHero.vue frontend/src/components/workspace/WorkspaceStats.vue frontend/src/views/__tests__/ProjectDetailView.spec.ts frontend/src/components/workspace/__tests__/ProjectHero.spec.ts
git commit -m "test: verify project detail top card redesign"
```
