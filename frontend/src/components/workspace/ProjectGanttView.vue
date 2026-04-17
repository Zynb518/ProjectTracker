<script setup lang="ts">
import { computed, onBeforeUnmount, ref } from 'vue'

import GanttScaleSwitcher from '@/components/workspace/GanttScaleSwitcher.vue'
import type { GanttNodeSummary, GanttPerspective, GanttScale, ProjectStageGantt } from '@/types/gantt'
import { getWorkStatusLabel, getWorkStatusTone } from '@/utils/display'
import { buildGanttAxisItems, getGanttBarLayout } from '@/utils/gantt'
import { normalizeWheelDelta } from '@/utils/smoothWheel'

const props = withDefaults(defineProps<{
  error?: string | null
  gantt: ProjectStageGantt | null
  isLoading?: boolean
  perspective?: GanttPerspective
  scale: GanttScale
}>(), {
  error: null,
  isLoading: false,
  perspective: 'stage',
})

const emit = defineEmits<{
  'open-node': [nodeId: number]
  retry: []
  'update:perspective': [perspective: GanttPerspective]
  'update:scale': [scale: GanttScale]
}>()

type HoverCardPlacement = 'bottom-left' | 'bottom-right' | 'top-left' | 'top-right'

const hoveredNodeId = ref<number | null>(null)
const hoveredNodeAnchor = ref({ x: 0, y: 0 })
const hoveredNodePlacement = ref<HoverCardPlacement>('top-right')
let hideDetailTimer: ReturnType<typeof window.setTimeout> | null = null

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

const hoveredNode = computed(() =>
  props.gantt?.nodes.find((node) => node.id === hoveredNodeId.value) ?? null,
)

const detailCardStyle = computed(() => ({
  left: `${hoveredNodeAnchor.value.x}px`,
  top: `${hoveredNodeAnchor.value.y}px`,
}))

function getScheduleDays(startDate: string, endDate: string) {
  const start = new Date(`${startDate}T00:00:00`)
  const end = new Date(`${endDate}T00:00:00`)

  return Math.max(1, Math.round((end.getTime() - start.getTime()) / 86400000) + 1)
}

function formatCompletedAt(value: string) {
  return value.slice(0, 16).replace('T', ' ')
}

function getBarStyle(node: GanttNodeSummary) {
  const layout = getGanttBarLayout(
    axisStartDate.value,
    node.planned_start_date,
    node.planned_end_date,
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

function showNodeDetail(nodeId: number, event?: FocusEvent | MouseEvent) {
  cancelHideNodeDetail()
  hoveredNodeId.value = nodeId
  hoveredNodeAnchor.value = getHoverPoint(event)
  hoveredNodePlacement.value = getHoverPlacement(hoveredNodeAnchor.value)
}

function cancelHideNodeDetail() {
  if (hideDetailTimer !== null) {
    window.clearTimeout(hideDetailTimer)
    hideDetailTimer = null
  }
}

function scheduleHideNodeDetail() {
  cancelHideNodeDetail()

  hideDetailTimer = window.setTimeout(() => {
    hoveredNodeId.value = null
    hideDetailTimer = null
  }, DETAIL_HIDE_DELAY_MS)
}

function clearNodeDetail(nodeId?: number) {
  if (nodeId !== undefined && hoveredNodeId.value !== nodeId) {
    return
  }

  scheduleHideNodeDetail()
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
  cancelHideNodeDetail()
})
</script>

<template>
  <section class="project-gantt" data-testid="project-gantt-view">
    <header class="project-gantt__toolbar">
      <div class="project-gantt__toolbar-copy">
        <p class="project-gantt__eyebrow">Timeline Matrix</p>
        <div>
          <h3>阶段甘特图</h3>
          <p>顶部按时间粒度缩放，左侧按阶段查看，点击某个阶段即可下钻到子任务时间视图。</p>
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

        <GanttScaleSwitcher :scale="scale" @update:scale="emit('update:scale', $event)" />
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
                v-for="node in gantt.nodes"
                :key="node.id"
                :data-testid="`project-gantt-row-label-${node.id}`"
                class="project-gantt__sidebar-row"
              >
                <strong>{{ node.name }}</strong>
                <span
                  :class="[
                    'project-gantt__status-pill',
                    `project-gantt__status-pill--${getWorkStatusTone(node.status)}`,
                  ]"
                >
                  {{ getWorkStatusLabel(node.status) }}
                </span>
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
                <div v-for="node in gantt.nodes" :key="node.id" class="project-gantt__row">
                  <div class="project-gantt__track">
                    <div
                      :data-testid="`project-gantt-track-grid-${node.id}`"
                      aria-hidden="true"
                      class="project-gantt__track-grid"
                    >
                      <div
                        v-for="item in axisItems"
                        :key="`${node.id}-${item.key}`"
                        :class="['project-gantt__track-cell', `project-gantt__track-cell--${scale}`]"
                        :style="{ width: `${item.widthPx}px` }"
                      />
                    </div>
                    <button
                      :data-testid="`project-gantt-stage-bar-${node.id}`"
                      :class="[
                        'project-gantt__bar',
                        `project-gantt__bar--${getWorkStatusTone(node.status)}`,
                      ]"
                      :style="getBarStyle(node)"
                      :title="`${node.name}｜${getWorkStatusLabel(node.status)}｜${node.planned_start_date} - ${node.planned_end_date}`"
                      type="button"
                      @blur="clearNodeDetail(node.id)"
                      @click="emit('open-node', node.id)"
                      @focus="showNodeDetail(node.id, $event)"
                      @mouseleave="clearNodeDetail(node.id)"
                      @mouseenter="showNodeDetail(node.id, $event)"
                    >
                      <span>{{ node.name }}</span>
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

.project-gantt__bar span {
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  font-size: 0.84rem;
  font-weight: 600;
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
}
</style>
