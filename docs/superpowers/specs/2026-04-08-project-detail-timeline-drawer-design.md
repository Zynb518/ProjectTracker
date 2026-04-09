# Project Detail Timeline Drawer Design

**Date:** 2026-04-08

**Scope:** Redesign only the lower workspace layout of the project detail page.

## Goal

Replace the current node list + always-visible subtask table interaction with:

- a Git-style node timeline on the left
- a drawer-style node details and subtasks surface in the middle
- an always-visible member panel on the right

## Approved Direction

### Layout

Desktop layout remains three columns:

- left: stage node timeline
- middle: drawer host area
- right: member panel

### Node Timeline Behavior

- `已完成` nodes:
  - gray
  - not clickable
  - still visible in the timeline history
  - can still show hover detail
- `未开始 / 进行中 / 已延期` nodes:
  - clickable
  - hover shows node detail
  - click opens the middle-area drawer

### Hover Behavior

Hovering a node shows a lightweight node detail card only while hovered.

The hover card includes:

- node name
- status
- period
- description
- subtask completion summary

### Drawer Behavior

Clicking an interactive node opens a right-sliding drawer anchored inside the middle column.

The drawer contains:

- node overview block:
  - name
  - status
  - period
  - description
- subtask list block:
  - all current subtasks for that node

The member panel stays visible at the right side and is not covered by the drawer.

## Data Flow

- hover detail uses already loaded node data only
- clicking an interactive node reuses the existing `selectNode(projectId, nodeId)` flow
- no backend changes

## Out Of Scope

- member panel redesign
- top overview card redesign
- backend API changes
- node CRUD workflow redesign
