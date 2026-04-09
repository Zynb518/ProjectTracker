<script setup lang="ts">
import { computed, ref } from 'vue'

import GanttScaleSwitcher from '@/components/workspace/GanttScaleSwitcher.vue'
import type {
  GanttMemberSubtaskSummary,
  GanttPerspective,
  GanttScale,
  ProjectMemberGantt,
} from '@/types/gantt'
import { getWorkStatusTone } from '@/utils/display'
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

const MEMBER_ROW_MIN_HEIGHT_PX = 62
const MEMBER_ROW_PADDING_PX = 12
const MEMBER_BAR_HEIGHT_PX = 36
const MEMBER_LANE_GAP_PX = 8
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
      v-smooth-wheel
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
                      <div
                        v-for="subtask in memberRow.packedSubtasks"
                        :key="subtask.item.subtask_id"
                        :data-testid="`project-member-gantt-bar-${subtask.item.subtask_id}`"
                        :class="[
                          'project-member-gantt__bar',
                          `project-member-gantt__bar--${getWorkStatusTone(subtask.item.status)}`,
                        ]"
                        :style="getBarStyle(subtask.item, subtask.laneIndex)"
                        :title="`${subtask.item.name}｜${subtask.item.node_name}｜${subtask.item.planned_start_date} - ${subtask.item.planned_end_date}`"
                      >
                        <span>{{ subtask.item.name }}</span>
                      </div>
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
  border: 1px solid color-mix(in srgb, var(--accent-line) 34%, var(--border-soft));
  border-radius: 28px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--glass-bg) 96%, transparent), color-mix(in srgb, var(--glass-bg) 88%, transparent)),
    radial-gradient(circle at 100% 0%, color-mix(in srgb, var(--accent-end) 16%, transparent), transparent 32%);
  box-shadow: var(--shadow-panel-hover);
  backdrop-filter: var(--backdrop-blur);
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
  border: 1px solid color-mix(in srgb, var(--accent-line) 22%, var(--border-soft));
  border-radius: 999px;
  background: var(--dialog-control-bg);
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
  background: var(--meta-surface-bg), var(--meta-surface-glow);
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
  background: var(--dialog-control-bg), var(--meta-surface-glow);
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
  background: color-mix(in srgb, var(--glass-bg) 96%, transparent);
  backdrop-filter: blur(12px);
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
  background: linear-gradient(180deg, color-mix(in srgb, var(--accent-end) 16%, transparent), color-mix(in srgb, var(--glass-bg) 92%, transparent));
  box-shadow: inset 0 1px 0 color-mix(in srgb, #ffffff 14%, transparent);
}

.project-member-gantt__axis-cell:nth-child(even) {
  background: linear-gradient(180deg, color-mix(in srgb, var(--accent-start) 14%, transparent), color-mix(in srgb, var(--glass-bg) 90%, transparent));
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
}

.project-member-gantt__track {
  position: relative;
  height: 100%;
  min-height: 62px;
  overflow: hidden;
  border: 1px solid color-mix(in srgb, var(--accent-line) 18%, transparent);
  border-radius: 18px;
  background: linear-gradient(180deg, color-mix(in srgb, var(--glass-bg) 74%, transparent), transparent);
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
  background: linear-gradient(180deg, color-mix(in srgb, var(--accent-end) 10%, transparent), color-mix(in srgb, var(--glass-bg) 92%, transparent));
}

.project-member-gantt__track-cell:nth-child(even) {
  background: linear-gradient(180deg, color-mix(in srgb, var(--accent-start) 8%, transparent), color-mix(in srgb, var(--glass-bg) 90%, transparent));
}

.project-member-gantt__track-cell--day {
  background: linear-gradient(180deg, color-mix(in srgb, var(--accent-end) 12%, transparent), color-mix(in srgb, var(--glass-bg) 90%, transparent));
}

.project-member-gantt__bar {
  position: absolute;
  display: inline-flex;
  align-items: center;
  height: 36px;
  padding: 0 12px;
  border-radius: 14px;
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
  background: linear-gradient(135deg, color-mix(in srgb, var(--text-muted) 36%, transparent), color-mix(in srgb, var(--text-soft) 22%, transparent));
  color: var(--text-main);
}

.project-member-gantt__bar--active {
  background: linear-gradient(135deg, var(--accent-start), var(--accent-end));
  color: #f6fbff;
}

.project-member-gantt__bar--done {
  background: linear-gradient(135deg, color-mix(in srgb, var(--text-muted) 34%, transparent), color-mix(in srgb, var(--text-soft) 18%, transparent));
  color: color-mix(in srgb, var(--text-main) 70%, transparent);
}

.project-member-gantt__bar--delayed {
  background: linear-gradient(135deg, color-mix(in srgb, var(--accent-warning) 76%, #ffffff 10%), color-mix(in srgb, var(--accent-warning) 58%, transparent));
  color: #fffaf2;
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
}
</style>
