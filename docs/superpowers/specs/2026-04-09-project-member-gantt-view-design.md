# Project Member Gantt View Design

**Date:** 2026-04-09

**Scope:** Extend the existing project detail gantt area with a second `人员 / 时间` timeline view, while keeping the current `阶段 / 时间` gantt and workspace behavior intact.

## Goal

Add a member-oriented gantt view inside the current project detail page so users can inspect workload and schedule distribution by employee rather than by stage.

This design builds on the existing gantt implementation and keeps the current structure:

- top-level page switch remains:
  - `工作区`
  - `甘特图`
- gantt-level content gains a second perspective:
  - `阶段 / 时间`
  - `人员 / 时间`

The member gantt in this phase focuses on:

- member rows on the left
- time axis on the top
- subtask bars grouped by responsible member
- overlap-safe rendering for members with concurrent subtasks
- the same `天 / 周 / 月` scale behavior already used by the stage gantt

This phase does not include:

- clicking subtask bars
- hover detail cards for member subtasks
- drag-to-edit timeline dates
- member load analytics beyond direct visual scheduling

## Approved Direction

### Page and View Structure

The member gantt does not become a new route and does not become a new page-level tab.

It lives inside the existing gantt workspace as a second gantt perspective.

Layout order:

- top: existing project overview card
- below overview: existing page-level switcher
  - `工作区`
  - `甘特图`
- inside the gantt card toolbar area:
  - gantt perspective switcher
    - `阶段 / 时间`
    - `人员 / 时间`
  - shared time scale controls
    - `天`
    - `周`
    - `月`

Default gantt perspective remains `阶段 / 时间` to preserve the current entry path and avoid surprising users.

### Member Gantt Main Layout

The member gantt uses the same large card container and visual language as the current gantt views.

The card has three main areas:

- toolbar
- fixed left column for member rows
- scrollable timeline area

The visual structure is:

- top horizontal axis: time scale
- left vertical axis: one member per grouped row
- right chart body: one or more subtask lanes inside each member row

Each member row maps to one item from:

- `GET /api/projects/{project_id}/gantt/members`

Each row shows:

- member real name
- optional lightweight count of assigned subtasks in the current payload
- one timeline area containing that member's subtasks

### Overlapping Subtasks

The key interaction decision for this design is how to handle multiple subtasks assigned to the same member during overlapping time ranges.

Approved behavior:

- a member is represented as one grouped row
- the row may contain multiple internal lanes
- subtasks are automatically packed into the first available non-overlapping lane
- if a member has several overlapping subtasks, those bars are stacked vertically inside that member row
- the member row height grows to fit all required lanes

This avoids visual collisions and keeps each subtask bar directly visible.

### Scroll Model

The design avoids local per-member scrolling.

Approved behavior:

- the gantt body uses a fixed-height main viewport
- vertical scrolling happens at the whole gantt body level
- horizontal scrolling happens in the timeline area
- the left member column remains fixed
- the top time axis remains visible

This means:

- large member counts do not make the full page endlessly taller
- members with many overlapping subtasks can still fully expand within their row
- the overall interaction model stays consistent with the existing gantt workspace

### Toolbar Behavior

The member gantt toolbar includes:

- title: `人员甘特图`
- short description explaining the member schedule perspective
- perspective switcher:
  - `阶段 / 时间`
  - `人员 / 时间`
- scale controls:
  - `天`
  - `周`
  - `月`
- zoom buttons, if the current gantt toolbar keeps them

Changing scale only re-renders the visual timeline and does not reload data.

Changing between `阶段 / 时间` and `人员 / 时间` reuses cached data when available.

### Member Row Visual Rules

Each member row shows the responsible member as the grouping label and renders every assigned subtask as a bar positioned by:

- `planned_start_date`
- `planned_end_date`

Bar tones follow the existing semantic color system:

- `已完成`: muted gray-toned bar
- `未开始`: neutral toned bar
- `进行中`: primary blue-to-cyan semantic bar
- `已延期`: warning-toned bar

Bar labels show:

- subtask name only

Because this phase is intentionally display-first, bars do not respond to click and do not open dialogs or detail cards yet.

## Data Flow

### Member Gantt Data Source

When the user switches to `人员 / 时间` inside the gantt area:

- request `GET /api/projects/{project_id}/gantt/members` on first entry
- cache the response at the page-view level
- reuse cached data when switching between gantt perspectives

This keeps the member gantt decoupled from:

- project stage gantt data
- node subtask gantt data

### Response Mapping

The response shape is:

- `project`
- `member_rows`
  - `user_id`
  - `real_name`
  - `subtasks`

Each subtask item provides:

- `subtask_id`
- `name`
- `node_id`
- `node_name`
- `status`
- `progress_percent`
- `planned_start_date`
- `planned_end_date`
- `completed_at`

The member gantt uses:

- `real_name` for the fixed left column
- `subtasks` for timeline bars
- project-level planned range for axis generation

### Refresh Boundaries

- switching scale: UI-only recalculation
- switching between gantt perspectives: no project detail reload
- workspace mutations that currently invalidate gantt cache should also invalidate member gantt cache

This keeps all gantt perspectives aligned after stage or subtask data changes.

## Component Boundaries

Recommended frontend split:

- `ProjectDetailView`
  - owns page-level `工作区 / 甘特图` mode
  - owns gantt perspective state
  - owns stage/member gantt caches
  - decides which gantt perspective component to render
- `ProjectGanttView`
  - keeps the current `阶段 / 时间` gantt
- `ProjectMemberGanttView`
  - new component for `人员 / 时间`
  - renders member rows, overlap lanes, and timeline bars
- `GanttScaleSwitcher`
  - remains shared between gantt perspectives
- shared gantt helpers
  - axis item generation
  - bar position calculation
  - lane packing for overlapping bars

This keeps the current stage gantt stable and prevents `ProjectGanttView` from becoming a mixed multi-purpose component.

## Error Handling

Member gantt load failure:

- only affects the `人员 / 时间` perspective
- shows inline error state inside the main gantt card
- keeps `阶段 / 时间` and `工作区` available
- provides retry action

Empty states:

- if the project has no members, show an empty member gantt state
- if members exist but have no assigned subtasks, show member rows with empty timeline tracks or an explicit empty-state card, whichever matches the existing gantt visual language more cleanly during implementation

## Testing

Add tests for:

- gantt perspective switcher defaults to `阶段 / 时间`
- switching to `人员 / 时间` lazily loads member gantt data
- member gantt renders fixed left member rows and matching time-axis cells
- overlapping subtasks for one member are packed into separate internal lanes
- member gantt keeps the fixed-height scrolling container model
- switching scale updates axis and bar widths without refetching
- member gantt errors stay isolated from stage gantt errors

## Out Of Scope

- click behavior on member subtasks
- hover detail cards for member subtasks
- right-side task detail drawers
- member capacity metrics, utilization percentages, or warning badges
- drag-to-reassign subtasks
- drag-to-edit subtask time ranges
