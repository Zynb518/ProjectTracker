<script setup lang="ts">
import { computed, onBeforeUnmount, ref } from 'vue'

import GanttScaleSwitcher from '@/components/workspace/GanttScaleSwitcher.vue'
import type {
  GanttMemberSubtaskSummary,
  GanttPerspective,
  GanttScale,
  ProjectMemberGantt,
} from '@/types/gantt'
import { getWorkStatusLabel, getWorkStatusTone } from '@/utils/display'
import { buildGanttAxisItems, getGanttBarLayout, packGanttLaneItems } from '@/utils/gantt'

const props = withDefaults(defineProps<{
  error?: string | null
  gantt: ProjectMemberGantt | null
  isLoading?: boolean
  perspective?: GanttPerspective
  scale: GanttScale
}>(), {
  error: null,
  isLoading: false,
  perspective: 'member',
})

const emit = defineEmits<{
  retry: []
  'update:perspective': [perspective: GanttPerspective]
  'update:scale': [scale: GanttScale]
}>()

type HoverCardPlacement = 'bottom-left' | 'bottom-right' | 'top-left' | 'top-right'

const MEMBER_ROW_MIN_HEIGHT_PX = 62
const MEMBER_ROW_PADDING_PX = 12
const MEMBER_BAR_HEIGHT_PX = 36
const MEMBER_LANE_GAP_PX = 8
const DETAIL_HIDE_DELAY_MS = 180
const DETAIL_CARD_EDGE_GAP_PX = 24
const DETAIL_CARD_HEIGHT_HINT_PX = 256
const DETAIL_CARD_WIDTH_PX = 288
const hoveredSubtaskId = ref<number | null>(null)
const hoveredMemberUserId = ref<number | null>(null)
const hoveredSubtaskAnchor = ref({ x: 0, y: 0 })
const hoveredSubtaskPlacement = ref<HoverCardPlacement>('top-right')
let hideDetailTimer: ReturnType<typeof window.setTimeout> | null = null
const axisScrollRef = ref<HTMLElement | null>(null)
const rowsScrollRef = ref<HTMLElement | null>(null)

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

const hoveredSubtaskEntry = computed(() => {
  if (hoveredSubtaskId.value === null || hoveredMemberUserId.value === null) {
    return null
  }

  const memberRow = props.gantt?.member_rows.find((row) => row.user_id === hoveredMemberUserId.value) ?? null
  const subtask = memberRow?.subtasks.find((item) => item.subtask_id === hoveredSubtaskId.value) ?? null

  if (memberRow === null || subtask === null) {
    return null
  }

  return {
    memberRow,
    subtask,
  }
})

const detailCardStyle = computed(() => ({
  left: `${hoveredSubtaskAnchor.value.x}px`,
  top: `${hoveredSubtaskAnchor.value.y}px`,
}))

const memberRows = computed(() =>
  props.gantt?.member_rows.map((memberRow) => {
    const packedSubtasks = packGanttLaneItems(memberRow.subtasks)
    const laneCount = Math.max(
      1,
      packedSubtasks.reduce((maxLane, subtask) => Math.max(maxLane, subtask.laneIndex + 1), 0),
    )

    return {
      ...memberRow,
      laneCount,
      packedSubtasks,
      rowHeightPx: Math.max(
        MEMBER_ROW_MIN_HEIGHT_PX,
        MEMBER_ROW_PADDING_PX * 2 + laneCount * MEMBER_BAR_HEIGHT_PX + (laneCount - 1) * MEMBER_LANE_GAP_PX,
      ),
    }
  }) ?? [],
)

function getRowStyle(rowHeightPx: number) {
  return {
    height: `${rowHeightPx}px`,
    minHeight: `${rowHeightPx}px`,
  }
}

function getBarStyle(subtask: GanttMemberSubtaskSummary, laneIndex: number) {
  const layout = getGanttBarLayout(
    axisStartDate.value,
    subtask.planned_start_date,
    subtask.planned_end_date,
    props.scale,
  )

  return {
    left: `${layout.leftPx}px`,
    width: `${layout.widthPx}px`,
    top: `${MEMBER_ROW_PADDING_PX + laneIndex * (MEMBER_BAR_HEIGHT_PX + MEMBER_LANE_GAP_PX)}px`,
  }
}

function formatCompletedAt(value: string) {
  return value.slice(0, 16).replace('T', ' ')
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

function showSubtaskDetail(subtaskId: number, userId: number, event?: FocusEvent | MouseEvent) {
  cancelHideSubtaskDetail()
  hoveredSubtaskId.value = subtaskId
  hoveredMemberUserId.value = userId
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
    hoveredMemberUserId.value = null
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
  if (source === null || target === null || target.scrollLeft === source.scrollLeft) {
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
  <section class="project-member-gantt" data-testid="project-member-gantt-view">
    <header class="project-member-gantt__toolbar">
      <div class="project-member-gantt__toolbar-copy">
        <p class="project-member-gantt__eyebrow">Member Matrix</p>
        <div>
          <h3>人员甘特图</h3>
          <p>顶部按时间粒度缩放，左侧按成员查看，右侧展示各成员名下子任务的时间分布与重叠关系。</p>
        </div>
      </div>

      <div class="project-member-gantt__toolbar-actions">
        <div class="project-member-gantt__perspective-switch" role="tablist" aria-label="甘特图视角">
          <button
            :class="['project-member-gantt__perspective-button', { 'is-active': perspective === 'stage' }]"
            type="button"
            @click="emit('update:perspective', 'stage')"
          >
            阶段 / 时间
          </button>
          <button
            :class="['project-member-gantt__perspective-button', { 'is-active': perspective === 'member' }]"
            type="button"
            @click="emit('update:perspective', 'member')"
          >
            人员 / 时间
          </button>
        </div>

        <GanttScaleSwitcher :scale="scale" @update:scale="emit('update:scale', $event)" />
      </div>
    </header>

    <p v-if="isLoading" class="project-member-gantt__state loading-panel">人员甘特图加载中...</p>

    <section v-else-if="error" class="project-member-gantt__state project-member-gantt__state--error">
      <div>
        <strong>人员甘特图暂时不可用</strong>
        <p>{{ error }}</p>
      </div>

      <button type="button" @click="emit('retry')">重试</button>
    </section>

    <section
      v-else-if="gantt === null || memberRows.length === 0"
      class="project-member-gantt__state"
    >
      当前还没有可展示的成员排期。
    </section>

    <div
      v-else
      v-smooth-wheel="{ axis: 'vertical', wheelBehavior: 'native', multiplier: 1.3 }"
      class="project-member-gantt__body-scroll smooth-scroll-surface"
    >
      <div class="project-member-gantt__body">
        <aside class="project-member-gantt__sidebar">
          <div class="project-member-gantt__sidebar-head">
            <span>成员</span>
            <span>任务数</span>
          </div>

          <div
            v-for="memberRow in memberRows"
            :key="memberRow.user_id"
            :data-testid="`project-member-gantt-row-label-${memberRow.user_id}`"
            :style="getRowStyle(memberRow.rowHeightPx)"
            class="project-member-gantt__sidebar-row"
          >
            <strong>{{ memberRow.real_name }}</strong>
            <span class="project-member-gantt__member-count">{{ memberRow.subtasks.length }} 项</span>
          </div>
        </aside>

        <div class="project-member-gantt__timeline-shell">
          <div
            ref="axisScrollRef"
            data-testid="project-member-gantt-axis-scroll"
            v-smooth-wheel="{ axis: 'horizontal', wheelBehavior: 'block' }"
            class="project-member-gantt__axis-scroll smooth-scroll-surface"
            @scroll="handleAxisScroll"
          >
            <div class="project-member-gantt__timeline-canvas" :style="timelineCanvasStyle">
              <div class="project-member-gantt__axis" data-testid="project-member-gantt-axis">
                <div
                  v-for="item in axisItems"
                  :key="item.key"
                  :class="['project-member-gantt__axis-cell', `project-member-gantt__axis-cell--${scale}`]"
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
            data-testid="project-member-gantt-rows-scroll"
            v-smooth-wheel="{ axis: 'horizontal', wheelBehavior: 'block' }"
            class="project-member-gantt__timeline-scroll smooth-scroll-surface"
            @scroll="handleRowsScroll"
          >
            <div class="project-member-gantt__timeline-canvas" :style="timelineCanvasStyle">
              <div class="project-member-gantt__rows">
                <div
                  v-for="memberRow in memberRows"
                  :key="memberRow.user_id"
                  :data-testid="`project-member-gantt-row-${memberRow.user_id}`"
                  :style="getRowStyle(memberRow.rowHeightPx)"
                  class="project-member-gantt__row"
                >
                  <div class="project-member-gantt__track">
                    <div
                      :data-testid="`project-member-gantt-track-grid-${memberRow.user_id}`"
                      aria-hidden="true"
                      class="project-member-gantt__track-grid"
                    >
                      <div
                        v-for="item in axisItems"
                        :key="`${memberRow.user_id}-${item.key}`"
                        :class="['project-member-gantt__track-cell', `project-member-gantt__track-cell--${scale}`]"
                        :style="{ width: `${item.widthPx}px` }"
                      />
                    </div>

                    <template v-if="memberRow.packedSubtasks.length > 0">
                      <button
                        v-for="subtask in memberRow.packedSubtasks"
                        :key="subtask.item.subtask_id"
                        :data-testid="`project-member-gantt-bar-${subtask.item.subtask_id}`"
                        :class="[
                          'project-member-gantt__bar',
                          `project-member-gantt__bar--${getWorkStatusTone(subtask.item.status)}`,
                        ]"
                        :style="getBarStyle(subtask.item, subtask.laneIndex)"
                        :title="`${subtask.item.name}｜${subtask.item.node_name}｜${subtask.item.planned_start_date} - ${subtask.item.planned_end_date}`"
                        type="button"
                        @blur="clearSubtaskDetail(subtask.item.subtask_id)"
                        @focus="showSubtaskDetail(subtask.item.subtask_id, memberRow.user_id, $event)"
                        @mouseleave="clearSubtaskDetail(subtask.item.subtask_id)"
                        @mouseenter="showSubtaskDetail(subtask.item.subtask_id, memberRow.user_id, $event)"
                      >
                        <span>{{ subtask.item.name }}</span>
                      </button>
                    </template>

                    <div v-else class="project-member-gantt__empty-track">
                      当前成员暂无排期任务
                    </div>
                  </div>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>

      <Teleport to="body">
        <aside
          v-if="hoveredSubtaskEntry"
          :data-placement="hoveredSubtaskPlacement"
          :style="detailCardStyle"
          class="project-member-gantt__detail-card"
          data-testid="project-member-gantt-detail-card"
          @mouseenter="cancelHideSubtaskDetail"
          @mouseleave="clearSubtaskDetail()"
        >
          <div class="project-member-gantt__detail-topline">
            <span class="project-member-gantt__detail-eyebrow">子任务详情</span>
            <span
              :class="[
                'project-member-gantt__detail-status',
                `project-member-gantt__detail-status--${getWorkStatusTone(hoveredSubtaskEntry.subtask.status)}`,
              ]"
            >
              {{ getWorkStatusLabel(hoveredSubtaskEntry.subtask.status) }}
            </span>
          </div>

          <strong class="project-member-gantt__detail-title">{{ hoveredSubtaskEntry.subtask.name }}</strong>

          <div class="project-member-gantt__detail-meta">
            <span class="project-member-gantt__detail-chip">
              所属阶段 · {{ hoveredSubtaskEntry.subtask.node_name }}
            </span>
          </div>

          <section class="project-member-gantt__detail-progress" aria-label="子任务进度">
            <div class="project-member-gantt__detail-progress-head">
              <span>完成进度</span>
              <strong>{{ hoveredSubtaskEntry.subtask.progress_percent }}%</strong>
            </div>
            <div class="project-member-gantt__detail-progress-track">
              <span
                class="project-member-gantt__detail-progress-fill"
                data-testid="project-member-gantt-detail-progress-fill"
                :style="{ width: `${hoveredSubtaskEntry.subtask.progress_percent}%` }"
              />
            </div>
          </section>

          <dl class="project-member-gantt__detail-grid">
            <div class="project-member-gantt__detail-grid-item">
              <dt>负责人</dt>
              <dd>{{ hoveredSubtaskEntry.memberRow.real_name }}</dd>
            </div>
            <div class="project-member-gantt__detail-grid-item">
              <dt>计划周期</dt>
              <dd>{{ hoveredSubtaskEntry.subtask.planned_start_date }}<br>{{ hoveredSubtaskEntry.subtask.planned_end_date }}</dd>
            </div>
            <div
              :class="[
                'project-member-gantt__detail-grid-item',
                'project-member-gantt__detail-grid-item--completion',
                {
                  'is-empty': !hoveredSubtaskEntry.subtask.completed_at,
                },
              ]"
            >
              <dt>完成时间</dt>
              <dd>{{ hoveredSubtaskEntry.subtask.completed_at ? formatCompletedAt(hoveredSubtaskEntry.subtask.completed_at) : '尚未完成' }}</dd>
            </div>
          </dl>
        </aside>
      </Teleport>
    </div>
  </section>
</template>

<style scoped>
.project-member-gantt {
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

.project-member-gantt::before {
  content: '';
  position: absolute;
  inset: 0;
  background:
    linear-gradient(var(--grid-line) 1px, transparent 1px),
    linear-gradient(90deg, var(--grid-line) 1px, transparent 1px);
  background-size: 24px 24px;
  opacity: 0.18;
  pointer-events: none;
}

.project-member-gantt__toolbar,
.project-member-gantt__state,
.project-member-gantt__body-scroll {
  position: relative;
  z-index: 1;
}

.project-member-gantt__toolbar {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 18px;
}

.project-member-gantt__toolbar-copy {
  display: grid;
  gap: 10px;
}

.project-member-gantt__eyebrow,
.project-member-gantt__toolbar-copy h3,
.project-member-gantt__toolbar-copy p {
  margin: 0;
}

.project-member-gantt__eyebrow {
  font-size: 0.76rem;
  letter-spacing: 0.18em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.project-member-gantt__toolbar-copy h3 {
  font-size: 1.46rem;
  color: color-mix(in srgb, var(--text-main) 86%, #ffffff 14%);
}

.project-member-gantt__toolbar-copy p {
  color: var(--text-soft);
  line-height: 1.6;
}

.project-member-gantt__toolbar-actions {
  display: grid;
  justify-items: end;
  gap: 12px;
}

.project-member-gantt__perspective-switch {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  padding: 6px;
  border: 1px solid var(--dialog-control-border);
  border-radius: 999px;
  background: var(--dialog-control-bg), var(--card-sheen);
  box-shadow: var(--dialog-control-shadow);
}

.project-member-gantt__perspective-button {
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

.project-member-gantt__perspective-button.is-active {
  background: linear-gradient(135deg, color-mix(in srgb, var(--accent-start) 80%, #0d2754 20%), color-mix(in srgb, var(--accent-end) 68%, #0f2c5b 32%));
  color: #f7fbff;
  box-shadow:
    inset 0 1px 0 color-mix(in srgb, #ffffff 18%, transparent),
    0 10px 18px color-mix(in srgb, var(--accent-end) 16%, transparent);
}

.project-member-gantt__state {
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

.project-member-gantt__state--error {
  align-items: flex-start;
}

.project-member-gantt__state--error strong,
.project-member-gantt__state--error p {
  margin: 0;
}

.project-member-gantt__state button {
  padding: 10px 16px;
  border: 0;
  border-radius: 12px;
  background: linear-gradient(135deg, var(--accent-start), var(--accent-end));
  color: #f8fbff;
  font-weight: 600;
  cursor: pointer;
}

.project-member-gantt__body-scroll {
  max-height: 620px;
  overflow: auto;
  padding-right: 4px;
}

.project-member-gantt__body {
  display: grid;
  grid-template-columns: minmax(220px, 260px) minmax(0, 1fr);
  gap: 18px;
  align-items: start;
  min-width: 0;
}

.project-member-gantt__sidebar,
.project-member-gantt__timeline-shell {
  min-width: 0;
}

.project-member-gantt__sidebar {
  display: grid;
  gap: 10px;
}

.project-member-gantt__sidebar-head,
.project-member-gantt__sidebar-row {
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  gap: 12px;
  align-items: center;
  padding: 14px 16px;
  border: 1px solid var(--dialog-control-border);
  border-radius: 18px;
  background: var(--dialog-control-bg), var(--meta-surface-glow), var(--card-sheen);
  box-shadow: var(--dialog-control-shadow);
}

.project-member-gantt__sidebar-head {
  position: sticky;
  top: 0;
  z-index: 3;
  min-height: 56px;
  font-size: 0.78rem;
  letter-spacing: 0.12em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.project-member-gantt__sidebar-row {
  overflow: hidden;
  content-visibility: auto;
  contain-intrinsic-size: 62px;
}

.project-member-gantt__sidebar-row strong {
  min-width: 0;
  overflow: hidden;
  font-size: 1rem;
  text-overflow: ellipsis;
  white-space: nowrap;
  color: var(--text-main);
}

.project-member-gantt__member-count {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 58px;
  padding: 8px 10px;
  border-radius: 999px;
  background: color-mix(in srgb, var(--accent-end) 12%, transparent);
  color: var(--text-main);
  font-size: 0.76rem;
  font-weight: 700;
}

.project-member-gantt__timeline-shell {
  display: grid;
  gap: 10px;
}

.project-member-gantt__axis-scroll {
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

.project-member-gantt__axis-scroll::-webkit-scrollbar {
  height: 12px;
}

.project-member-gantt__axis-scroll::-webkit-scrollbar-track {
  border-radius: 999px;
  background: color-mix(in srgb, var(--panel-bg) 84%, transparent);
}

.project-member-gantt__axis-scroll::-webkit-scrollbar-thumb {
  border: 2px solid transparent;
  border-radius: 999px;
  background: linear-gradient(90deg, color-mix(in srgb, var(--accent-start) 72%, #0d2754 28%), color-mix(in srgb, var(--accent-end) 72%, #0f2c5b 28%));
  background-clip: padding-box;
}

.project-member-gantt__timeline-scroll {
  overflow-x: hidden;
  overflow-y: hidden;
  padding-bottom: 0;
}

.project-member-gantt__timeline-canvas {
  min-width: 100%;
  box-sizing: content-box;
}

.project-member-gantt__axis {
  display: flex;
  min-height: 56px;
  background: var(--dialog-control-bg), var(--card-sheen);
}

.project-member-gantt__axis-cell {
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
  background: linear-gradient(
    180deg,
    color-mix(in srgb, var(--accent-end) 10%, var(--dialog-control-bg)),
    var(--dialog-control-bg)
  );
  box-shadow: inset 0 1px 0 color-mix(in srgb, #ffffff 14%, transparent);
}

.project-member-gantt__axis-cell:nth-child(even) {
  background: linear-gradient(
    180deg,
    color-mix(in srgb, var(--accent-start) 8%, var(--dialog-control-bg)),
    var(--dialog-control-bg)
  );
}

.project-member-gantt__axis-cell--day {
  justify-content: center;
  padding: 0 6px;
  font-size: 0.72rem;
  font-weight: 600;
  letter-spacing: 0.01em;
  font-variant-numeric: tabular-nums;
}

.project-member-gantt__axis-cell--week,
.project-member-gantt__axis-cell--month {
  font-variant-numeric: tabular-nums;
}

.project-member-gantt__axis-cell:first-child {
  border-left: 1px solid color-mix(in srgb, var(--accent-line) 20%, transparent);
  border-top-left-radius: 16px;
  border-bottom-left-radius: 16px;
}

.project-member-gantt__axis-cell:last-child {
  border-top-right-radius: 16px;
  border-bottom-right-radius: 16px;
}

.project-member-gantt__rows {
  display: grid;
  gap: 10px;
}

.project-member-gantt__row {
  min-height: 62px;
  contain: layout paint;
  content-visibility: auto;
  contain-intrinsic-size: 62px;
}

.project-member-gantt__track {
  position: relative;
  height: 100%;
  min-height: 62px;
  overflow: hidden;
  border: 1px solid color-mix(in srgb, var(--accent-line) 18%, transparent);
  border-radius: 18px;
  background: linear-gradient(
    180deg,
    color-mix(in srgb, var(--accent-end) 6%, var(--dialog-control-bg)),
    color-mix(in srgb, var(--accent-start) 2%, var(--panel-bg))
  );
}

.project-member-gantt__track-grid {
  position: absolute;
  inset: 0;
  display: flex;
  pointer-events: none;
}

.project-member-gantt__track-cell {
  flex: 0 0 auto;
  height: 100%;
  border-right: 1px solid color-mix(in srgb, var(--accent-line) 14%, transparent);
  background: linear-gradient(
    180deg,
    color-mix(in srgb, var(--accent-end) 8%, var(--dialog-control-bg)),
    var(--dialog-control-bg)
  );
}

.project-member-gantt__track-cell:nth-child(even) {
  background: linear-gradient(
    180deg,
    color-mix(in srgb, var(--accent-start) 6%, var(--dialog-control-bg)),
    var(--dialog-control-bg)
  );
}

.project-member-gantt__track-cell--day {
  background: linear-gradient(
    180deg,
    color-mix(in srgb, var(--accent-end) 9%, var(--dialog-control-bg)),
    var(--dialog-control-bg)
  );
}

.project-member-gantt__bar {
  position: absolute;
  display: inline-flex;
  align-items: center;
  min-width: 0;
  height: 36px;
  padding: 0 12px;
  border: 0;
  border-radius: 14px;
  cursor: pointer;
  appearance: none;
  text-align: left;
  box-shadow:
    inset 0 1px 0 color-mix(in srgb, #ffffff 14%, transparent),
    0 12px 20px color-mix(in srgb, var(--text-main) 12%, transparent);
}

.project-member-gantt__bar span {
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  font-size: 0.82rem;
  font-weight: 700;
}

.project-member-gantt__bar--pending {
  background: var(--work-status-pending-strong);
  color: var(--work-status-pending-contrast);
}

.project-member-gantt__bar--active {
  background: var(--work-status-active-strong);
  color: var(--work-status-active-contrast);
}

.project-member-gantt__bar--done {
  background: var(--work-status-done-strong);
  color: var(--work-status-done-contrast);
}

.project-member-gantt__bar--delayed {
  background: var(--work-status-delayed-strong);
  color: var(--work-status-delayed-contrast);
}

.project-member-gantt__detail-card {
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

.project-member-gantt__detail-card[data-placement='top-right'] {
  transform: translate(16px, calc(-100% - 14px));
  transform-origin: left bottom;
}

.project-member-gantt__detail-card[data-placement='top-left'] {
  transform: translate(calc(-100% - 16px), calc(-100% - 14px));
  transform-origin: right bottom;
}

.project-member-gantt__detail-card[data-placement='bottom-right'] {
  transform: translate(16px, 14px);
  transform-origin: left top;
}

.project-member-gantt__detail-card[data-placement='bottom-left'] {
  transform: translate(calc(-100% - 16px), 14px);
  transform-origin: right top;
}

.project-member-gantt__detail-topline {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
}

.project-member-gantt__detail-eyebrow {
  font-size: 0.66rem;
  letter-spacing: 0.16em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.project-member-gantt__detail-title,
.project-member-gantt__detail-status,
.project-member-gantt__detail-chip,
.project-member-gantt__detail-progress-head span,
.project-member-gantt__detail-progress-head strong,
.project-member-gantt__detail-grid dt,
.project-member-gantt__detail-grid dd {
  margin: 0;
}

.project-member-gantt__detail-title {
  font-size: 0.95rem;
  line-height: 1.32;
  color: var(--text-main);
}

.project-member-gantt__detail-status {
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

.project-member-gantt__detail-status--pending {
  border-color: var(--work-status-pending-border);
  background: var(--work-status-pending-bg);
  color: var(--work-status-pending-color);
}

.project-member-gantt__detail-status--active {
  border-color: var(--work-status-active-border);
  background: var(--work-status-active-bg);
  color: var(--work-status-active-color);
}

.project-member-gantt__detail-status--done {
  border-color: var(--work-status-done-border);
  background: var(--work-status-done-bg);
  color: var(--work-status-done-color);
}

.project-member-gantt__detail-status--delayed {
  border-color: var(--work-status-delayed-border);
  background: var(--work-status-delayed-bg);
  color: var(--work-status-delayed-color);
}

.project-member-gantt__detail-status--unknown {
  border-color: var(--work-status-unknown-border);
  background: var(--work-status-unknown-bg);
  color: var(--work-status-unknown-color);
}

.project-member-gantt__detail-meta {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.project-member-gantt__detail-chip {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-height: 30px;
  padding: 0 10px;
  border-radius: 999px;
  background: color-mix(in srgb, var(--accent-end) 12%, transparent);
  color: var(--text-main);
  font-size: 0.75rem;
  font-weight: 700;
}

.project-member-gantt__detail-progress {
  display: grid;
  gap: 8px;
  padding: 12px;
  border: 1px solid color-mix(in srgb, var(--accent-line) 18%, transparent);
  border-radius: 16px;
  background: linear-gradient(180deg, color-mix(in srgb, var(--panel-bg) 92%, var(--app-bg)), color-mix(in srgb, var(--accent-end) 10%, var(--panel-bg)));
}

.project-member-gantt__detail-progress-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
}

.project-member-gantt__detail-progress-head span {
  font-size: 0.72rem;
  letter-spacing: 0.08em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.project-member-gantt__detail-progress-head strong {
  font-size: 1.04rem;
  line-height: 1;
  color: var(--text-main);
}

.project-member-gantt__detail-progress-track {
  position: relative;
  overflow: hidden;
  height: 8px;
  border-radius: 999px;
  background: color-mix(in srgb, var(--panel-bg) 80%, transparent);
  box-shadow: inset 0 0 0 1px color-mix(in srgb, var(--accent-line) 18%, transparent);
}

.project-member-gantt__detail-progress-fill {
  position: absolute;
  inset: 0 auto 0 0;
  border-radius: inherit;
  background: linear-gradient(90deg, var(--accent-start), var(--accent-end));
  box-shadow:
    0 0 0 1px color-mix(in srgb, #ffffff 18%, transparent),
    0 0 18px color-mix(in srgb, var(--accent-end) 22%, transparent);
}

.project-member-gantt__detail-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 8px;
}

.project-member-gantt__detail-grid-item {
  display: grid;
  gap: 4px;
  min-height: 68px;
  padding: 10px 11px;
  border: 1px solid color-mix(in srgb, var(--border-soft) 92%, transparent);
  border-radius: 14px;
  background: linear-gradient(180deg, color-mix(in srgb, var(--panel-bg) 92%, var(--app-bg)), color-mix(in srgb, var(--accent-end) 10%, var(--panel-bg)));
}

.project-member-gantt__detail-grid-item--completion {
  grid-column: span 2;
}

.project-member-gantt__detail-grid-item dt {
  color: var(--text-soft);
  font-size: 0.7rem;
  font-weight: 600;
}

.project-member-gantt__detail-grid-item dd {
  color: var(--text-main);
  font-size: 0.82rem;
  font-weight: 700;
  line-height: 1.45;
}

.project-member-gantt__detail-grid-item.is-empty dd {
  color: var(--text-soft);
}

.project-member-gantt__empty-track {
  position: absolute;
  inset: 50% auto auto 18px;
  transform: translateY(-50%);
  color: var(--text-muted);
  font-size: 0.82rem;
  font-weight: 600;
}

@media (max-width: 1100px) {
  .project-member-gantt__body {
    grid-template-columns: 1fr;
  }

  .project-member-gantt__toolbar {
    flex-direction: column;
    align-items: stretch;
  }

  .project-member-gantt__toolbar-actions {
    width: 100%;
    justify-items: stretch;
  }

  .project-member-gantt__perspective-switch {
    width: 100%;
    justify-content: space-between;
  }

  .project-member-gantt__detail-grid {
    grid-template-columns: 1fr;
  }

  .project-member-gantt__detail-grid-item--completion {
    grid-column: auto;
  }
}
</style>
