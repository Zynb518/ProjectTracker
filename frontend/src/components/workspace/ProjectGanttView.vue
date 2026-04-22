<script setup lang="ts">
import { computed, onBeforeUnmount, ref } from 'vue'

import GanttScaleSwitcher from '@/components/workspace/GanttScaleSwitcher.vue'
import type {
  GanttNodeResizePayload,
  GanttNodeSummary,
  GanttPerspective,
  GanttScale,
  GanttSubtaskResizePayload,
  GanttSubtaskSummary,
  ProjectStageGantt,
} from '@/types/gantt'
import { getWorkStatusLabel, getWorkStatusTone } from '@/utils/display'
import { buildGanttAxisItems, getGanttBarLayout, getPixelsPerDay } from '@/utils/gantt'
import { normalizeWheelDelta } from '@/utils/smoothWheel'

const props = withDefaults(defineProps<{
  canEditSchedule?: boolean
  expandedNodeIds?: number[]
  error?: string | null
  gantt: ProjectStageGantt | null
  isLoading?: boolean
  isExpandingAll?: boolean
  loadingNodeIds?: number[]
  nodeLoadErrors?: Record<number, string | null>
  nodeSubtasksById?: Record<number, GanttSubtaskSummary[]>
  perspective?: GanttPerspective
  scale: GanttScale
  savingNodeIds?: number[]
  savingSubtaskIds?: number[]
}>(), {
  canEditSchedule: false,
  expandedNodeIds: () => [],
  error: null,
  isExpandingAll: false,
  isLoading: false,
  loadingNodeIds: () => [],
  nodeLoadErrors: () => ({}),
  nodeSubtasksById: () => ({}),
  perspective: 'stage',
  savingNodeIds: () => [],
  savingSubtaskIds: () => [],
})

const emit = defineEmits<{
  'collapse-all': []
  'expand-all': []
  'resize-node': [payload: GanttNodeResizePayload]
  'resize-subtask': [payload: GanttSubtaskResizePayload]
  retry: []
  'retry-node': [nodeId: number]
  'toggle-node': [nodeId: number]
  'update:perspective': [perspective: GanttPerspective]
  'update:scale': [scale: GanttScale]
}>()

type HoverCardPlacement = 'bottom-left' | 'bottom-right' | 'top-left' | 'top-right'
type ResizeEdge = 'start' | 'end'
type ResizeTarget =
  | {
      edge: ResizeEdge
      kind: 'node'
      maxDate: string
      minDate: string
      nodeId: number
      originalEndDate: string
      originalStartDate: string
    }
  | {
      edge: ResizeEdge
      kind: 'subtask'
      maxDate: string
      minDate: string
      nodeId: number
      originalEndDate: string
      originalStartDate: string
      subtaskId: number
    }
type ActiveResizeState = ResizeTarget & {
  plannedEndDate: string
  plannedStartDate: string
  pointerStartX: number
}
type VisibleGanttRow =
  | {
      key: string
      node: GanttNodeSummary
      type: 'node'
    }
  | {
      key: string
      node: GanttNodeSummary
      subtask: GanttSubtaskSummary
      type: 'subtask'
    }
  | {
      key: string
      node: GanttNodeSummary
      type: 'loading'
    }
  | {
      error: string
      key: string
      node: GanttNodeSummary
      type: 'error'
    }
  | {
      key: string
      node: GanttNodeSummary
      type: 'empty'
    }

const hoveredNodeId = ref<number | null>(null)
const hoveredNodeAnchor = ref({ x: 0, y: 0 })
const hoveredNodePlacement = ref<HoverCardPlacement>('top-right')
const hoveredSubtaskId = ref<number | null>(null)
const hoveredSubtaskAnchor = ref({ x: 0, y: 0 })
const hoveredSubtaskPlacement = ref<HoverCardPlacement>('top-right')
const activeResize = ref<ActiveResizeState | null>(null)
let showNodeDetailTimer: ReturnType<typeof window.setTimeout> | null = null
let showSubtaskDetailTimer: ReturnType<typeof window.setTimeout> | null = null
let hideNodeDetailTimer: ReturnType<typeof window.setTimeout> | null = null
let hideSubtaskDetailTimer: ReturnType<typeof window.setTimeout> | null = null

const DETAIL_SHOW_DELAY_MS = 1000
const DETAIL_HIDE_DELAY_MS = 180
const DETAIL_CARD_EDGE_GAP_PX = 24
const DETAIL_CARD_HEIGHT_HINT_PX = 220
const DETAIL_CARD_WIDTH_PX = 288
const sidebarRowsRailRef = ref<HTMLElement | null>(null)
const axisScrollRef = ref<HTMLElement | null>(null)
const topScrollRef = ref<HTMLElement | null>(null)
const rowsScrollRef = ref<HTMLElement | null>(null)
const ignoredHorizontalScroll = new WeakMap<HTMLElement, number>()

const axisItems = computed(() =>
  props.gantt === null
    ? []
    : buildGanttAxisItems(
        props.gantt.project.planned_start_date,
        props.gantt.project.planned_end_date,
        props.scale,
      ),
)

const axisStartDate = computed(
  () => axisItems.value[0]?.startDate ?? props.gantt?.project.planned_start_date ?? '',
)

const canvasWidth = computed(() =>
  axisItems.value.reduce((total, item) => total + item.widthPx, 0),
)

const timelineCanvasStyle = computed(() => ({
  width: `${canvasWidth.value}px`,
}))

const expandedNodeIdSet = computed(() => new Set(props.expandedNodeIds))
const loadingNodeIdSet = computed(() => new Set(props.loadingNodeIds))
const savingNodeIdSet = computed(() => new Set(props.savingNodeIds))
const savingSubtaskIdSet = computed(() => new Set(props.savingSubtaskIds))

const visibleRows = computed<VisibleGanttRow[]>(() => {
  if (props.gantt === null) {
    return []
  }

  return props.gantt.nodes.flatMap((node) => {
    const rows: VisibleGanttRow[] = [
      {
        key: `node-${node.id}`,
        node,
        type: 'node',
      },
    ]

    if (!expandedNodeIdSet.value.has(node.id)) {
      return rows
    }

    if (loadingNodeIdSet.value.has(node.id)) {
      rows.push({
        key: `node-loading-${node.id}`,
        node,
        type: 'loading',
      })
      return rows
    }

    const nodeError = props.nodeLoadErrors[node.id]
    if (nodeError) {
      rows.push({
        error: nodeError,
        key: `node-error-${node.id}`,
        node,
        type: 'error',
      })
      return rows
    }

    const subtasks = props.nodeSubtasksById[node.id] ?? []
    if (subtasks.length === 0) {
      rows.push({
        key: `node-empty-${node.id}`,
        node,
        type: 'empty',
      })
      return rows
    }

    return rows.concat(
      subtasks.map((subtask) => ({
        key: `subtask-${subtask.id}`,
        node,
        subtask,
        type: 'subtask' as const,
      })),
    )
  })
})

const hoveredNode = computed(() =>
  props.gantt?.nodes.find((node) => node.id === hoveredNodeId.value) ?? null,
)

const allSubtasks = computed(() =>
  Object.values(props.nodeSubtasksById).flat(),
)

const hoveredSubtask = computed(() =>
  allSubtasks.value.find((subtask) => subtask.id === hoveredSubtaskId.value) ?? null,
)

const detailCardStyle = computed(() => ({
  left: `${hoveredNodeAnchor.value.x}px`,
  top: `${hoveredNodeAnchor.value.y}px`,
}))

const subtaskDetailCardStyle = computed(() => ({
  left: `${hoveredSubtaskAnchor.value.x}px`,
  top: `${hoveredSubtaskAnchor.value.y}px`,
}))

function getScheduleDays(startDate: string, endDate: string) {
  const start = new Date(`${startDate}T00:00:00`)
  const end = new Date(`${endDate}T00:00:00`)

  return Math.max(1, Math.round((end.getTime() - start.getTime()) / 86400000) + 1)
}

function formatCompletedAt(value: string) {
  return value.slice(0, 16).replace('T', ' ')
}

function parseDate(value: string) {
  const [year, month, day] = value.split('-').map(Number)
  return new Date(Date.UTC(year, month - 1, day))
}

function formatDate(value: Date) {
  return value.toISOString().slice(0, 10)
}

function shiftDate(value: string, days: number) {
  const date = parseDate(value)
  date.setUTCDate(date.getUTCDate() + days)
  return formatDate(date)
}

function clampDate(value: string, minDate: string, maxDate: string) {
  if (value < minDate) {
    return minDate
  }

  if (value > maxDate) {
    return maxDate
  }

  return value
}

function getVisibleRange(
  item: GanttNodeSummary | GanttSubtaskSummary,
  kind: 'node' | 'subtask',
) {
  const draft = activeResize.value

  if (draft === null || draft.kind !== kind) {
    return {
      planned_end_date: item.planned_end_date,
      planned_start_date: item.planned_start_date,
    }
  }

  if (kind === 'node' && draft.nodeId === item.id) {
    return {
      planned_end_date: draft.plannedEndDate,
      planned_start_date: draft.plannedStartDate,
    }
  }

  if (kind === 'subtask' && 'subtaskId' in draft && draft.subtaskId === item.id) {
    return {
      planned_end_date: draft.plannedEndDate,
      planned_start_date: draft.plannedStartDate,
    }
  }

  return {
    planned_end_date: item.planned_end_date,
    planned_start_date: item.planned_start_date,
  }
}

function getBarStyle(node: GanttNodeSummary | GanttSubtaskSummary, kind: 'node' | 'subtask') {
  const range = getVisibleRange(node, kind)
  const layout = getGanttBarLayout(
    axisStartDate.value,
    range.planned_start_date,
    range.planned_end_date,
    props.scale,
  )

  return {
    left: `${layout.leftPx}px`,
    width: `${layout.widthPx}px`,
  }
}

function getHoverPoint(event?: FocusEvent | MouseEvent) {
  if (event instanceof MouseEvent && (event.clientX !== 0 || event.clientY !== 0)) {
    return {
      x: event.clientX,
      y: event.clientY,
    }
  }

  const target = event?.currentTarget
  if (target instanceof HTMLElement) {
    const rect = target.getBoundingClientRect()

    return {
      x: rect.right,
      y: rect.top + rect.height / 2,
    }
  }

  return {
    x: window.innerWidth / 2,
    y: window.innerHeight / 2,
  }
}

function getHoverPlacement(anchor: { x: number; y: number }): HoverCardPlacement {
  const shouldFlipHorizontally = window.innerWidth - anchor.x < DETAIL_CARD_WIDTH_PX + DETAIL_CARD_EDGE_GAP_PX
  const shouldFlipVertically = anchor.y < DETAIL_CARD_HEIGHT_HINT_PX + DETAIL_CARD_EDGE_GAP_PX

  if (shouldFlipVertically) {
    return shouldFlipHorizontally ? 'bottom-left' : 'bottom-right'
  }

  return shouldFlipHorizontally ? 'top-left' : 'top-right'
}

function dismissNodeDetail(nodeId?: number) {
  cancelShowNodeDetail()
  cancelHideNodeDetail()

  if (nodeId !== undefined && hoveredNodeId.value !== null && hoveredNodeId.value !== nodeId) {
    return
  }

  hoveredNodeId.value = null
}

function showNodeDetail(nodeId: number, event?: FocusEvent | MouseEvent) {
  dismissNodeDetail(nodeId)
  hoveredNodeId.value = nodeId
  hoveredNodeAnchor.value = getHoverPoint(event)
  hoveredNodePlacement.value = getHoverPlacement(hoveredNodeAnchor.value)
}

function scheduleShowNodeDetail(nodeId: number, event?: MouseEvent) {
  if (activeResize.value !== null) {
    return
  }

  cancelShowNodeDetail()
  cancelHideNodeDetail()
  const anchor = getHoverPoint(event)
  const placement = getHoverPlacement(anchor)

  showNodeDetailTimer = window.setTimeout(() => {
    hoveredNodeId.value = nodeId
    hoveredNodeAnchor.value = anchor
    hoveredNodePlacement.value = placement
    showNodeDetailTimer = null
  }, DETAIL_SHOW_DELAY_MS)
}

function showSubtaskDetail(subtaskId: number, event?: FocusEvent | MouseEvent) {
  dismissSubtaskDetail(subtaskId)
  hoveredSubtaskId.value = subtaskId
  hoveredSubtaskAnchor.value = getHoverPoint(event)
  hoveredSubtaskPlacement.value = getHoverPlacement(hoveredSubtaskAnchor.value)
}

function scheduleShowSubtaskDetail(subtaskId: number, event?: MouseEvent) {
  if (activeResize.value !== null) {
    return
  }

  cancelShowSubtaskDetail()
  cancelHideSubtaskDetail()
  const anchor = getHoverPoint(event)
  const placement = getHoverPlacement(anchor)

  showSubtaskDetailTimer = window.setTimeout(() => {
    hoveredSubtaskId.value = subtaskId
    hoveredSubtaskAnchor.value = anchor
    hoveredSubtaskPlacement.value = placement
    showSubtaskDetailTimer = null
  }, DETAIL_SHOW_DELAY_MS)
}

function cancelShowNodeDetail() {
  if (showNodeDetailTimer !== null) {
    window.clearTimeout(showNodeDetailTimer)
    showNodeDetailTimer = null
  }
}

function cancelHideNodeDetail() {
  if (hideNodeDetailTimer !== null) {
    window.clearTimeout(hideNodeDetailTimer)
    hideNodeDetailTimer = null
  }
}

function scheduleHideNodeDetail() {
  cancelHideNodeDetail()

  hideNodeDetailTimer = window.setTimeout(() => {
    hoveredNodeId.value = null
    hideNodeDetailTimer = null
  }, DETAIL_HIDE_DELAY_MS)
}

function clearNodeDetail(nodeId?: number) {
  cancelShowNodeDetail()

  if (nodeId !== undefined && hoveredNodeId.value !== nodeId) {
    return
  }

  if (hoveredNodeId.value === null) {
    return
  }

  scheduleHideNodeDetail()
}

function cancelShowSubtaskDetail() {
  if (showSubtaskDetailTimer !== null) {
    window.clearTimeout(showSubtaskDetailTimer)
    showSubtaskDetailTimer = null
  }
}

function cancelHideSubtaskDetail() {
  if (hideSubtaskDetailTimer !== null) {
    window.clearTimeout(hideSubtaskDetailTimer)
    hideSubtaskDetailTimer = null
  }
}

function scheduleHideSubtaskDetail() {
  cancelHideSubtaskDetail()

  hideSubtaskDetailTimer = window.setTimeout(() => {
    hoveredSubtaskId.value = null
    hideSubtaskDetailTimer = null
  }, DETAIL_HIDE_DELAY_MS)
}

function dismissSubtaskDetail(subtaskId?: number) {
  cancelShowSubtaskDetail()
  cancelHideSubtaskDetail()

  if (subtaskId !== undefined && hoveredSubtaskId.value !== null && hoveredSubtaskId.value !== subtaskId) {
    return
  }

  hoveredSubtaskId.value = null
}

function clearSubtaskDetail(subtaskId?: number) {
  cancelShowSubtaskDetail()

  if (subtaskId !== undefined && hoveredSubtaskId.value !== subtaskId) {
    return
  }

  if (hoveredSubtaskId.value === null) {
    return
  }

  scheduleHideSubtaskDetail()
}

function getResizedRange(target: ResizeTarget, pointerClientX: number) {
  const dayWidth = getPixelsPerDay(props.scale)
  const pointerStartX = 'pointerStartX' in target
    ? (target as ActiveResizeState).pointerStartX
    : pointerClientX
  const deltaDays = Math.round((pointerClientX - pointerStartX) / dayWidth)

  if (target.edge === 'start') {
    const nextStartDate = clampDate(
      shiftDate(target.originalStartDate, deltaDays),
      target.minDate,
      target.maxDate,
    )

    return {
      plannedEndDate: target.originalEndDate,
      plannedStartDate: nextStartDate > target.originalEndDate ? target.originalEndDate : nextStartDate,
    }
  }

  const nextEndDate = clampDate(
    shiftDate(target.originalEndDate, deltaDays),
    target.minDate,
    target.maxDate,
  )

  return {
    plannedEndDate: nextEndDate < target.originalStartDate ? target.originalStartDate : nextEndDate,
    plannedStartDate: target.originalStartDate,
  }
}

function stopResize() {
  window.removeEventListener('mousemove', handleResizeMove)
  window.removeEventListener('mouseup', handleResizeEnd)
}

function cancelDetailHoverForResize(target: ResizeTarget) {
  if (target.kind === 'node') {
    dismissNodeDetail(target.nodeId)
    return
  }

  dismissSubtaskDetail(target.subtaskId)
}

function beginResize(target: ResizeTarget, event: MouseEvent) {
  if (event.button !== 0) {
    return
  }

  event.preventDefault()
  event.stopPropagation()
  cancelDetailHoverForResize(target)

  activeResize.value = {
    ...target,
    plannedEndDate: target.originalEndDate,
    plannedStartDate: target.originalStartDate,
    pointerStartX: event.clientX,
  }

  window.addEventListener('mousemove', handleResizeMove)
  window.addEventListener('mouseup', handleResizeEnd)
}

function handleResizeMove(event: MouseEvent) {
  if (activeResize.value === null) {
    return
  }

  const resizedRange = getResizedRange(activeResize.value, event.clientX)
  activeResize.value = {
    ...activeResize.value,
    ...resizedRange,
  }
}

function handleResizeEnd(event: MouseEvent) {
  const resizeDraft = activeResize.value

  if (resizeDraft === null) {
    stopResize()
    return
  }

  stopResize()
  activeResize.value = null

  const resizedRange = getResizedRange(resizeDraft, event.clientX)
  if (
    resizedRange.plannedStartDate === resizeDraft.originalStartDate
    && resizedRange.plannedEndDate === resizeDraft.originalEndDate
  ) {
    return
  }

  if (resizeDraft.kind === 'node') {
    emit('resize-node', {
      nodeId: resizeDraft.nodeId,
      plannedEndDate: resizedRange.plannedEndDate,
      plannedStartDate: resizedRange.plannedStartDate,
    })
    return
  }

  emit('resize-subtask', {
    nodeId: resizeDraft.nodeId,
    plannedEndDate: resizedRange.plannedEndDate,
    plannedStartDate: resizedRange.plannedStartDate,
    subtaskId: resizeDraft.subtaskId,
  })
}

function isNodeResizeDisabled(nodeId: number) {
  return !props.canEditSchedule || savingNodeIdSet.value.has(nodeId)
}

function isSubtaskResizeDisabled(subtaskId: number) {
  return !props.canEditSchedule || savingSubtaskIdSet.value.has(subtaskId)
}

function isActiveResizeTarget(kind: 'node' | 'subtask', id: number) {
  if (activeResize.value === null || activeResize.value.kind !== kind) {
    return false
  }

  return kind === 'node'
    ? activeResize.value.nodeId === id
    : 'subtaskId' in activeResize.value && activeResize.value.subtaskId === id
}

function getNodeBarTitle(node: GanttNodeSummary) {
  const range = getVisibleRange(node, 'node')
  return `${node.name}｜${getWorkStatusLabel(node.status)}｜${range.planned_start_date} - ${range.planned_end_date}`
}

function getSubtaskBarTitle(subtask: GanttSubtaskSummary) {
  const range = getVisibleRange(subtask, 'subtask')
  return `${subtask.name}｜${subtask.responsible_real_name}｜${range.planned_start_date} - ${range.planned_end_date}`
}

function markIgnoredScroll(
  ignoredScrollMap: WeakMap<HTMLElement, number>,
  target: HTMLElement,
  value: number,
) {
  ignoredScrollMap.set(target, value)
}

function shouldIgnoreProgrammaticScroll(
  ignoredScrollMap: WeakMap<HTMLElement, number>,
  source: HTMLElement | null,
  scrollKey: 'scrollLeft' | 'scrollTop',
) {
  if (source === null) {
    return false
  }

  const ignoredValue = ignoredScrollMap.get(source)

  if (ignoredValue === undefined) {
    return false
  }

  const currentValue = source[scrollKey]

  if (currentValue === ignoredValue) {
    ignoredScrollMap.delete(source)
    return true
  }

  ignoredScrollMap.delete(source)
  return false
}

function syncScrollPosition(
  ignoredScrollMap: WeakMap<HTMLElement, number>,
  source: HTMLElement | null,
  target: HTMLElement | null,
  scrollKey: 'scrollLeft' | 'scrollTop',
) {
  if (source === null || target === null) {
    return
  }

  const value = source[scrollKey]

  if (target[scrollKey] === value) {
    return
  }

  markIgnoredScroll(ignoredScrollMap, target, value)
  target[scrollKey] = value
}

function syncSidebarOffset(scrollTop: number) {
  if (sidebarRowsRailRef.value !== null) {
    sidebarRowsRailRef.value.style.transform = `translateY(-${scrollTop}px)`
  }
}

function handleSidebarWheel(event: WheelEvent) {
  if (event.defaultPrevented || event.ctrlKey || rowsScrollRef.value === null) {
    return
  }

  const delta = normalizeWheelDelta(event, rowsScrollRef.value.clientHeight).y

  if (Math.abs(delta) < 0.1) {
    return
  }

  const maxScrollTop = Math.max(rowsScrollRef.value.scrollHeight - rowsScrollRef.value.clientHeight, 0)
  const unclampedScrollTop = rowsScrollRef.value.scrollTop + delta
  const nextScrollTop = maxScrollTop > 0
    ? Math.min(Math.max(unclampedScrollTop, 0), maxScrollTop)
    : unclampedScrollTop

  if (nextScrollTop === rowsScrollRef.value.scrollTop) {
    return
  }

  event.preventDefault()
  rowsScrollRef.value.scrollTop = nextScrollTop
  syncSidebarOffset(nextScrollTop)
}

function handleTopHorizontalScroll() {
  if (!shouldIgnoreProgrammaticScroll(ignoredHorizontalScroll, topScrollRef.value, 'scrollLeft')) {
    syncScrollPosition(ignoredHorizontalScroll, topScrollRef.value, rowsScrollRef.value, 'scrollLeft')
    syncScrollPosition(ignoredHorizontalScroll, topScrollRef.value, axisScrollRef.value, 'scrollLeft')
  }
}

function handleRowsScroll() {
  if (rowsScrollRef.value === null) {
    return
  }

  syncSidebarOffset(rowsScrollRef.value.scrollTop)

  if (shouldIgnoreProgrammaticScroll(ignoredHorizontalScroll, rowsScrollRef.value, 'scrollLeft')) {
    return
  }

  syncScrollPosition(ignoredHorizontalScroll, rowsScrollRef.value, topScrollRef.value, 'scrollLeft')
  syncScrollPosition(ignoredHorizontalScroll, rowsScrollRef.value, axisScrollRef.value, 'scrollLeft')
}

onBeforeUnmount(() => {
  cancelShowNodeDetail()
  cancelHideNodeDetail()
  cancelShowSubtaskDetail()
  cancelHideSubtaskDetail()
  stopResize()
})
</script>

<template>
  <section class="project-gantt" data-testid="project-gantt-view">
    <header class="project-gantt__toolbar">
      <div class="project-gantt__toolbar-copy">
        <p class="project-gantt__eyebrow">Timeline Matrix</p>
        <div>
          <h3>阶段甘特图</h3>
          <p>保持当前阶段时间视角，点击阶段条即可展开该阶段下的全部子任务排期。</p>
        </div>
      </div>

      <div class="project-gantt__toolbar-actions">
        <div class="project-gantt__perspective-switch" role="tablist" aria-label="甘特图视角">
          <button
            :class="['project-gantt__perspective-button', { 'is-active': perspective === 'stage' }]"
            type="button"
            @click="emit('update:perspective', 'stage')"
          >
            阶段 / 时间
          </button>
          <button
            :class="['project-gantt__perspective-button', { 'is-active': perspective === 'member' }]"
            type="button"
            @click="emit('update:perspective', 'member')"
          >
            人员 / 时间
          </button>
        </div>

        <div class="project-gantt__toolbar-scale-row">
          <GanttScaleSwitcher :scale="scale" @update:scale="emit('update:scale', $event)" />

          <div class="project-gantt__tree-actions">
            <button
              data-testid="project-gantt-expand-all"
              type="button"
              :disabled="isExpandingAll"
              @click="emit('expand-all')"
            >
              {{ isExpandingAll ? '展开中...' : '全部展开' }}
            </button>
            <button
              data-testid="project-gantt-collapse-all"
              type="button"
              @click="emit('collapse-all')"
            >
              全部收起
            </button>
          </div>
        </div>
      </div>
    </header>

    <p v-if="isLoading" class="project-gantt__state loading-panel">阶段甘特图加载中...</p>

    <section v-else-if="error" class="project-gantt__state project-gantt__state--error">
      <div>
        <strong>阶段甘特图暂时不可用</strong>
        <p>{{ error }}</p>
      </div>

      <button type="button" @click="emit('retry')">重试</button>
    </section>

    <section v-else-if="gantt === null || gantt.nodes.length === 0" class="project-gantt__state">
      当前还没有可展示的阶段排期。
    </section>

    <div v-else class="project-gantt__body-scroll smooth-scroll-surface">
      <div class="project-gantt__body">
        <aside class="project-gantt__sidebar">
          <div aria-hidden="true" class="project-gantt__sidebar-top-spacer" />
          <div class="project-gantt__sidebar-head">
            <span>阶段</span>
            <span>状态</span>
          </div>

          <div
            data-testid="project-gantt-sidebar-scroll"
            class="project-gantt__sidebar-scroll smooth-scroll-surface"
            @wheel="handleSidebarWheel"
          >
            <div ref="sidebarRowsRailRef" class="project-gantt__sidebar-rows-rail">
              <div
                v-for="row in visibleRows"
                :key="row.key"
                :data-testid="row.type === 'node' ? `project-gantt-row-label-${row.node.id}` : undefined"
                :class="[
                  'project-gantt__sidebar-row',
                  `project-gantt__sidebar-row--${row.type}`,
                  {
                    'is-expanded': row.type === 'node' && expandedNodeIdSet.has(row.node.id),
                  },
                ]"
              >
                <template v-if="row.type === 'node'">
                  <div class="project-gantt__sidebar-title">
                    <span
                      :class="[
                        'project-gantt__sidebar-caret',
                        {
                          'is-expanded': expandedNodeIdSet.has(row.node.id),
                        },
                      ]"
                      aria-hidden="true"
                    />
                    <strong>{{ row.node.name }}</strong>
                  </div>
                  <span
                    :class="[
                      'project-gantt__status-pill',
                      `project-gantt__status-pill--${getWorkStatusTone(row.node.status)}`,
                    ]"
                  >
                    {{ getWorkStatusLabel(row.node.status) }}
                  </span>
                </template>

                <template v-else-if="row.type === 'subtask'">
                  <div class="project-gantt__sidebar-branch" aria-hidden="true">
                    <span class="project-gantt__sidebar-branch-line" />
                    <span class="project-gantt__sidebar-branch-dot" />
                  </div>
                  <span class="project-gantt__sidebar-placeholder" aria-hidden="true" />
                </template>

                <template v-else-if="row.type === 'loading'">
                  <span class="project-gantt__sidebar-inline-state">子任务加载中...</span>
                  <span class="project-gantt__sidebar-placeholder" aria-hidden="true" />
                </template>

                <template v-else-if="row.type === 'error'">
                  <span class="project-gantt__sidebar-inline-state project-gantt__sidebar-inline-state--error">
                    加载失败
                  </span>
                  <span class="project-gantt__sidebar-placeholder" aria-hidden="true" />
                </template>

                <template v-else>
                  <span class="project-gantt__sidebar-inline-state">暂无子任务</span>
                  <span class="project-gantt__sidebar-placeholder" aria-hidden="true" />
                </template>
              </div>
            </div>
          </div>
        </aside>

        <div class="project-gantt__timeline-shell">
          <div
            ref="topScrollRef"
            data-testid="project-gantt-axis-scroll"
            class="project-gantt__top-scroll smooth-scroll-surface"
            @scroll="handleTopHorizontalScroll"
          >
            <div
              aria-hidden="true"
              data-testid="project-gantt-top-scroll-content"
              class="project-gantt__top-scroll-content"
              :style="timelineCanvasStyle"
            />
          </div>

          <div ref="axisScrollRef" class="project-gantt__axis-scroll">
            <div class="project-gantt__axis" data-testid="project-gantt-axis" :style="timelineCanvasStyle">
              <div
                v-for="item in axisItems"
                :key="item.key"
                :class="['project-gantt__axis-cell', `project-gantt__axis-cell--${scale}`]"
                :style="{ width: `${item.widthPx}px` }"
                :title="`${item.startDate} 至 ${item.endDate}`"
              >
                {{ item.label }}
              </div>
            </div>
          </div>

          <div
            ref="rowsScrollRef"
            data-testid="project-gantt-rows-scroll"
            class="project-gantt__timeline-scroll smooth-scroll-surface"
            @scroll="handleRowsScroll"
          >
            <div class="project-gantt__timeline-canvas" :style="timelineCanvasStyle">
              <div class="project-gantt__rows">
                <div
                  v-for="row in visibleRows"
                  :key="row.key"
                  :class="[
                    'project-gantt__row',
                    `project-gantt__row--${row.type}`,
                  ]"
                >
                  <div class="project-gantt__track">
                    <div
                      :data-testid="
                        row.type === 'node'
                          ? `project-gantt-track-grid-${row.node.id}`
                          : row.type === 'subtask'
                            ? `project-gantt-subtask-track-grid-${row.subtask.id}`
                            : undefined
                      "
                      aria-hidden="true"
                      class="project-gantt__track-grid"
                    >
                      <div
                        v-for="item in axisItems"
                        :key="`${row.key}-${item.key}`"
                        :class="['project-gantt__track-cell', `project-gantt__track-cell--${scale}`]"
                        :style="{ width: `${item.widthPx}px` }"
                      />
                    </div>

                    <button
                      v-if="row.type === 'node'"
                      :data-testid="`project-gantt-stage-bar-${row.node.id}`"
                      :class="[
                        'project-gantt__bar',
                        'project-gantt__bar--stage',
                        `project-gantt__bar--${getWorkStatusTone(row.node.status)}`,
                        {
                          'is-expanded': expandedNodeIdSet.has(row.node.id),
                          'is-resizing': isActiveResizeTarget('node', row.node.id),
                          'is-schedule-editable': !isNodeResizeDisabled(row.node.id),
                        },
                      ]"
                      :style="getBarStyle(row.node, 'node')"
                      :title="getNodeBarTitle(row.node)"
                      type="button"
                      @blur="clearNodeDetail(row.node.id)"
                      @click="emit('toggle-node', row.node.id)"
                      @focus="showNodeDetail(row.node.id, $event)"
                      @mouseleave="clearNodeDetail(row.node.id)"
                      @mouseenter="scheduleShowNodeDetail(row.node.id, $event)"
                    >
                      <span class="project-gantt__bar-label">{{ row.node.name }}</span>
                      <span
                        v-if="!isNodeResizeDisabled(row.node.id)"
                        :data-testid="`project-gantt-stage-resize-start-${row.node.id}`"
                        class="project-gantt__resize-handle project-gantt__resize-handle--start"
                        aria-hidden="true"
                        @click.stop.prevent
                        @mousedown.stop.prevent="beginResize({
                          edge: 'start',
                          kind: 'node',
                          maxDate: gantt.project.planned_end_date,
                          minDate: gantt.project.planned_start_date,
                          nodeId: row.node.id,
                          originalEndDate: row.node.planned_end_date,
                          originalStartDate: row.node.planned_start_date,
                        }, $event)"
                        @mouseenter.stop="dismissNodeDetail(row.node.id)"
                      />
                      <span
                        v-if="!isNodeResizeDisabled(row.node.id)"
                        :data-testid="`project-gantt-stage-resize-end-${row.node.id}`"
                        class="project-gantt__resize-handle project-gantt__resize-handle--end"
                        aria-hidden="true"
                        @click.stop.prevent
                        @mousedown.stop.prevent="beginResize({
                          edge: 'end',
                          kind: 'node',
                          maxDate: gantt.project.planned_end_date,
                          minDate: gantt.project.planned_start_date,
                          nodeId: row.node.id,
                          originalEndDate: row.node.planned_end_date,
                          originalStartDate: row.node.planned_start_date,
                        }, $event)"
                        @mouseenter.stop="dismissNodeDetail(row.node.id)"
                      />
                    </button>

                    <button
                      v-else-if="row.type === 'subtask'"
                      :data-testid="`project-gantt-subtask-bar-${row.subtask.id}`"
                      :class="[
                        'project-gantt__bar',
                        'project-gantt__bar--subtask',
                        `project-gantt__bar--${getWorkStatusTone(row.subtask.status)}`,
                        {
                          'is-resizing': isActiveResizeTarget('subtask', row.subtask.id),
                          'is-schedule-editable': !isSubtaskResizeDisabled(row.subtask.id),
                        },
                      ]"
                      :style="getBarStyle(row.subtask, 'subtask')"
                      :title="getSubtaskBarTitle(row.subtask)"
                      type="button"
                      @blur="clearSubtaskDetail(row.subtask.id)"
                      @focus="showSubtaskDetail(row.subtask.id, $event)"
                      @mouseleave="clearSubtaskDetail(row.subtask.id)"
                      @mouseenter="scheduleShowSubtaskDetail(row.subtask.id, $event)"
                    >
                      <span class="project-gantt__bar-label">{{ row.subtask.name }}</span>
                      <span
                        v-if="!isSubtaskResizeDisabled(row.subtask.id)"
                        :data-testid="`project-gantt-subtask-resize-start-${row.subtask.id}`"
                        class="project-gantt__resize-handle project-gantt__resize-handle--start"
                        aria-hidden="true"
                        @click.stop.prevent
                        @mousedown.stop.prevent="beginResize({
                          edge: 'start',
                          kind: 'subtask',
                          maxDate: row.node.planned_end_date,
                          minDate: row.node.planned_start_date,
                          nodeId: row.node.id,
                          originalEndDate: row.subtask.planned_end_date,
                          originalStartDate: row.subtask.planned_start_date,
                          subtaskId: row.subtask.id,
                        }, $event)"
                        @mouseenter.stop="dismissSubtaskDetail(row.subtask.id)"
                      />
                      <span
                        v-if="!isSubtaskResizeDisabled(row.subtask.id)"
                        :data-testid="`project-gantt-subtask-resize-end-${row.subtask.id}`"
                        class="project-gantt__resize-handle project-gantt__resize-handle--end"
                        aria-hidden="true"
                        @click.stop.prevent
                        @mousedown.stop.prevent="beginResize({
                          edge: 'end',
                          kind: 'subtask',
                          maxDate: row.node.planned_end_date,
                          minDate: row.node.planned_start_date,
                          nodeId: row.node.id,
                          originalEndDate: row.subtask.planned_end_date,
                          originalStartDate: row.subtask.planned_start_date,
                          subtaskId: row.subtask.id,
                        }, $event)"
                        @mouseenter.stop="dismissSubtaskDetail(row.subtask.id)"
                      />
                    </button>

                    <div
                      v-else-if="row.type === 'loading'"
                      class="project-gantt__inline-state-chip project-gantt__inline-state-chip--loading"
                    >
                      子任务加载中...
                    </div>

                    <div
                      v-else-if="row.type === 'empty'"
                      class="project-gantt__inline-state-chip"
                    >
                      该阶段下暂无子任务排期
                    </div>

                    <button
                      v-else
                      class="project-gantt__inline-state-chip project-gantt__inline-state-chip--error"
                      type="button"
                      @click="emit('retry-node', row.node.id)"
                    >
                      {{ row.error }}，点击重试
                    </button>
                  </div>
                </div>
              </div>
            </div>
          </div>

          <Teleport to="body">
            <aside
              v-if="hoveredNode"
              :data-placement="hoveredNodePlacement"
              :style="detailCardStyle"
              class="project-gantt__detail-card"
              data-testid="project-gantt-detail-card"
              @mouseenter="cancelHideNodeDetail"
              @mouseleave="clearNodeDetail()"
            >
              <div class="project-gantt__detail-topline">
                <span class="project-gantt__detail-eyebrow">阶段详情</span>
                <span
                  :class="[
                    'project-gantt__detail-status',
                    `project-gantt__detail-status--${getWorkStatusTone(hoveredNode.status)}`,
                  ]"
                >
                  {{ getWorkStatusLabel(hoveredNode.status) }}
                </span>
              </div>

              <strong class="project-gantt__detail-title">{{ hoveredNode.name }}</strong>

              <div
                :class="[
                  'project-gantt__detail-stat',
                  `project-gantt__detail-stat--${getWorkStatusTone(hoveredNode.status)}`,
                ]"
              >
                <span>计划窗口</span>
                <strong>{{ getScheduleDays(hoveredNode.planned_start_date, hoveredNode.planned_end_date) }} 天</strong>
                <small>Phase {{ hoveredNode.sequence_no }}</small>
              </div>

              <dl class="project-gantt__detail-grid">
                <div class="project-gantt__detail-grid-item">
                  <dt>阶段顺序</dt>
                  <dd>#{{ hoveredNode.sequence_no }}</dd>
                </div>
                <div class="project-gantt__detail-grid-item">
                  <dt>计划周期</dt>
                  <dd>{{ hoveredNode.planned_start_date }}<br>{{ hoveredNode.planned_end_date }}</dd>
                </div>
                <div
                  :class="[
                    'project-gantt__detail-grid-item',
                    'project-gantt__detail-grid-item--completion',
                    {
                      'is-empty': !hoveredNode.completed_at,
                    },
                  ]"
                >
                  <dt>完成时间</dt>
                  <dd>{{ hoveredNode.completed_at ? formatCompletedAt(hoveredNode.completed_at) : '尚未完成' }}</dd>
                </div>
              </dl>
            </aside>
          </Teleport>

          <Teleport to="body">
            <aside
              v-if="hoveredSubtask"
              :data-placement="hoveredSubtaskPlacement"
              :style="subtaskDetailCardStyle"
              class="project-gantt__subtask-detail-card"
              data-testid="project-gantt-subtask-detail-card"
              @mouseenter="cancelHideSubtaskDetail"
              @mouseleave="clearSubtaskDetail()"
            >
              <div class="project-gantt__subtask-detail-topline">
                <span class="project-gantt__subtask-detail-eyebrow">子任务详情</span>
                <span
                  :class="[
                    'project-gantt__subtask-detail-status',
                    `project-gantt__subtask-detail-status--${getWorkStatusTone(hoveredSubtask.status)}`,
                  ]"
                >
                  {{ getWorkStatusLabel(hoveredSubtask.status) }}
                </span>
              </div>

              <strong class="project-gantt__subtask-detail-title">{{ hoveredSubtask.name }}</strong>

              <div class="project-gantt__subtask-detail-progress">
                <div class="project-gantt__subtask-detail-progress-head">
                  <span>完成进度</span>
                  <strong>{{ hoveredSubtask.progress_percent }}%</strong>
                </div>
                <div class="project-gantt__subtask-detail-progress-track">
                  <span
                    class="project-gantt__subtask-detail-progress-fill"
                    :style="{ width: `${hoveredSubtask.progress_percent}%` }"
                  />
                </div>
              </div>

              <dl class="project-gantt__subtask-detail-grid">
                <div class="project-gantt__subtask-detail-grid-item">
                  <dt>所属阶段</dt>
                  <dd>{{ hoveredSubtask.node_name }}</dd>
                </div>
                <div class="project-gantt__subtask-detail-grid-item">
                  <dt>负责人</dt>
                  <dd>{{ hoveredSubtask.responsible_real_name }}</dd>
                </div>
                <div class="project-gantt__subtask-detail-grid-item">
                  <dt>计划周期</dt>
                  <dd>{{ hoveredSubtask.planned_start_date }}<br>{{ hoveredSubtask.planned_end_date }}</dd>
                </div>
              </dl>
            </aside>
          </Teleport>
        </div>
      </div>
    </div>
  </section>
</template>

<style scoped>
.project-gantt {
  --project-gantt-top-scroll-height: 18px;
  --project-gantt-axis-height: 56px;
  --project-gantt-row-height: 62px;
  position: relative;
  overflow: hidden;
  display: grid;
  gap: 18px;
  padding: 24px;
  border: 1px solid var(--meta-surface-border);
  border-radius: 28px;
  background: var(--meta-surface-bg), var(--meta-surface-glow), var(--card-sheen);
  box-shadow:
    var(--meta-surface-shadow),
    0 24px 48px rgba(2, 6, 18, 0.24);
}

.project-gantt::before {
  content: '';
  position: absolute;
  inset: 0;
  background:
    linear-gradient(var(--grid-line) 1px, transparent 1px),
    linear-gradient(90deg, var(--grid-line) 1px, transparent 1px);
  background-size: 24px 24px;
  opacity: 0.18;
  pointer-events: auto;
}

.project-gantt__toolbar,
.project-gantt__body-scroll,
.project-gantt__state {
  position: relative;
  z-index: 1;
}

.project-gantt__toolbar {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 18px;
}

.project-gantt__toolbar-copy {
  display: grid;
  gap: 10px;
}

.project-gantt__eyebrow,
.project-gantt__toolbar-copy h3,
.project-gantt__toolbar-copy p {
  margin: 0;
}

.project-gantt__eyebrow {
  font-size: 0.76rem;
  letter-spacing: 0.18em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.project-gantt__toolbar-copy h3 {
  font-size: 1.46rem;
  color: color-mix(in srgb, var(--text-main) 86%, #ffffff 14%);
}

.project-gantt__toolbar-copy p {
  color: var(--text-soft);
  line-height: 1.6;
}

.project-gantt__toolbar-actions {
  display: grid;
  justify-items: end;
  gap: 12px;
}

.project-gantt__toolbar-scale-row {
  display: inline-flex;
  align-items: center;
  justify-content: flex-end;
  flex-wrap: wrap;
  gap: 12px;
}

.project-gantt__tree-actions {
  display: inline-flex;
  align-items: center;
  flex-wrap: wrap;
  gap: 10px;
}

.project-gantt__tree-actions button {
  padding: 9px 14px;
  border: 1px solid var(--dialog-control-border);
  border-radius: 999px;
  background: var(--dialog-control-bg), var(--card-sheen);
  box-shadow: var(--dialog-control-shadow);
  color: var(--text-main);
  font-size: 0.78rem;
  font-weight: 700;
  cursor: pointer;
  transition:
    transform 180ms ease-out,
    border-color 180ms ease-out,
    box-shadow 180ms ease-out,
    color 180ms ease-out;
}

.project-gantt__tree-actions button:hover:not(:disabled) {
  transform: translateY(-1px);
  border-color: color-mix(in srgb, var(--accent-line) 42%, var(--dialog-control-border));
  box-shadow:
    var(--dialog-control-shadow),
    0 10px 18px color-mix(in srgb, var(--accent-end) 12%, transparent);
}

.project-gantt__tree-actions button:disabled {
  opacity: 0.72;
  cursor: wait;
}

.project-gantt__perspective-switch {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  padding: 6px;
  border: 1px solid var(--dialog-control-border);
  border-radius: 999px;
  background: var(--dialog-control-bg), var(--card-sheen);
  box-shadow: var(--dialog-control-shadow);
}

.project-gantt__perspective-button {
  padding: 9px 14px;
  border: 0;
  border-radius: 999px;
  background: transparent;
  color: var(--text-soft);
  font-size: 0.8rem;
  font-weight: 700;
  cursor: pointer;
  transition:
    transform 180ms ease-out,
    background 180ms ease-out,
    color 180ms ease-out,
    box-shadow 180ms ease-out;
}

.project-gantt__perspective-button.is-active {
  background: linear-gradient(135deg, color-mix(in srgb, var(--accent-start) 80%, #0d2754 20%), color-mix(in srgb, var(--accent-end) 68%, #0f2c5b 32%));
  color: #f7fbff;
  box-shadow:
    inset 0 1px 0 color-mix(in srgb, #ffffff 18%, transparent),
    0 10px 18px color-mix(in srgb, var(--accent-end) 16%, transparent);
}

.project-gantt__state {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 18px;
  min-height: 240px;
  padding: 24px;
  border: 1px solid var(--meta-surface-border);
  border-radius: 22px;
  background: var(--meta-surface-bg), var(--meta-surface-glow), var(--card-sheen);
  box-shadow: var(--meta-surface-shadow);
  color: var(--text-soft);
}

.project-gantt__state--error strong,
.project-gantt__state--error p {
  margin: 0;
}

.project-gantt__state--error {
  align-items: flex-start;
}

.project-gantt__state button {
  padding: 10px 16px;
  border: 0;
  border-radius: 12px;
  background: linear-gradient(135deg, var(--accent-start), var(--accent-end));
  color: #f8fbff;
  font-weight: 600;
  cursor: pointer;
}

.project-gantt__body-scroll {
  max-height: 80vh;
  overflow: hidden;
  padding-right: 4px;
}

.project-gantt__body {
  display: grid;
  grid-template-columns: minmax(240px, 280px) minmax(0, 1fr);
  gap: 0;
  align-items: start;
  min-width: 0;
  border: 1px solid var(--meta-surface-border);
  border-radius: 22px;
  background: var(--meta-surface-bg), var(--meta-surface-glow), var(--card-sheen);
  box-shadow: var(--meta-surface-shadow);
}

.project-gantt__sidebar,
.project-gantt__timeline-shell {
  min-width: 0;
  min-height: 0;
  border: 0;
  border-radius: 0;
  background: transparent;
}

.project-gantt__sidebar {
  display: grid;
  grid-template-rows: auto auto minmax(0, 1fr);
  align-content: start;
  border-right: 1px solid var(--meta-surface-border);
  overflow: hidden;
}

.project-gantt__sidebar-top-spacer,
.project-gantt__sidebar-head,
.project-gantt__sidebar-row {
  box-sizing: border-box;
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  gap: 12px;
  align-items: center;
  min-width: 0;
  padding: 0 16px;
}

.project-gantt__sidebar-top-spacer {
  position: sticky;
  top: 0;
  z-index: 4;
  height: var(--project-gantt-top-scroll-height);
  padding: 0;
  border-bottom: 1px solid var(--meta-surface-border);
  background: color-mix(in srgb, var(--panel-bg) 94%, transparent);
}

.project-gantt__sidebar-row {
  height: var(--project-gantt-row-height);
  border-bottom: 1px solid var(--meta-surface-border);
  overflow: hidden;
  contain: layout paint;
  content-visibility: auto;
  contain-intrinsic-size: 62px;
  background: color-mix(in srgb, var(--panel-bg) 94%, transparent);
}

.project-gantt__sidebar-head {
  position: sticky;
  top: var(--project-gantt-top-scroll-height);
  z-index: 3;
  height: var(--project-gantt-axis-height);
  min-height: var(--project-gantt-axis-height);
  border-bottom: 1px solid var(--meta-surface-border);
  font-size: 0.78rem;
  letter-spacing: 0.12em;
  text-transform: uppercase;
  color: var(--text-soft);
  background: color-mix(in srgb, var(--panel-bg) 94%, transparent);
}

.project-gantt__sidebar-scroll {
  min-height: 0;
  max-height: calc(80vh - var(--project-gantt-top-scroll-height) - var(--project-gantt-axis-height));
  overflow: hidden;
  background: color-mix(in srgb, var(--panel-bg) 94%, transparent);
}

.project-gantt__sidebar-rows-rail {
  transform: translateY(0);
}

.project-gantt__sidebar-rows {
  display: grid;
}

.project-gantt__sidebar-row:last-child {
  border-bottom: 0;
}

.project-gantt__sidebar-row strong {
  min-width: 0;
  overflow: hidden;
  font-size: 1rem;
  text-overflow: ellipsis;
  white-space: nowrap;
  color: var(--text-main);
}

.project-gantt__sidebar-title {
  min-width: 0;
  display: inline-flex;
  align-items: center;
  gap: 10px;
}

.project-gantt__sidebar-caret {
  width: 10px;
  height: 10px;
  border-right: 2px solid color-mix(in srgb, var(--text-soft) 80%, transparent);
  border-bottom: 2px solid color-mix(in srgb, var(--text-soft) 80%, transparent);
  transform: rotate(-45deg);
  transition: transform 180ms ease-out, border-color 180ms ease-out;
  flex: none;
}

.project-gantt__sidebar-caret.is-expanded {
  border-color: color-mix(in srgb, var(--accent-end) 74%, var(--text-soft));
  transform: rotate(45deg) translateY(-1px);
}

.project-gantt__sidebar-row--subtask,
.project-gantt__sidebar-row--loading,
.project-gantt__sidebar-row--error,
.project-gantt__sidebar-row--empty {
  grid-template-columns: minmax(0, 1fr);
  padding-left: 42px;
}

.project-gantt__sidebar-branch {
  display: inline-flex;
  align-items: center;
  gap: 10px;
}

.project-gantt__sidebar-branch-line {
  width: 18px;
  height: 1px;
  background: color-mix(in srgb, var(--accent-line) 22%, transparent);
}

.project-gantt__sidebar-branch-dot {
  width: 6px;
  height: 6px;
  border-radius: 999px;
  background: color-mix(in srgb, var(--accent-end) 46%, var(--text-soft));
  box-shadow: 0 0 0 4px color-mix(in srgb, var(--accent-end) 10%, transparent);
}

.project-gantt__sidebar-inline-state {
  color: var(--text-soft);
  font-size: 0.82rem;
  font-weight: 600;
}

.project-gantt__sidebar-inline-state--error {
  color: var(--work-status-delayed-color);
}

.project-gantt__sidebar-placeholder {
  display: none;
}

.project-gantt__status-pill {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 74px;
  padding: 8px 12px;
  border: 1px solid transparent;
  border-radius: 999px;
  font-size: 0.78rem;
  font-weight: 600;
  flex-shrink: 0;
  white-space: nowrap;
}

.project-gantt__status-pill--pending {
  border-color: var(--work-status-pending-border);
  background: var(--work-status-pending-bg);
  color: var(--work-status-pending-color);
}

.project-gantt__status-pill--active {
  border-color: var(--work-status-active-border);
  background: var(--work-status-active-bg);
  color: var(--work-status-active-color);
}

.project-gantt__status-pill--done {
  border-color: var(--work-status-done-border);
  background: var(--work-status-done-bg);
  color: var(--work-status-done-color);
}

.project-gantt__status-pill--delayed {
  border-color: var(--work-status-delayed-border);
  background: var(--work-status-delayed-bg);
  color: var(--work-status-delayed-color);
}

.project-gantt__timeline-shell {
  position: relative;
  min-width: 0;
  min-height: 0;
  display: grid;
  grid-template-rows: auto auto minmax(0, 1fr);
  overflow: hidden;
}

.project-gantt__top-scroll {
  box-sizing: border-box;
  position: sticky;
  top: 0;
  z-index: 4;
  height: var(--project-gantt-top-scroll-height);
  overflow-x: auto;
  overflow-y: hidden;
  border-bottom: 1px solid var(--meta-surface-border);
  scrollbar-gutter: stable;
  scrollbar-width: thin;
  scrollbar-color: color-mix(in srgb, var(--accent-end) 58%, var(--panel-bg)) color-mix(in srgb, var(--panel-bg) 84%, transparent);
  background: color-mix(in srgb, var(--panel-bg) 94%, transparent);
}

.project-gantt__top-scroll-content {
  height: 1px;
}

.project-gantt__top-scroll::-webkit-scrollbar {
  height: var(--project-gantt-top-scroll-height);
}

.project-gantt__top-scroll::-webkit-scrollbar-track {
  border-radius: 999px;
  background: color-mix(in srgb, var(--panel-bg) 84%, transparent);
}

.project-gantt__top-scroll::-webkit-scrollbar-thumb {
  border: 2px solid transparent;
  border-radius: 999px;
  background: linear-gradient(90deg, color-mix(in srgb, var(--accent-start) 72%, #0d2754 28%), color-mix(in srgb, var(--accent-end) 72%, #0f2c5b 28%));
  background-clip: padding-box;
}

.project-gantt__axis-scroll {
  box-sizing: border-box;
  position: sticky;
  top: var(--project-gantt-top-scroll-height);
  z-index: 3;
  height: var(--project-gantt-axis-height);
  overflow: hidden;
  background: color-mix(in srgb, var(--panel-bg) 94%, transparent);
}

.project-gantt__timeline-scroll {
  min-height: 0;
  max-height: calc(80vh - var(--project-gantt-top-scroll-height) - var(--project-gantt-axis-height));
  overflow-x: auto;
  overflow-y: auto;
  padding-bottom: 0;
}

.project-gantt__timeline-canvas {
  min-width: 100%;
  min-height: 100%;
  box-sizing: content-box;
}

.project-gantt__axis {
  box-sizing: border-box;
  display: flex;
  min-height: 100%;
  height: 100%;
  border-bottom: 1px solid var(--meta-surface-border);
  background: var(--dialog-control-bg), var(--card-sheen);
}

.project-gantt__axis-cell {
  flex: 0 0 auto;
  display: flex;
  align-items: center;
  justify-content: flex-start;
  padding: 0 12px;
  border-right: 1px solid color-mix(in srgb, var(--accent-line) 16%, transparent);
  font-size: 0.8rem;
  color: var(--text-soft);
  white-space: nowrap;
  overflow: hidden;
  background: linear-gradient(
    180deg,
    color-mix(in srgb, var(--accent-end) 10%, var(--dialog-control-bg)),
    var(--dialog-control-bg)
  );
  box-shadow: inset 0 1px 0 color-mix(in srgb, #ffffff 14%, transparent);
}

.project-gantt__axis-cell:nth-child(even) {
  background: linear-gradient(
    180deg,
    color-mix(in srgb, var(--accent-start) 8%, var(--dialog-control-bg)),
    var(--dialog-control-bg)
  );
}

.project-gantt__axis-cell--day {
  justify-content: center;
  padding: 0 6px;
  font-size: 0.72rem;
  font-weight: 600;
  letter-spacing: 0.01em;
  font-variant-numeric: tabular-nums;
}

.project-gantt__axis-cell--week,
.project-gantt__axis-cell--month {
  font-variant-numeric: tabular-nums;
}

.project-gantt__axis-cell:first-child {
  border-left: 1px solid color-mix(in srgb, var(--accent-line) 20%, transparent);
}

.project-gantt__rows {
  display: grid;
}

.project-gantt__row {
  box-sizing: border-box;
  height: var(--project-gantt-row-height);
  border-bottom: 1px solid var(--meta-surface-border);
  contain: layout paint;
  content-visibility: auto;
  contain-intrinsic-size: 62px;
}

.project-gantt__row:last-child {
  border-bottom: 0;
}

.project-gantt__track {
  position: relative;
  height: 100%;
  overflow: hidden;
  border: 0;
  border-radius: 0;
  background: linear-gradient(
    180deg,
    color-mix(in srgb, var(--accent-end) 6%, var(--dialog-control-bg)),
    color-mix(in srgb, var(--accent-start) 2%, var(--panel-bg))
  );
}

.project-gantt__track-grid {
  position: absolute;
  inset: 0;
  display: flex;
  pointer-events: none;
}

.project-gantt__track-cell {
  flex: 0 0 auto;
  height: 100%;
  border-right: 1px solid color-mix(in srgb, var(--accent-line) 14%, transparent);
  background: linear-gradient(
    180deg,
    color-mix(in srgb, var(--accent-end) 8%, var(--dialog-control-bg)),
    var(--dialog-control-bg)
  );
}

.project-gantt__track-cell:nth-child(even) {
  background: linear-gradient(
    180deg,
    color-mix(in srgb, var(--accent-start) 6%, var(--dialog-control-bg)),
    var(--dialog-control-bg)
  );
}

.project-gantt__track-cell--day {
  background: linear-gradient(
    180deg,
    color-mix(in srgb, var(--accent-end) 9%, var(--dialog-control-bg)),
    var(--dialog-control-bg)
  );
}

.project-gantt__bar {
  position: absolute;
  top: 50%;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  height: 38px;
  padding: 0 14px;
  border: 0;
  border-radius: 14px;
  cursor: pointer;
  transform: translateY(-50%);
  z-index: 1;
  transition:
    transform 180ms ease-out,
    box-shadow 180ms ease-out,
    filter 180ms ease-out;
}

.project-gantt__bar.is-schedule-editable {
  padding-inline: 20px;
}

.project-gantt__bar.is-resizing {
  cursor: ew-resize;
  transition: none;
}

.project-gantt__bar-label {
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  font-size: 0.84rem;
  font-weight: 600;
}

.project-gantt__resize-handle {
  position: absolute;
  top: 4px;
  bottom: 4px;
  width: 12px;
  border-radius: 999px;
  opacity: 0;
  transition:
    opacity 160ms ease-out,
    background 160ms ease-out,
    box-shadow 160ms ease-out;
  cursor: ew-resize;
}

.project-gantt__resize-handle--start {
  left: 4px;
}

.project-gantt__resize-handle--end {
  right: 4px;
}

.project-gantt__bar.is-schedule-editable:hover .project-gantt__resize-handle,
.project-gantt__bar.is-schedule-editable:focus-visible .project-gantt__resize-handle,
.project-gantt__bar.is-resizing .project-gantt__resize-handle {
  opacity: 1;
  background: color-mix(in srgb, #ffffff 18%, transparent);
  box-shadow:
    inset 0 0 0 1px color-mix(in srgb, #ffffff 28%, transparent),
    0 0 0 1px color-mix(in srgb, #071221 16%, transparent);
}

.project-gantt__bar--stage.is-expanded {
  box-shadow:
    inset 0 0 0 1px color-mix(in srgb, #ffffff 16%, transparent),
    0 16px 28px color-mix(in srgb, var(--accent-end) 14%, transparent);
}

.project-gantt__bar--subtask {
  height: 32px;
  padding: 0 12px;
  border-radius: 12px;
  box-shadow: inset 0 0 0 1px color-mix(in srgb, #ffffff 12%, transparent);
}

.project-gantt__row--subtask .project-gantt__track {
  background: linear-gradient(
    180deg,
    color-mix(in srgb, var(--accent-end) 4%, var(--dialog-control-bg)),
    color-mix(in srgb, var(--panel-bg) 96%, var(--dialog-control-bg))
  );
}

.project-gantt__row--loading .project-gantt__track,
.project-gantt__row--error .project-gantt__track,
.project-gantt__row--empty .project-gantt__track {
  display: grid;
  align-items: center;
  padding: 0 16px;
}

.project-gantt__inline-state-chip {
  position: relative;
  z-index: 1;
  display: inline-flex;
  align-items: center;
  width: fit-content;
  max-width: min(420px, calc(100% - 12px));
  min-height: 34px;
  padding: 0 12px;
  border: 1px solid var(--dialog-control-border);
  border-radius: 12px;
  background: var(--dialog-control-bg-strong);
  color: var(--text-soft);
  font-size: 0.8rem;
  font-weight: 600;
}

.project-gantt__inline-state-chip--loading {
  border-color: var(--work-status-active-border);
  color: var(--work-status-active-color);
}

.project-gantt__inline-state-chip--error {
  border-color: var(--work-status-delayed-border);
  background: var(--work-status-delayed-bg);
  color: var(--work-status-delayed-color);
  cursor: pointer;
}

.project-gantt__bar:hover,
.project-gantt__bar:focus-visible {
  box-shadow: 0 16px 28px color-mix(in srgb, var(--text-main) 18%, transparent);
  filter: saturate(1.04);
  outline: none;
}

.project-gantt__bar--pending {
  background: var(--work-status-pending-strong);
  color: var(--work-status-pending-contrast);
}

.project-gantt__bar--active {
  background: var(--work-status-active-strong);
  color: var(--work-status-active-contrast);
}

.project-gantt__bar--done {
  background: var(--work-status-done-strong);
  color: var(--work-status-done-contrast);
}

.project-gantt__bar--delayed {
  background: var(--work-status-delayed-strong);
  color: var(--work-status-delayed-contrast);
}

.project-gantt__detail-card {
  position: fixed;
  z-index: 260;
  width: min(288px, calc(100vw - 32px));
  max-height: calc(100vh - 48px);
  overflow-y: auto;
  display: grid;
  gap: 10px;
  padding: 14px;
  border: 1px solid color-mix(in srgb, var(--accent-line) 38%, var(--border-soft));
  border-radius: 22px;
  background: linear-gradient(160deg, color-mix(in srgb, var(--accent-end) 18%, var(--panel-bg)), color-mix(in srgb, var(--accent-start) 24%, var(--panel-bg)));
  box-shadow:
    0 0 0 1px color-mix(in srgb, var(--accent-line) 58%, var(--accent-end)),
    0 24px 44px color-mix(in srgb, #061831 26%, transparent),
    inset 0 1px 0 color-mix(in srgb, #ffffff 14%, transparent);
  scrollbar-gutter: stable;
  pointer-events: auto;
}

.project-gantt__detail-card[data-placement='top-right'] {
  transform: translate(16px, calc(-100% - 14px));
  transform-origin: left bottom;
}

.project-gantt__detail-card[data-placement='top-left'] {
  transform: translate(calc(-100% - 16px), calc(-100% - 14px));
  transform-origin: right bottom;
}

.project-gantt__detail-card[data-placement='bottom-right'] {
  transform: translate(16px, 14px);
  transform-origin: left top;
}

.project-gantt__detail-card[data-placement='bottom-left'] {
  transform: translate(calc(-100% - 16px), 14px);
  transform-origin: right top;
}

.project-gantt__detail-topline {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
}

.project-gantt__detail-eyebrow {
  font-size: 0.66rem;
  letter-spacing: 0.16em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.project-gantt__detail-title,
.project-gantt__detail-status,
.project-gantt__detail-stat span,
.project-gantt__detail-stat strong,
.project-gantt__detail-stat small,
.project-gantt__detail-grid dt,
.project-gantt__detail-grid dd {
  margin: 0;
}

.project-gantt__detail-title {
  font-size: 0.96rem;
  line-height: 1.3;
  color: var(--text-main);
}

.project-gantt__detail-status {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 74px;
  padding: 6px 10px;
  border: 1px solid transparent;
  border-radius: 999px;
  font-size: 0.74rem;
  font-weight: 700;
  white-space: nowrap;
}

.project-gantt__detail-status--pending {
  border-color: var(--work-status-pending-border);
  background: var(--work-status-pending-bg);
  color: var(--work-status-pending-color);
}

.project-gantt__detail-status--active {
  border-color: var(--work-status-active-border);
  background: var(--work-status-active-bg);
  color: var(--work-status-active-color);
}

.project-gantt__detail-status--done {
  border-color: var(--work-status-done-border);
  background: var(--work-status-done-bg);
  color: var(--work-status-done-color);
}

.project-gantt__detail-status--delayed {
  border-color: var(--work-status-delayed-border);
  background: var(--work-status-delayed-bg);
  color: var(--work-status-delayed-color);
}

.project-gantt__detail-status--unknown {
  border-color: var(--work-status-unknown-border);
  background: var(--work-status-unknown-bg);
  color: var(--work-status-unknown-color);
}

.project-gantt__detail-stat {
  display: grid;
  gap: 3px;
  padding: 11px 12px;
  border: 1px solid color-mix(in srgb, var(--accent-line) 18%, transparent);
  border-radius: 16px;
  background: linear-gradient(180deg, color-mix(in srgb, var(--panel-bg) 92%, var(--app-bg)), color-mix(in srgb, var(--accent-end) 10%, var(--panel-bg)));
}

.project-gantt__detail-stat span {
  color: var(--text-soft);
  font-size: 0.68rem;
  letter-spacing: 0.08em;
  text-transform: uppercase;
}

.project-gantt__detail-stat strong {
  font-size: 1.12rem;
  line-height: 1.1;
  color: var(--text-main);
}

.project-gantt__detail-stat small {
  color: var(--text-soft);
  font-size: 0.74rem;
  font-weight: 600;
}

.project-gantt__detail-stat--pending {
  border-color: var(--work-status-pending-border);
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--panel-bg) 82%, transparent), color-mix(in srgb, var(--panel-bg) 66%, transparent)),
    radial-gradient(circle at 100% 0%, color-mix(in srgb, var(--work-status-pending-color) 18%, transparent), transparent 62%);
}

.project-gantt__detail-stat--active {
  border-color: var(--work-status-active-border);
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--panel-bg) 82%, transparent), color-mix(in srgb, var(--panel-bg) 66%, transparent)),
    radial-gradient(circle at 100% 0%, color-mix(in srgb, var(--work-status-active-color) 22%, transparent), transparent 58%);
}

.project-gantt__detail-stat--done {
  border-color: var(--work-status-done-border);
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--panel-bg) 82%, transparent), color-mix(in srgb, var(--panel-bg) 66%, transparent)),
    radial-gradient(circle at 100% 0%, color-mix(in srgb, var(--work-status-done-color) 18%, transparent), transparent 58%);
}

.project-gantt__detail-stat--delayed {
  border-color: var(--work-status-delayed-border);
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--panel-bg) 82%, transparent), color-mix(in srgb, var(--panel-bg) 66%, transparent)),
    radial-gradient(circle at 100% 0%, color-mix(in srgb, var(--work-status-delayed-color) 20%, transparent), transparent 58%);
}

.project-gantt__detail-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 8px;
}

.project-gantt__detail-grid-item {
  display: grid;
  gap: 4px;
  min-height: 68px;
  padding: 10px 11px;
  border: 1px solid color-mix(in srgb, var(--border-soft) 92%, transparent);
  border-radius: 14px;
  background: linear-gradient(180deg, color-mix(in srgb, var(--panel-bg) 92%, var(--app-bg)), color-mix(in srgb, var(--accent-end) 10%, var(--panel-bg)));
}

.project-gantt__detail-grid-item dt {
  color: var(--text-soft);
  font-size: 0.7rem;
  font-weight: 600;
}

.project-gantt__detail-grid-item dd {
  color: var(--text-main);
  font-size: 0.82rem;
  font-weight: 700;
  line-height: 1.45;
}

.project-gantt__detail-grid-item--completion {
  grid-column: 1 / -1;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--accent-success) 10%, transparent), color-mix(in srgb, var(--panel-bg) 82%, transparent)),
    color-mix(in srgb, var(--panel-bg) 82%, transparent);
}

.project-gantt__detail-grid-item--completion.is-empty {
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--text-muted) 10%, transparent), color-mix(in srgb, var(--panel-bg) 82%, transparent)),
    color-mix(in srgb, var(--panel-bg) 82%, transparent);
}

.project-gantt__subtask-detail-card {
  position: fixed;
  z-index: 260;
  width: min(288px, calc(100vw - 32px));
  max-height: calc(100vh - 48px);
  overflow-y: auto;
  display: grid;
  gap: 10px;
  padding: 14px;
  border: 1px solid color-mix(in srgb, var(--accent-line) 38%, var(--border-soft));
  border-radius: 22px;
  background: linear-gradient(160deg, color-mix(in srgb, var(--accent-end) 18%, var(--panel-bg)), color-mix(in srgb, var(--accent-start) 24%, var(--panel-bg)));
  box-shadow:
    0 0 0 1px color-mix(in srgb, var(--accent-line) 58%, var(--accent-end)),
    0 24px 44px color-mix(in srgb, #061831 26%, transparent),
    inset 0 1px 0 color-mix(in srgb, #ffffff 14%, transparent);
  pointer-events: auto;
}

.project-gantt__subtask-detail-card[data-placement='top-right'] {
  transform: translate(16px, calc(-100% - 14px));
  transform-origin: left bottom;
}

.project-gantt__subtask-detail-card[data-placement='top-left'] {
  transform: translate(calc(-100% - 16px), calc(-100% - 14px));
  transform-origin: right bottom;
}

.project-gantt__subtask-detail-card[data-placement='bottom-right'] {
  transform: translate(16px, 14px);
  transform-origin: left top;
}

.project-gantt__subtask-detail-card[data-placement='bottom-left'] {
  transform: translate(calc(-100% - 16px), 14px);
  transform-origin: right top;
}

.project-gantt__subtask-detail-topline {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
}

.project-gantt__subtask-detail-eyebrow {
  font-size: 0.66rem;
  letter-spacing: 0.16em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.project-gantt__subtask-detail-title,
.project-gantt__subtask-detail-status,
.project-gantt__subtask-detail-grid dt,
.project-gantt__subtask-detail-grid dd {
  margin: 0;
}

.project-gantt__subtask-detail-title {
  font-size: 0.96rem;
  line-height: 1.3;
  color: var(--text-main);
}

.project-gantt__subtask-detail-status {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 74px;
  padding: 6px 10px;
  border: 1px solid transparent;
  border-radius: 999px;
  font-size: 0.74rem;
  font-weight: 700;
  white-space: nowrap;
}

.project-gantt__subtask-detail-status--pending {
  border-color: var(--work-status-pending-border);
  background: var(--work-status-pending-bg);
  color: var(--work-status-pending-color);
}

.project-gantt__subtask-detail-status--active {
  border-color: var(--work-status-active-border);
  background: var(--work-status-active-bg);
  color: var(--work-status-active-color);
}

.project-gantt__subtask-detail-status--done {
  border-color: var(--work-status-done-border);
  background: var(--work-status-done-bg);
  color: var(--work-status-done-color);
}

.project-gantt__subtask-detail-status--delayed {
  border-color: var(--work-status-delayed-border);
  background: var(--work-status-delayed-bg);
  color: var(--work-status-delayed-color);
}

.project-gantt__subtask-detail-progress {
  display: grid;
  gap: 8px;
  padding: 11px 12px;
  border: 1px solid color-mix(in srgb, var(--accent-line) 18%, transparent);
  border-radius: 16px;
  background: linear-gradient(180deg, color-mix(in srgb, var(--panel-bg) 92%, var(--app-bg)), color-mix(in srgb, var(--accent-end) 10%, var(--panel-bg)));
}

.project-gantt__subtask-detail-progress-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  color: var(--text-soft);
  font-size: 0.74rem;
  font-weight: 600;
}

.project-gantt__subtask-detail-progress-head strong {
  color: var(--text-main);
  font-size: 1rem;
}

.project-gantt__subtask-detail-progress-track {
  height: 8px;
  overflow: hidden;
  border-radius: 999px;
  background: color-mix(in srgb, var(--accent-line) 14%, transparent);
}

.project-gantt__subtask-detail-progress-fill {
  display: block;
  height: 100%;
  border-radius: inherit;
  background: linear-gradient(90deg, var(--accent-start), var(--accent-end));
}

.project-gantt__subtask-detail-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 8px;
}

.project-gantt__subtask-detail-grid-item {
  display: grid;
  gap: 4px;
  min-height: 68px;
  padding: 10px 11px;
  border: 1px solid color-mix(in srgb, var(--border-soft) 92%, transparent);
  border-radius: 14px;
  background: linear-gradient(180deg, color-mix(in srgb, var(--panel-bg) 92%, var(--app-bg)), color-mix(in srgb, var(--accent-end) 10%, var(--panel-bg)));
}

.project-gantt__subtask-detail-grid-item:last-child {
  grid-column: 1 / -1;
}

.project-gantt__subtask-detail-grid-item dt {
  color: var(--text-soft);
  font-size: 0.7rem;
  font-weight: 600;
}

.project-gantt__subtask-detail-grid-item dd {
  color: var(--text-main);
  font-size: 0.82rem;
  font-weight: 700;
  line-height: 1.45;
}

@media (max-width: 1100px) {
  .project-gantt__body {
    grid-template-columns: minmax(200px, 220px) minmax(0, 1fr);
  }

  .project-gantt__toolbar {
    flex-direction: column;
    align-items: stretch;
  }

  .project-gantt__toolbar-actions {
    width: 100%;
    justify-items: stretch;
  }

  .project-gantt__toolbar-scale-row {
    width: 100%;
    justify-content: space-between;
  }

  .project-gantt__tree-actions {
    justify-content: flex-end;
  }

  .project-gantt__tree-actions button {
    flex: 1 1 auto;
  }

  .project-gantt__perspective-switch {
    width: 100%;
    justify-content: space-between;
  }

  .project-gantt__detail-grid {
    grid-template-columns: 1fr;
  }

  .project-gantt__detail-grid-item--completion {
    grid-column: auto;
  }

  .project-gantt__subtask-detail-grid {
    grid-template-columns: 1fr;
  }

  .project-gantt__subtask-detail-grid-item:last-child {
    grid-column: auto;
  }
}
</style>
