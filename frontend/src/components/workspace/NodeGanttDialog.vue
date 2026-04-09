<script setup lang="ts">
import { computed, onBeforeUnmount, ref } from 'vue'

import type { GanttScale, GanttSubtaskSummary, ProjectNodeSubtaskGantt } from '@/types/gantt'
import { getWorkStatusLabel, getWorkStatusTone } from '@/utils/display'
import { buildGanttAxisItems, getGanttBarLayout } from '@/utils/gantt'

const props = withDefaults(defineProps<{
  error?: string | null
  gantt: ProjectNodeSubtaskGantt | null
  isLoading?: boolean
  modelValue: boolean
  scale: GanttScale
}>(), {
  error: null,
  isLoading: false,
})

const emit = defineEmits<{
  retry: []
  'update:modelValue': [value: boolean]
}>()

type HoverCardPlacement = 'bottom-left' | 'bottom-right' | 'top-left' | 'top-right'

const hoveredSubtaskId = ref<number | null>(null)
const hoveredSubtaskAnchor = ref({ x: 0, y: 0 })
const hoveredSubtaskPlacement = ref<HoverCardPlacement>('top-right')
let hideDetailTimer: ReturnType<typeof window.setTimeout> | null = null

const DETAIL_HIDE_DELAY_MS = 180
const DETAIL_CARD_EDGE_GAP_PX = 24
const DETAIL_CARD_HEIGHT_HINT_PX = 220
const DETAIL_CARD_WIDTH_PX = 288
const axisScrollRef = ref<HTMLElement | null>(null)
const rowsScrollRef = ref<HTMLElement | null>(null)

const dialogRange = computed(() => {
  if (props.gantt === null) {
    return null
  }

  const dates = [
    props.gantt.node.planned_start_date,
    props.gantt.node.planned_end_date,
    ...props.gantt.subtasks.flatMap((subtask) => [subtask.planned_start_date, subtask.planned_end_date]),
  ].sort()

  return {
    startDate: dates[0],
    endDate: dates[dates.length - 1],
  }
})

const axisItems = computed(() => {
  if (dialogRange.value === null) {
    return []
  }

  return buildGanttAxisItems(dialogRange.value.startDate, dialogRange.value.endDate, props.scale)
})

const axisStartDate = computed(() => axisItems.value[0]?.startDate ?? dialogRange.value?.startDate ?? '')
const canvasWidth = computed(() => axisItems.value.reduce((total, item) => total + item.widthPx, 0))
const timelineCanvasStyle = computed(() => ({
  width: `${canvasWidth.value}px`,
}))

const hoveredSubtask = computed(() =>
  props.gantt?.subtasks.find((subtask) => subtask.id === hoveredSubtaskId.value) ?? null,
)

const detailCardStyle = computed(() => ({
  left: `${hoveredSubtaskAnchor.value.x}px`,
  top: `${hoveredSubtaskAnchor.value.y}px`,
}))

type PriorityTone = 'high' | 'low' | 'medium' | 'unknown'

function getPriorityLabel(priority: number) {
  return (
    {
      1: '低',
      2: '中',
      3: '高',
    }[priority] ?? '未设定'
  )
}

function getPriorityTone(priority: number): PriorityTone {
  if (priority === 1) {
    return 'low'
  }

  if (priority === 2) {
    return 'medium'
  }

  if (priority === 3) {
    return 'high'
  }

  return 'unknown'
}

function closeDialog() {
  emit('update:modelValue', false)
}

function getBarStyle(subtask: GanttSubtaskSummary) {
  const layout = getGanttBarLayout(
    axisStartDate.value,
    subtask.planned_start_date,
    subtask.planned_end_date,
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

function showSubtaskDetail(subtaskId: number, event?: FocusEvent | MouseEvent) {
  cancelHideSubtaskDetail()
  hoveredSubtaskId.value = subtaskId
  hoveredSubtaskAnchor.value = getHoverPoint(event)
  hoveredSubtaskPlacement.value = getHoverPlacement(hoveredSubtaskAnchor.value)
}

function cancelHideSubtaskDetail() {
  if (hideDetailTimer !== null) {
    window.clearTimeout(hideDetailTimer)
    hideDetailTimer = null
  }
}

function scheduleHideSubtaskDetail() {
  cancelHideSubtaskDetail()

  hideDetailTimer = window.setTimeout(() => {
    hoveredSubtaskId.value = null
    hideDetailTimer = null
  }, DETAIL_HIDE_DELAY_MS)
}

function clearSubtaskDetail(subtaskId?: number) {
  if (subtaskId !== undefined && hoveredSubtaskId.value !== subtaskId) {
    return
  }

  scheduleHideSubtaskDetail()
}

function syncHorizontalScroll(source: HTMLElement | null, target: HTMLElement | null) {
  if (source === null || target === null || source.scrollLeft === target.scrollLeft) {
    return
  }

  target.scrollLeft = source.scrollLeft
}

function handleAxisScroll() {
  syncHorizontalScroll(axisScrollRef.value, rowsScrollRef.value)
}

function handleRowsScroll() {
  syncHorizontalScroll(rowsScrollRef.value, axisScrollRef.value)
}

onBeforeUnmount(() => {
  cancelHideSubtaskDetail()
})
</script>

<template>
  <Transition name="node-gantt-dialog">
    <div
      v-if="modelValue"
      class="node-gantt-dialog__backdrop"
      data-testid="node-gantt-dialog-backdrop"
      @click.self="closeDialog"
    >
      <section class="node-gantt-dialog" data-testid="node-gantt-dialog">
        <header class="node-gantt-dialog__header">
          <div v-if="gantt" class="node-gantt-dialog__copy">
            <p class="node-gantt-dialog__eyebrow">Node Breakdown</p>
            <div>
              <h3>{{ gantt.node.name }}</h3>
              <p>
                {{ getWorkStatusLabel(gantt.node.status) }} ·
                {{ gantt.node.planned_start_date }} 至 {{ gantt.node.planned_end_date }}
              </p>
            </div>
          </div>

          <button
            aria-label="关闭阶段子任务甘特图"
            class="node-gantt-dialog__close"
            type="button"
            @click="closeDialog"
          >
            <svg viewBox="0 0 24 24">
              <path
                d="M7 7 17 17 M17 7 7 17"
                fill="none"
                stroke="currentColor"
                stroke-linecap="round"
                stroke-linejoin="round"
                stroke-width="1.8"
              />
            </svg>
          </button>
        </header>

        <p v-if="isLoading" class="node-gantt-dialog__state loading-panel">子任务甘特图加载中...</p>

        <section
          v-else-if="error"
          class="node-gantt-dialog__state node-gantt-dialog__state--error"
        >
          <div>
            <strong>子任务时间视图暂时不可用</strong>
            <p>{{ error }}</p>
          </div>

          <button type="button" @click="emit('retry')">重试</button>
        </section>

        <section
          v-else-if="gantt === null || gantt.subtasks.length === 0"
          class="node-gantt-dialog__state"
        >
          该阶段下暂无子任务排期。
        </section>

        <div
          v-else
          v-smooth-wheel
          class="node-gantt-dialog__body-scroll smooth-scroll-surface"
        >
          <div class="node-gantt-dialog__body">
            <aside class="node-gantt-dialog__sidebar">
              <div class="node-gantt-dialog__sidebar-head">
                <span>子任务</span>
                <span>状态</span>
              </div>

              <div
                v-for="subtask in gantt.subtasks"
                :key="subtask.id"
                :data-testid="`node-gantt-row-label-${subtask.id}`"
                class="node-gantt-dialog__sidebar-row"
              >
                <strong>{{ subtask.name }}</strong>
                <span
                  :class="[
                    'node-gantt-dialog__status-pill',
                    `node-gantt-dialog__status-pill--${getWorkStatusTone(subtask.status)}`,
                  ]"
                >
                  {{ getWorkStatusLabel(subtask.status) }}
                </span>
              </div>
            </aside>

            <div class="node-gantt-dialog__timeline-shell">
              <div
                ref="axisScrollRef"
                data-testid="node-gantt-axis-scroll"
                v-smooth-wheel="{ axis: 'horizontal', wheelBehavior: 'block' }"
                class="node-gantt-dialog__axis-scroll smooth-scroll-surface"
                @scroll="handleAxisScroll"
              >
                <div class="node-gantt-dialog__timeline-canvas" :style="timelineCanvasStyle">
                  <div class="node-gantt-dialog__axis" data-testid="node-gantt-axis">
                    <div
                      v-for="item in axisItems"
                      :key="item.key"
                      :class="['node-gantt-dialog__axis-cell', `node-gantt-dialog__axis-cell--${scale}`]"
                      :style="{ width: `${item.widthPx}px` }"
                      :title="`${item.startDate} 至 ${item.endDate}`"
                    >
                      {{ item.label }}
                    </div>
                  </div>
                </div>
              </div>

              <div
                ref="rowsScrollRef"
                data-testid="node-gantt-rows-scroll"
                v-smooth-wheel="{ axis: 'horizontal', wheelBehavior: 'block' }"
                class="node-gantt-dialog__timeline-scroll smooth-scroll-surface"
                @scroll="handleRowsScroll"
              >
                <div class="node-gantt-dialog__timeline-canvas" :style="timelineCanvasStyle">
                  <div class="node-gantt-dialog__rows">
                    <div v-for="subtask in gantt.subtasks" :key="subtask.id" class="node-gantt-dialog__row">
                      <div class="node-gantt-dialog__track">
                        <div
                          :data-testid="`node-gantt-track-grid-${subtask.id}`"
                          aria-hidden="true"
                          class="node-gantt-dialog__track-grid"
                        >
                          <div
                            v-for="item in axisItems"
                            :key="`${subtask.id}-${item.key}`"
                            :class="['node-gantt-dialog__track-cell', `node-gantt-dialog__track-cell--${scale}`]"
                            :style="{ width: `${item.widthPx}px` }"
                          />
                        </div>
                        <button
                          :data-testid="`node-gantt-bar-${subtask.id}`"
                          :class="[
                            'node-gantt-dialog__bar',
                            `node-gantt-dialog__bar--${getWorkStatusTone(subtask.status)}`,
                          ]"
                          :style="getBarStyle(subtask)"
                          :title="`${subtask.name}｜${subtask.responsible_real_name}｜${subtask.progress_percent}%`"
                          type="button"
                          @blur="clearSubtaskDetail(subtask.id)"
                          @focus="showSubtaskDetail(subtask.id, $event)"
                          @mouseleave="clearSubtaskDetail(subtask.id)"
                          @mouseenter="showSubtaskDetail(subtask.id, $event)"
                        >
                          <span>{{ subtask.name }}</span>
                        </button>
                      </div>
                    </div>
                  </div>
                </div>
              </div>

              <Teleport to="body">
                <aside
                  v-if="hoveredSubtask"
                  :data-placement="hoveredSubtaskPlacement"
                  :style="detailCardStyle"
                  class="node-gantt-dialog__detail-card"
                  data-testid="node-gantt-detail-card"
                  @mouseenter="cancelHideSubtaskDetail"
                  @mouseleave="clearSubtaskDetail()"
                >
                  <div class="node-gantt-dialog__detail-topline">
                    <span class="node-gantt-dialog__detail-eyebrow">子任务详情</span>
                    <span
                      :class="[
                        'node-gantt-dialog__detail-status',
                        `node-gantt-dialog__detail-status--${getWorkStatusTone(hoveredSubtask.status)}`,
                      ]"
                    >
                      {{ getWorkStatusLabel(hoveredSubtask.status) }}
                    </span>
                  </div>

                  <strong class="node-gantt-dialog__detail-title">{{ hoveredSubtask.name }}</strong>

                  <div class="node-gantt-dialog__detail-meta">
                    <span
                      :class="[
                        'node-gantt-dialog__detail-priority',
                        `node-gantt-dialog__detail-priority--${getPriorityTone(hoveredSubtask.priority)}`,
                      ]"
                    >
                      {{ getPriorityLabel(hoveredSubtask.priority) }}优先级
                    </span>
                  </div>

                  <section class="node-gantt-dialog__detail-progress" aria-label="子任务进度">
                    <div class="node-gantt-dialog__detail-progress-head">
                      <span>完成进度</span>
                      <strong>{{ hoveredSubtask.progress_percent }}%</strong>
                    </div>
                    <div class="node-gantt-dialog__detail-progress-track">
                      <span
                        class="node-gantt-dialog__detail-progress-fill"
                        data-testid="node-gantt-detail-progress-fill"
                        :style="{ width: `${hoveredSubtask.progress_percent}%` }"
                      />
                    </div>
                  </section>

                  <dl class="node-gantt-dialog__detail-grid">
                    <div class="node-gantt-dialog__detail-grid-item">
                      <dt>负责人</dt>
                      <dd>{{ hoveredSubtask.responsible_real_name }}</dd>
                    </div>
                    <div class="node-gantt-dialog__detail-grid-item">
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
    </div>
  </Transition>
</template>

<style scoped>
.node-gantt-dialog__backdrop {
  position: fixed;
  inset: 0;
  z-index: 170;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 28px;
  background: var(--overlay-backdrop);
  backdrop-filter: blur(16px);
}

.node-gantt-dialog {
  width: min(1240px, calc(100vw - 56px));
  max-height: min(80vh, calc(100vh - 32px));
  overflow: hidden;
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
  gap: 18px;
  padding: 24px;
  border: 1px solid var(--dialog-surface-border);
  border-radius: 28px;
  background: var(--dialog-surface-bg), var(--dialog-surface-glow), var(--card-sheen);
  box-shadow: var(--dialog-surface-shadow);
  backdrop-filter: var(--backdrop-blur);
}

.node-gantt-dialog__header,
.node-gantt-dialog__body-scroll,
.node-gantt-dialog__state {
  min-width: 0;
}

.node-gantt-dialog__header {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 16px;
}

.node-gantt-dialog__copy {
  display: grid;
  gap: 10px;
}

.node-gantt-dialog__eyebrow,
.node-gantt-dialog__copy h3,
.node-gantt-dialog__copy p {
  margin: 0;
}

.node-gantt-dialog__eyebrow {
  font-size: 0.74rem;
  letter-spacing: 0.18em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.node-gantt-dialog__copy h3 {
  font-size: 1.42rem;
  color: var(--text-main);
}

.node-gantt-dialog__copy p,
.node-gantt-dialog__detail-card p,
.node-gantt-dialog__detail-card dt {
  color: var(--text-soft);
}

.node-gantt-dialog__close {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 42px;
  height: 42px;
  border: 0;
  border-radius: 14px;
  background: var(--dialog-control-bg);
  box-shadow: var(--dialog-control-shadow);
  color: var(--text-main);
  cursor: pointer;
}

.node-gantt-dialog__close svg {
  width: 18px;
  height: 18px;
}

.node-gantt-dialog__state {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 18px;
  min-height: 220px;
  padding: 24px;
  border: 1px solid var(--meta-surface-border);
  border-radius: 22px;
  background: var(--meta-surface-bg), var(--meta-surface-glow);
  box-shadow: var(--meta-surface-shadow);
  color: var(--text-soft);
}

.node-gantt-dialog__state--error {
  align-items: flex-start;
}

.node-gantt-dialog__state--error strong,
.node-gantt-dialog__state--error p {
  margin: 0;
}

.node-gantt-dialog__state button {
  padding: 10px 16px;
  border: 0;
  border-radius: 12px;
  background: linear-gradient(135deg, var(--accent-start), var(--accent-end));
  color: #f8fbff;
  font-weight: 600;
  cursor: pointer;
}

.node-gantt-dialog__body-scroll {
  min-height: 0;
  overflow: auto;
  padding-right: 4px;
}

.node-gantt-dialog__body {
  min-height: 0;
  min-width: 0;
  display: grid;
  grid-template-columns: minmax(240px, 280px) minmax(0, 1fr);
  gap: 18px;
}

.node-gantt-dialog__sidebar {
  display: grid;
  gap: 10px;
}

.node-gantt-dialog__sidebar-head,
.node-gantt-dialog__sidebar-row {
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  gap: 12px;
  align-items: center;
  min-height: 62px;
  padding: 14px 16px;
  border: 1px solid var(--dialog-control-border);
  border-radius: 18px;
  background: var(--dialog-control-bg), var(--meta-surface-glow);
  box-shadow: var(--dialog-control-shadow);
}

.node-gantt-dialog__sidebar-row {
  overflow: hidden;
}

.node-gantt-dialog__sidebar-head {
  position: sticky;
  top: 0;
  z-index: 3;
  min-height: 56px;
  font-size: 0.78rem;
  letter-spacing: 0.12em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.node-gantt-dialog__sidebar-row strong {
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  font-size: 0.98rem;
  color: var(--text-main);
}

.node-gantt-dialog__status-pill {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 72px;
  padding: 8px 12px;
  border-radius: 999px;
  font-size: 0.78rem;
  font-weight: 600;
  flex-shrink: 0;
  white-space: nowrap;
}

.node-gantt-dialog__status-pill--pending {
  background: color-mix(in srgb, var(--text-muted) 18%, transparent);
  color: var(--text-soft);
}

.node-gantt-dialog__status-pill--active {
  background: linear-gradient(135deg, color-mix(in srgb, var(--accent-start) 78%, #0d2754 22%), color-mix(in srgb, var(--accent-end) 68%, #0f2c5b 32%));
  color: #f7fbff;
  box-shadow:
    inset 0 1px 0 color-mix(in srgb, #ffffff 18%, transparent),
    0 8px 16px color-mix(in srgb, var(--accent-end) 14%, transparent);
}

.node-gantt-dialog__status-pill--done {
  background: color-mix(in srgb, var(--text-muted) 20%, transparent);
  color: color-mix(in srgb, var(--text-soft) 94%, transparent);
}

.node-gantt-dialog__status-pill--delayed {
  background: color-mix(in srgb, var(--accent-warning) 18%, transparent);
  color: var(--accent-warning);
}

.node-gantt-dialog__timeline-shell {
  min-width: 0;
  display: grid;
  gap: 10px;
}

.node-gantt-dialog__axis-scroll {
  position: sticky;
  top: 0;
  z-index: 4;
  overflow-x: scroll;
  overflow-y: hidden;
  padding-bottom: 4px;
  scrollbar-gutter: stable;
  scrollbar-width: thin;
  scrollbar-color: color-mix(in srgb, var(--accent-end) 58%, var(--panel-bg)) color-mix(in srgb, var(--panel-bg) 84%, transparent);
}

.node-gantt-dialog__axis-scroll::-webkit-scrollbar {
  height: 12px;
}

.node-gantt-dialog__axis-scroll::-webkit-scrollbar-track {
  border-radius: 999px;
  background: color-mix(in srgb, var(--panel-bg) 84%, transparent);
}

.node-gantt-dialog__axis-scroll::-webkit-scrollbar-thumb {
  border: 2px solid transparent;
  border-radius: 999px;
  background: linear-gradient(90deg, color-mix(in srgb, var(--accent-start) 72%, #0d2754 28%), color-mix(in srgb, var(--accent-end) 72%, #0f2c5b 28%));
  background-clip: padding-box;
}

.node-gantt-dialog__timeline-scroll {
  overflow-x: hidden;
  overflow-y: hidden;
  padding-bottom: 0;
}

.node-gantt-dialog__timeline-canvas {
  min-width: 100%;
  box-sizing: content-box;
}

.node-gantt-dialog__axis {
  display: flex;
  min-height: 56px;
  background: color-mix(in srgb, var(--glass-bg) 96%, transparent);
  backdrop-filter: blur(12px);
}

.node-gantt-dialog__axis-cell {
  flex: 0 0 auto;
  display: flex;
  align-items: center;
  justify-content: flex-start;
  padding: 0 12px;
  border-top: 1px solid color-mix(in srgb, var(--accent-line) 20%, transparent);
  border-bottom: 1px solid color-mix(in srgb, var(--accent-line) 20%, transparent);
  border-right: 1px solid color-mix(in srgb, var(--accent-line) 16%, transparent);
  font-size: 0.8rem;
  color: var(--text-soft);
  white-space: nowrap;
  overflow: hidden;
  background: linear-gradient(180deg, color-mix(in srgb, var(--accent-end) 12%, var(--dialog-control-bg-strong)), color-mix(in srgb, var(--accent-start) 6%, var(--dialog-control-bg)));
  box-shadow: inset 0 1px 0 color-mix(in srgb, #ffffff 14%, transparent);
}

.node-gantt-dialog__axis-cell:nth-child(even) {
  background: linear-gradient(180deg, color-mix(in srgb, var(--accent-start) 10%, var(--dialog-control-bg-strong)), color-mix(in srgb, var(--accent-end) 4%, var(--dialog-control-bg)));
}

.node-gantt-dialog__axis-cell--day {
  justify-content: center;
  padding: 0 6px;
  font-size: 0.72rem;
  font-weight: 600;
  letter-spacing: 0.01em;
  font-variant-numeric: tabular-nums;
}

.node-gantt-dialog__axis-cell--week,
.node-gantt-dialog__axis-cell--month {
  font-variant-numeric: tabular-nums;
}

.node-gantt-dialog__axis-cell:first-child {
  border-left: 1px solid color-mix(in srgb, var(--accent-line) 20%, transparent);
  border-top-left-radius: 16px;
  border-bottom-left-radius: 16px;
}

.node-gantt-dialog__axis-cell:last-child {
  border-top-right-radius: 16px;
  border-bottom-right-radius: 16px;
}

.node-gantt-dialog__rows {
  display: grid;
  gap: 10px;
}

.node-gantt-dialog__row {
  min-height: 62px;
}

.node-gantt-dialog__track {
  position: relative;
  min-height: 62px;
  overflow: hidden;
  border: 1px solid color-mix(in srgb, var(--accent-line) 18%, transparent);
  border-radius: 18px;
  background: linear-gradient(180deg, color-mix(in srgb, var(--accent-end) 6%, var(--dialog-control-bg-strong)), color-mix(in srgb, var(--accent-start) 3%, var(--dialog-control-bg)));
}

.node-gantt-dialog__track-grid {
  position: absolute;
  inset: 0;
  display: flex;
  pointer-events: auto;
}

.node-gantt-dialog__track-cell {
  flex: 0 0 auto;
  height: 100%;
  border-right: 1px solid color-mix(in srgb, var(--accent-line) 14%, transparent);
  background: linear-gradient(180deg, color-mix(in srgb, var(--accent-end) 8%, var(--dialog-control-bg-strong)), color-mix(in srgb, var(--accent-end) 4%, var(--dialog-control-bg)));
}

.node-gantt-dialog__track-cell:nth-child(even) {
  background: linear-gradient(180deg, color-mix(in srgb, var(--accent-start) 8%, var(--dialog-control-bg-strong)), color-mix(in srgb, var(--accent-start) 3%, var(--dialog-control-bg)));
}

.node-gantt-dialog__track-cell--day {
  background: linear-gradient(180deg, color-mix(in srgb, var(--accent-end) 10%, var(--dialog-control-bg-strong)), color-mix(in srgb, var(--accent-end) 4%, var(--dialog-control-bg)));
}

.node-gantt-dialog__bar {
  position: absolute;
  top: 50%;
  display: inline-flex;
  align-items: center;
  height: 38px;
  padding: 0 14px;
  border: 0;
  border-radius: 14px;
  cursor: default;
  transform: translateY(-50%);
  z-index: 1;
  transition:
    transform 180ms ease-out,
    box-shadow 180ms ease-out,
    filter 180ms ease-out;
}

.node-gantt-dialog__bar span {
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  font-size: 0.84rem;
  font-weight: 600;
}

.node-gantt-dialog__bar:hover,
.node-gantt-dialog__bar:focus-visible {
  box-shadow: 0 16px 28px color-mix(in srgb, var(--text-main) 18%, transparent);
  filter: saturate(1.04);
  outline: none;
}

.node-gantt-dialog__bar--pending {
  background: linear-gradient(135deg, color-mix(in srgb, var(--text-muted) 36%, transparent), color-mix(in srgb, var(--text-soft) 22%, transparent));
  color: var(--text-main);
}

.node-gantt-dialog__bar--active {
  background: linear-gradient(135deg, var(--accent-start), var(--accent-end));
  color: #f6fbff;
}

.node-gantt-dialog__bar--done {
  background: linear-gradient(135deg, color-mix(in srgb, var(--text-muted) 34%, transparent), color-mix(in srgb, var(--text-soft) 18%, transparent));
  color: color-mix(in srgb, var(--text-main) 70%, transparent);
}

.node-gantt-dialog__bar--delayed {
  background: linear-gradient(135deg, color-mix(in srgb, var(--accent-warning) 76%, #ffffff 10%), color-mix(in srgb, var(--accent-warning) 58%, transparent));
  color: #fffaf2;
}

.node-gantt-dialog__detail-card {
  position: fixed;
  z-index: 280;
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

.node-gantt-dialog__detail-card[data-placement='top-right'] {
  transform: translate(16px, calc(-100% - 14px));
  transform-origin: left bottom;
}

.node-gantt-dialog__detail-card[data-placement='top-left'] {
  transform: translate(calc(-100% - 16px), calc(-100% - 14px));
  transform-origin: right bottom;
}

.node-gantt-dialog__detail-card[data-placement='bottom-right'] {
  transform: translate(16px, 14px);
  transform-origin: left top;
}

.node-gantt-dialog__detail-card[data-placement='bottom-left'] {
  transform: translate(calc(-100% - 16px), 14px);
  transform-origin: right top;
}

.node-gantt-dialog__detail-topline {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
}

.node-gantt-dialog__detail-eyebrow {
  font-size: 0.66rem;
  letter-spacing: 0.16em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.node-gantt-dialog__detail-title,
.node-gantt-dialog__detail-status,
.node-gantt-dialog__detail-priority,
.node-gantt-dialog__detail-progress-head span,
.node-gantt-dialog__detail-progress-head strong,
.node-gantt-dialog__detail-grid dt,
.node-gantt-dialog__detail-grid dd {
  margin: 0;
}

.node-gantt-dialog__detail-title {
  font-size: 0.95rem;
  line-height: 1.32;
  color: var(--text-main);
}

.node-gantt-dialog__detail-status {
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

.node-gantt-dialog__detail-status--pending {
  border-color: color-mix(in srgb, var(--text-muted) 24%, transparent);
  background: color-mix(in srgb, var(--text-muted) 16%, transparent);
  color: var(--text-soft);
}

.node-gantt-dialog__detail-status--active {
  border-color: color-mix(in srgb, var(--accent-end) 72%, #0f2c5b 28%);
  background: linear-gradient(135deg, color-mix(in srgb, var(--accent-start) 82%, #0d2754 18%), color-mix(in srgb, var(--accent-end) 70%, #0f2c5b 30%));
  color: #f7fbff;
  box-shadow:
    inset 0 1px 0 color-mix(in srgb, #ffffff 22%, transparent),
    0 10px 18px color-mix(in srgb, var(--accent-end) 16%, transparent);
}

.node-gantt-dialog__detail-status--done {
  border-color: color-mix(in srgb, var(--accent-success) 28%, transparent);
  background: color-mix(in srgb, var(--accent-success) 16%, transparent);
  color: var(--accent-success);
}

.node-gantt-dialog__detail-status--delayed {
  border-color: color-mix(in srgb, var(--accent-warning) 30%, transparent);
  background: color-mix(in srgb, var(--accent-warning) 14%, transparent);
  color: var(--accent-warning);
}

.node-gantt-dialog__detail-status--unknown {
  border-color: color-mix(in srgb, var(--border-soft) 90%, transparent);
  background: color-mix(in srgb, var(--panel-bg) 82%, transparent);
  color: var(--text-soft);
}

.node-gantt-dialog__detail-meta {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.node-gantt-dialog__detail-priority {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-height: 30px;
  padding: 0 10px;
  border-radius: 999px;
  font-size: 0.75rem;
  font-weight: 700;
}

.node-gantt-dialog__detail-priority--low {
  background: color-mix(in srgb, var(--accent-success) 12%, transparent);
  color: var(--accent-success);
}

.node-gantt-dialog__detail-priority--medium {
  background: color-mix(in srgb, var(--accent-warning) 14%, transparent);
  color: var(--accent-warning);
}

.node-gantt-dialog__detail-priority--high {
  background: color-mix(in srgb, #ff6b7a 16%, transparent);
  color: #ff6b7a;
}

.node-gantt-dialog__detail-priority--unknown {
  background: color-mix(in srgb, var(--text-muted) 14%, transparent);
  color: var(--text-soft);
}

.node-gantt-dialog__detail-progress {
  display: grid;
  gap: 8px;
  padding: 12px;
  border: 1px solid color-mix(in srgb, var(--accent-line) 18%, transparent);
  border-radius: 16px;
  background: linear-gradient(180deg, color-mix(in srgb, var(--panel-bg) 92%, var(--app-bg)), color-mix(in srgb, var(--accent-end) 10%, var(--panel-bg)));
}

.node-gantt-dialog__detail-progress-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
}

.node-gantt-dialog__detail-progress-head span {
  font-size: 0.72rem;
  letter-spacing: 0.08em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.node-gantt-dialog__detail-progress-head strong {
  font-size: 1.04rem;
  line-height: 1;
  color: var(--text-main);
}

.node-gantt-dialog__detail-progress-track {
  position: relative;
  overflow: hidden;
  height: 8px;
  border-radius: 999px;
  background: color-mix(in srgb, var(--panel-bg) 80%, transparent);
  box-shadow: inset 0 0 0 1px color-mix(in srgb, var(--accent-line) 18%, transparent);
}

.node-gantt-dialog__detail-progress-fill {
  position: absolute;
  inset: 0 auto 0 0;
  border-radius: inherit;
  background: linear-gradient(90deg, var(--accent-start), var(--accent-end));
  box-shadow:
    0 0 0 1px color-mix(in srgb, #ffffff 18%, transparent),
    0 0 18px color-mix(in srgb, var(--accent-end) 22%, transparent);
}

.node-gantt-dialog__detail-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 8px;
}

.node-gantt-dialog__detail-grid-item {
  display: grid;
  gap: 4px;
  min-height: 68px;
  padding: 10px 11px;
  border: 1px solid color-mix(in srgb, var(--border-soft) 92%, transparent);
  border-radius: 14px;
  background: linear-gradient(180deg, color-mix(in srgb, var(--panel-bg) 92%, var(--app-bg)), color-mix(in srgb, var(--accent-end) 10%, var(--panel-bg)));
}

.node-gantt-dialog__detail-grid-item dt {
  color: var(--text-soft);
  font-size: 0.7rem;
  font-weight: 600;
}

.node-gantt-dialog__detail-grid-item dd {
  color: var(--text-main);
  font-size: 0.82rem;
  font-weight: 700;
  line-height: 1.45;
}

.node-gantt-dialog-enter-active,
.node-gantt-dialog-leave-active {
  transition:
    opacity 220ms ease-out,
    transform 220ms ease-out;
}

.node-gantt-dialog-enter-from,
.node-gantt-dialog-leave-to {
  opacity: 0;
  transform: scale(0.97);
}

@media (max-width: 960px) {
  .node-gantt-dialog {
    width: min(1240px, calc(100vw - 24px));
    max-height: min(80vh, calc(100vh - 24px));
    padding: 18px;
  }

  .node-gantt-dialog__body {
    grid-template-columns: 1fr;
  }

  .node-gantt-dialog__detail-grid {
    grid-template-columns: 1fr;
  }

  .node-gantt-dialog__detail-grid-item--priority {
    grid-column: auto;
  }
}
</style>
