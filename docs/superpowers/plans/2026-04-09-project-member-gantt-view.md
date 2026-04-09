# Project Member Gantt View Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a `人员 / 时间` gantt perspective inside the existing project detail gantt workspace, with overlap-safe member rows and lazy API loading, without breaking the current stage gantt flow.

**Architecture:** Keep `ProjectDetailView` as the page owner for gantt mode, perspective state, and per-perspective caches. Reuse the existing gantt axis and bar layout helpers, extend them with member-row lane packing, and render the new perspective in a dedicated `ProjectMemberGanttView` component so the current `ProjectGanttView` stays focused on stages.

**Tech Stack:** Vue 3, TypeScript, Pinia, Vitest, Vue Testing Library

---

### Task 1: Add member gantt types and API support

**Files:**
- Modify: `frontend/src/types/gantt.ts`
- Modify: `frontend/src/api/projects.ts`

- [ ] Add typed member-gantt response interfaces beside the existing stage/node gantt types.
- [ ] Expose `getProjectGanttMembers(projectId)` from `frontend/src/api/projects.ts`.
- [ ] Keep naming aligned with backend response fields from `/api/projects/{project_id}/gantt/members`.

### Task 2: Extend gantt helpers for overlap-safe member lanes

**Files:**
- Modify: `frontend/src/utils/gantt.ts`
- Modify: `frontend/src/utils/__tests__/gantt.spec.ts`

- [ ] Write failing tests for:
  - packing overlapping tasks into separate lanes
  - reusing the same lane when tasks do not overlap
  - preserving deterministic lane order from the input sequence
- [ ] Run: `npm test -- src/utils/__tests__/gantt.spec.ts`
- [ ] Implement the minimal lane-packing helper in `frontend/src/utils/gantt.ts`.
- [ ] Re-run: `npm test -- src/utils/__tests__/gantt.spec.ts`

### Task 3: Build the member gantt component

**Files:**
- Create: `frontend/src/components/workspace/ProjectMemberGanttView.vue`
- Modify: `frontend/src/components/workspace/__tests__/ProjectGanttView.spec.ts`

- [ ] Write failing component tests for:
  - rendering fixed member rows and matching time-axis cells
  - placing overlapping subtasks into separate internal lanes for one member row
  - using the fixed-height internal scrolling model instead of stretching the page
  - keeping bars display-only with no click emission requirement
- [ ] Run: `npm test -- src/components/workspace/__tests__/ProjectGanttView.spec.ts`
- [ ] Implement `ProjectMemberGanttView.vue` with:
  - the existing dark/light blue visual language
  - a card-level toolbar with title, description, and scale switcher
  - fixed left member column
  - internal vertical scrolling viewport
  - horizontal timeline scrolling
  - semantic subtask bars grouped by member
- [ ] Re-run: `npm test -- src/components/workspace/__tests__/ProjectGanttView.spec.ts`

### Task 4: Integrate member perspective into project detail gantt mode

**Files:**
- Modify: `frontend/src/views/ProjectDetailView.vue`
- Modify: `frontend/src/views/__tests__/ProjectDetailView.spec.ts`

- [ ] Write failing integration tests for:
  - defaulting gantt perspective to `阶段 / 时间`
  - lazily loading member gantt data only after switching to `人员 / 时间`
  - keeping stage gantt behavior and node gantt overlay intact
  - invalidating member gantt cache with the existing gantt refresh boundary
- [ ] Run: `npm test -- src/views/__tests__/ProjectDetailView.spec.ts`
- [ ] Add perspective state, member gantt cache, API loading, retry handling, and conditional rendering.
- [ ] Keep the current workspace and stage gantt flows unchanged.
- [ ] Re-run: `npm test -- src/views/__tests__/ProjectDetailView.spec.ts`

### Task 5: Verify the full frontend remains safe

**Files:**
- Modify as needed: any touched frontend source or test files above

- [ ] Run targeted regression suite:
  - `npm test -- src/utils/__tests__/gantt.spec.ts src/components/workspace/__tests__/ProjectGanttView.spec.ts src/views/__tests__/ProjectDetailView.spec.ts`
- [ ] Run full suite:
  - `npm test`
- [ ] Run production build:
  - `npm run build`
