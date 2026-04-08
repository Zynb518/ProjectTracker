# Project Detail Top Card Redesign Design

**Date:** 2026-04-08

**Scope:** Redesign only the top summary area of the project detail page. The lower workspace layout stays unchanged for this iteration.

## Goal

Replace the current split `ProjectHero + WorkspaceStats` presentation on the project detail page with a single, high-information-density hero card that presents the project's overview more clearly and with stronger visual hierarchy.

## Current Context

The current project detail page renders:

- `ProjectHero.vue` for project name, description, status, owner, and period
- `WorkspaceStats.vue` for member count, node progress, and subtask progress
- the main workspace area below for nodes, subtasks, and members

This spreads the summary across two separate panels and weakens the sense that the top area is one coherent overview block.

## Approved Direction

The new top section will merge the existing summary information into one large overview card.

### Information Architecture

The card is split into two columns on desktop:

- Left column:
  - project name
  - project description
  - one summary metric for member count
  - two circular progress visuals:
    - stage node progress
    - subtask progress
- Right column:
  - one combined information panel
  - rows for:
    - project status
    - owner
    - planned period

On narrow screens, the card stacks vertically:

- summary content first
- combined metadata panel below

### Metric Presentation

The approved visual for `阶段节点` and `子任务` is dual circular progress indicators.

Each progress indicator must show:

- label
- completed count / total count inside the ring
- strong distinction between completed and remaining portions

The member metric remains a compact numeric summary card instead of a chart.

### Right-Side Metadata Panel

The approved layout for the right side is one merged panel, not three separate cards.

The panel uses a stacked row layout:

- row 1: status
- row 2: owner
- row 3: planned period

Status remains visually emphasized with a colored pill.

The period is shown across two lines to avoid horizontal crowding:

- planned start date
- planned end date

## Visual Design Requirements

The new hero card should stay consistent with the current frontend direction:

- technology-oriented
- translucent / glass-like
- dark and light theme compatible
- higher contrast than the current project detail header
- clear distinction between the main hero surface and the internal metric blocks

Specific visual requirements:

- the large card is one dominant container
- internal metric blocks are secondary surfaces
- circular progress elements should read like dashboard instrumentation, not analytics charts
- status pill keeps semantic color
- typography should prioritize the project name and progress counts

## Component Boundaries

This redesign should stay focused and avoid unnecessary routing or data changes.

Expected component changes:

- `frontend/src/views/ProjectDetailView.vue`
  - replace the separate `ProjectHero` and `WorkspaceStats` composition with one top-card composition
- `frontend/src/components/workspace/ProjectHero.vue`
  - refactor into the new merged overview card, or replace with a new focused top-card component if that keeps responsibilities clearer
- `frontend/src/components/workspace/WorkspaceStats.vue`
  - remove from this page if its responsibilities are absorbed into the new hero card
- related tests under `frontend/src/views/__tests__/ProjectDetailView.spec.ts`
  - update expectations to match the new merged summary structure

## Data Flow

No backend or API changes are needed.

The new top card must continue using existing project detail fields already loaded by `useProjectWorkspaceStore`:

- `name`
- `description`
- `status`
- `owner_real_name`
- `planned_start_date`
- `planned_end_date`
- `member_count`
- `node_count`
- `completed_node_count`
- `sub_task_count`
- `completed_sub_task_count`

## Error Handling

Existing error behavior remains unchanged:

- workspace load failures still surface through the global notification store
- no new retry logic or extra request layer is introduced

## Responsive Behavior

Desktop:

- large two-column hero card
- left side visually dominant
- right metadata panel fixed-width or constrained-width

Mobile / narrow tablet:

- single-column stacked layout
- project name and description first
- metrics after description
- metadata panel after metrics

The circular progress indicators may reduce in size on smaller screens, but should remain legible.

## Testing Strategy

This redesign needs test coverage for:

- the top section rendering as one integrated summary block
- project detail view no longer depending on a separate stats strip at the top
- presence of member summary, node ring summary, and subtask ring summary
- metadata panel rendering status, owner, and period inside one grouped block
- responsive-safe structure at the component markup/class level where practical

## Out of Scope

The following are intentionally not part of this redesign:

- lower node / subtask / member workspace layout changes
- backend API changes
- permission logic changes
- task table redesign
- node rail redesign
- member panel redesign

## Self-Review

Checked:

- no placeholders left
- scope limited to the top summary area only
- data fields align with current `ProjectDetail` usage
- no contradiction with the decision to keep the lower workspace untouched for now
