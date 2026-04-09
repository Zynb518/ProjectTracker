# Project Detail Gantt View Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a `工作区 / 甘特图` dual-view experience to the project detail page, with stage gantt overview and on-demand node subtask gantt overlay, without breaking the existing workspace flow.

**Architecture:** Keep `ProjectDetailView` as the page owner for view mode, gantt cache, and overlay state. Add focused gantt components plus shared date-to-timeline helpers, and reuse existing notification/error handling patterns. Keep the current workspace store untouched so the existing node rail, drawer, and member panel remain stable.

**Tech Stack:** Vue 3, TypeScript, Pinia, Vitest, Vue Testing Library

---

### Task 1: Add gantt types, helpers, and API clients

**Files:**
- Create: `frontend/src/types/gantt.ts`
- Create: `frontend/src/utils/gantt.ts`
- Create: `frontend/src/utils/__tests__/gantt.spec.ts`
- Modify: `frontend/src/api/projects.ts`
- Modify: `frontend/src/api/nodes.ts`

- [ ] Define dedicated gantt response types for project-level stage gantt and node-level subtask gantt.
- [ ] Write helper tests first for:
  - timeline scale labels (`day / week / month`)
  - timeline cell generation
  - bar offset and width calculation
  - empty/single-day safety cases
- [ ] Run: `npm test -- src/utils/__tests__/gantt.spec.ts`
- [ ] Implement the minimal gantt helpers and API functions to satisfy the tests.
- [ ] Re-run: `npm test -- src/utils/__tests__/gantt.spec.ts`

### Task 2: Build gantt UI components

**Files:**
- Create: `frontend/src/components/workspace/GanttScaleSwitcher.vue`
- Create: `frontend/src/components/workspace/ProjectGanttView.vue`
- Create: `frontend/src/components/workspace/NodeGanttDialog.vue`
- Create: `frontend/src/components/workspace/__tests__/ProjectGanttView.spec.ts`

- [ ] Write failing component tests first for:
  - rendering fixed left rows plus time-axis header
  - semantic stage bar styles
  - scale switching events
  - stage click emitting a node open event
  - node dialog rendering subtask timeline bars
- [ ] Run: `npm test -- src/components/workspace/__tests__/ProjectGanttView.spec.ts`
- [ ] Implement the minimal gantt components with the existing visual language:
  - glass surface
  - timeline grid
  - semantic status bars
  - animated floating overlay
- [ ] Re-run: `npm test -- src/components/workspace/__tests__/ProjectGanttView.spec.ts`

### Task 3: Integrate gantt view into project detail page

**Files:**
- Modify: `frontend/src/views/ProjectDetailView.vue`
- Modify: `frontend/src/views/__tests__/ProjectDetailView.spec.ts`

- [ ] Write failing integration tests first for:
  - defaulting to `工作区`
  - switching to `甘特图` lazily loading project gantt data
  - clicking a gantt stage loading node subtask gantt in an overlay
  - preserving the existing workspace card behavior when the workspace tab remains active
- [ ] Run: `npm test -- src/views/__tests__/ProjectDetailView.spec.ts`
- [ ] Add page-level view mode state, lazy gantt loading, overlay state, and local gantt cache invalidation.
- [ ] Keep existing workspace rendering and mutation flows intact.
- [ ] Re-run: `npm test -- src/views/__tests__/ProjectDetailView.spec.ts`

### Task 4: Verify feature safety

**Files:**
- Modify as needed: any touched frontend source or test files above

- [ ] Run targeted regression suite:
  - `npm test -- src/utils/__tests__/gantt.spec.ts src/components/workspace/__tests__/ProjectGanttView.spec.ts src/views/__tests__/ProjectDetailView.spec.ts src/components/workspace/__tests__/NodeRail.spec.ts src/components/workspace/__tests__/ProjectHero.spec.ts`
- [ ] Run full suite:
  - `npm test`
- [ ] Run production build:
  - `npm run build`
- [ ] If verification stays green, keep the implementation ready for visual review in the existing dev server flow.
