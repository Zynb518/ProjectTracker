# Project Detail Gantt View Design

**Date:** 2026-04-09

**Scope:** Add a second main view to the project detail page for gantt-based timeline browsing, without changing the existing workspace behavior.

## Goal

Extend the current project detail page so users can switch between:

- the existing `工作区` view
- a new `甘特图` view

The gantt view in phase one focuses on:

- project stage/time gantt
- node-level drill-down into subtask/time gantt
- timeline zoom from `天` to `月`
- hover and click interactions

Phase one does not include member gantt and does not include drag-to-save date editing.

## Approved Direction

### Page-Level View Switch

The gantt view lives inside the existing project detail page rather than becoming a separate route.

Layout order:

- top: existing project overview card
- below overview: a two-state view switcher
  - `工作区`
  - `甘特图`
- main content area:
  - `工作区`: keep the current node rail + node drawer + member panel layout unchanged
  - `甘特图`: replace the lower area with a dedicated full-width gantt workspace

This keeps the current detail workflow intact while giving the gantt chart enough horizontal space for timeline interaction.

### Gantt Main Layout

The gantt view is a single large card with three areas:

- toolbar
- fixed left column for stage rows
- scrollable timeline area

The visual structure is:

- top horizontal axis: time scale
- left vertical axis: one stage per row
- right chart body: visible time interval bars aligned to each stage row

Each stage row maps to one item from `GET /api/projects/{project_id}/gantt/nodes`.

The gantt main card does not show the member panel.

### Toolbar Behavior

The gantt toolbar includes:

- title: `阶段甘特图`
- scale controls:
  - `天`
  - `周`
  - `月`
- zoom icon buttons:
  - zoom in
  - zoom out

Scale limits:

- minimum granularity: `天`
- maximum granularity: `月`

Changing scale re-renders the timeline header and bar width mapping, but does not reload data.

### Stage Row Behavior

Each stage row includes:

- stage name
- stage status
- one timeline bar showing `planned_start_date` to `planned_end_date`

Visual rules:

- `已完成`: gray-toned bar
- `未开始 / 进行中 / 已延期`: use semantic status color

Interaction rules:

- hover a stage bar:
  - show a lightweight hover card
  - card content:
    - stage name
    - status
    - planned period
    - sequence number
    - completed time when available
- click a stage bar:
  - open a floating subtask gantt window

All stage rows remain viewable in the main gantt. Completed stages are gray but still clickable for historical review in the gantt context.

### Subtask Gantt Window

Clicking a stage opens a floating overlay window above the gantt view.

The floating window shows the node-specific gantt from `GET /api/projects/{project_id}/nodes/{node_id}/gantt`.

Window layout:

- header:
  - node name
  - node status
  - node planned period
  - close action
- body:
  - left column: subtask names
  - top axis: time scale
  - right chart body: one subtask timeline bar per row

Each subtask row shows:

- subtask name
- responsible user name in hover content
- status via bar tone
- progress percent in hover content
- planned time interval via the bar itself

Interaction rules:

- hover a subtask bar:
  - show subtask summary tooltip
- click a subtask bar:
  - reserved for future extension
  - phase one does not trigger subtask edit or progress mutation

The subtask gantt window uses the same `天 / 周 / 月` scale as the main gantt view to keep mental context stable.

## Data Flow

### Main Gantt Data

When the user switches to `甘特图`:

- load `GET /api/projects/{project_id}/gantt/nodes` on first entry
- cache the result in the page/store scope
- reuse cached data when the user switches between `工作区` and `甘特图`

The main gantt view does not request subtask gantt data up front.

### Node Drill-Down Data

When the user clicks a stage bar:

- open the floating window shell immediately
- request `GET /api/projects/{project_id}/nodes/{node_id}/gantt`
- render loading state inside the floating window
- replace loading state with subtask gantt after success

This keeps the main gantt fast and matches the backend API split by level.

### Refresh Boundaries

- switching scale: local UI-only update
- switching between `工作区` and `甘特图`: no forced project detail reload
- opening a node subtask gantt: only fetch that node's gantt payload

## Component Boundaries

Recommended frontend split:

- `ProjectDetailView`
  - owns the page-level view mode
  - hosts the switcher
  - decides whether to show workspace or gantt
- `ProjectGanttView`
  - owns the main gantt card
  - loads and renders project stage/time gantt
- `GanttScaleSwitcher`
  - owns `天 / 周 / 月` state switching UI
- `NodeGanttDialog`
  - floating window for single-stage subtask gantt
- shared gantt helpers
  - date range normalization
  - visible time-axis generation
  - bar position calculation
  - status-to-style mapping

This keeps gantt logic isolated from the existing node rail and node drawer implementation.

## Error Handling

Main gantt load failure:

- only affects the `甘特图` view
- show inline error state inside the gantt card
- keep `工作区` view usable
- provide retry action

Subtask gantt load failure:

- keep the floating window open
- replace content with an error state
- provide retry action for the same node

Empty states:

- if the project has no stages, show an empty gantt state
- if the selected stage has no subtasks, show an empty subtask gantt state

## Testing

Add tests for:

- project detail view switcher keeps current workspace path unchanged
- gantt view loads stage gantt only when selected
- day/week/month scale switching updates gantt header and bar layout
- clicking a stage opens the floating subtask gantt window
- subtask gantt uses on-demand API loading
- main gantt error state and subtask gantt error state are isolated

## Out Of Scope

- project member gantt view
- drag-to-edit stage dates
- drag-to-edit subtask dates
- dependency lines between stages or subtasks
- automatic conflict detection
- separate gantt route or standalone page
