<script setup lang="ts">
import { computed, ref } from 'vue'

import GanttScaleSwitcher from '@/components/workspace/GanttScaleSwitcher.vue'
import type { GanttScale } from '@/types/gantt'
import type { ProjectListItem } from '@/types/project'
import { getWorkStatusLabel, getWorkStatusTone } from '@/utils/display'
import { buildGanttAxisItems, getGanttBarLayout } from '@/utils/gantt'

type DialogMotionOrigin = {
  x: number
  y: number
  translateX: number
  translateY: number
}

const props = withDefaults(defineProps<{
  error?: string | null
  filtersSummary: string
  isLoading?: boolean
  modelValue: boolean
  motionOrigin?: DialogMotionOrigin | null
  projectCount: number
  projects: ProjectListItem[]
  scale: GanttScale
}>(), {
  error: null,
  isLoading: false,
})

const emit = defineEmits<{
  retry: []
  'open-project': [project: ProjectListItem]
  'update:modelValue': [value: boolean]
  'update:scale': [scale: GanttScale]
}>()

const axisScrollRef = ref<HTMLElement | null>(null)
const sidebarScrollRef = ref<HTMLElement | null>(null)
const rowsScrollRef = ref<HTMLElement | null>(null)

const timelineRange = computed(() => {
  if (props.projects.length === 0) {
    return null
  }

  const dates = props.projects
    .flatMap((project) => [project.planned_start_date, project.planned_end_date])
    .sort()

  return {
    startDate: dates[0],
    endDate: dates[dates.length - 1],
  }
})

const axisItems = computed(() => {
  if (timelineRange.value === null) {
    return []
  }

  return buildGanttAxisItems(timelineRange.value.startDate, timelineRange.value.endDate, props.scale)
})

const axisStartDate = computed(() => axisItems.value[0]?.startDate ?? timelineRange.value?.startDate ?? '')
const canvasWidth = computed(() => axisItems.value.reduce((total, item) => total + item.widthPx, 0))
const timelineCanvasStyle = computed(() => ({
  width: `${canvasWidth.value}px`,
}))
const backdropStyle = computed(() => {
  if (!props.motionOrigin) {
    return undefined
  }

  return {
    '--project-list-gantt-origin-x': `${props.motionOrigin.x}px`,
    '--project-list-gantt-origin-y': `${props.motionOrigin.y}px`,
    '--project-list-gantt-translate-x': `${props.motionOrigin.translateX}px`,
    '--project-list-gantt-translate-y': `${props.motionOrigin.translateY}px`,
  }
})

function closeDialog() {
  emit('update:modelValue', false)
}

function openProject(project: ProjectListItem) {
  emit('open-project', project)
}

function getBarStyle(project: ProjectListItem) {
  const layout = getGanttBarLayout(
    axisStartDate.value,
    project.planned_start_date,
    project.planned_end_date,
    props.scale,
  )

  return {
    left: `${layout.leftPx}px`,
    width: `${layout.widthPx}px`,
  }
}

function syncHorizontalScroll(source: HTMLElement | null, target: HTMLElement | null) {
  if (source === null || target === null || source.scrollLeft === target.scrollLeft) {
    return
  }

  target.scrollLeft = source.scrollLeft
}

function syncVerticalScroll(source: HTMLElement | null, target: HTMLElement | null) {
  if (source === null || target === null || source.scrollTop === target.scrollTop) {
    return
  }

  target.scrollTop = source.scrollTop
}

function handleAxisScroll() {
  syncHorizontalScroll(axisScrollRef.value, rowsScrollRef.value)
}

function handleRowsScroll() {
  syncHorizontalScroll(rowsScrollRef.value, axisScrollRef.value)
  syncVerticalScroll(rowsScrollRef.value, sidebarScrollRef.value)
}

function handleSidebarScroll() {
  syncVerticalScroll(sidebarScrollRef.value, rowsScrollRef.value)
}
</script>

<template>
  <Transition appear name="project-list-gantt-dialog">
    <div
      v-if="modelValue"
      :style="backdropStyle"
      class="project-list-gantt-dialog__backdrop"
      data-testid="project-list-gantt-dialog-backdrop"
      @click.self="closeDialog"
    >
      <div aria-hidden="true" class="project-list-gantt-dialog__glow" />

      <section class="project-list-gantt-dialog" data-testid="project-list-gantt-dialog">
        <header class="project-list-gantt-dialog__header">
          <div class="project-list-gantt-dialog__copy">
            <p class="project-list-gantt-dialog__eyebrow">Portfolio Timeline</p>
            <div>
              <h3>项目时间总览</h3>
              <p>{{ filtersSummary }} · 共 {{ projectCount }} 个项目</p>
            </div>
          </div>

          <div class="project-list-gantt-dialog__toolbar">
            <GanttScaleSwitcher :scale="scale" @update:scale="emit('update:scale', $event)" />

            <button
              aria-label="关闭项目甘特图"
              class="project-list-gantt-dialog__close"
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
          </div>
        </header>

        <p v-if="isLoading" class="project-list-gantt-dialog__state loading-panel">项目甘特图加载中...</p>

        <section
          v-else-if="error"
          class="project-list-gantt-dialog__state project-list-gantt-dialog__state--error"
        >
          <div>
            <strong>项目甘特图暂时不可用</strong>
            <p>{{ error }}</p>
          </div>

          <button type="button" @click="emit('retry')">重试</button>
        </section>

        <section
          v-else-if="projects.length === 0"
          class="project-list-gantt-dialog__state"
        >
          当前筛选条件下没有可展示的项目排期。
        </section>

        <div
          v-else
          v-smooth-wheel="{ axis: 'vertical', wheelBehavior: 'native', multiplier: 1.3 }"
          class="project-list-gantt-dialog__body-scroll smooth-scroll-surface"
        >
          <div class="project-list-gantt-dialog__body">
            <aside class="project-list-gantt-dialog__sidebar">
              <div class="project-list-gantt-dialog__sidebar-head">
                <span>项目</span>
                <span>摘要</span>
              </div>

              <div
                ref="sidebarScrollRef"
                data-testid="project-list-gantt-sidebar-scroll"
                class="project-list-gantt-dialog__sidebar-scroll smooth-scroll-surface"
                @scroll="handleSidebarScroll"
              >
                <div
                  v-for="project in projects"
                  :key="project.id"
                  :data-testid="`project-list-gantt-row-${project.id}`"
                  class="project-list-gantt-dialog__sidebar-row"
                >
                  <strong>{{ project.name }}</strong>
                  <div class="project-list-gantt-dialog__sidebar-meta">
                    <span
                      :class="[
                        'project-list-gantt-dialog__status-pill',
                        `project-list-gantt-dialog__status-pill--${getWorkStatusTone(project.status)}`,
                      ]"
                    >
                      {{ getWorkStatusLabel(project.status) }}
                    </span>
                    <span>{{ project.owner_real_name }}</span>
                  </div>
                </div>
              </div>
            </aside>

            <div class="project-list-gantt-dialog__timeline-shell">
              <div
                ref="axisScrollRef"
                data-testid="project-list-gantt-axis-scroll"
                v-smooth-wheel="{ axis: 'horizontal', wheelBehavior: 'block' }"
                class="project-list-gantt-dialog__axis-scroll smooth-scroll-surface"
                @scroll="handleAxisScroll"
              >
                <div class="project-list-gantt-dialog__timeline-canvas" :style="timelineCanvasStyle">
                  <div class="project-list-gantt-dialog__axis" data-testid="project-list-gantt-axis">
                    <div
                      v-for="item in axisItems"
                      :key="item.key"
                      :class="[
                        'project-list-gantt-dialog__axis-cell',
                        `project-list-gantt-dialog__axis-cell--${scale}`,
                      ]"
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
                data-testid="project-list-gantt-rows-scroll"
                v-smooth-wheel="{ axis: 'horizontal', wheelBehavior: 'block' }"
                class="project-list-gantt-dialog__timeline-scroll smooth-scroll-surface"
                @scroll="handleRowsScroll"
              >
                <div class="project-list-gantt-dialog__timeline-canvas" :style="timelineCanvasStyle">
                  <div class="project-list-gantt-dialog__rows">
                    <div
                      v-for="project in projects"
                      :key="project.id"
                      class="project-list-gantt-dialog__row"
                    >
                      <div class="project-list-gantt-dialog__track">
                        <div class="project-list-gantt-dialog__track-grid">
                          <div
                            v-for="item in axisItems"
                            :key="item.key"
                            :class="[
                              'project-list-gantt-dialog__track-cell',
                              `project-list-gantt-dialog__track-cell--${scale}`,
                            ]"
                            :style="{ width: `${item.widthPx}px` }"
                          />
                        </div>

                        <button
                          :aria-label="`打开项目 ${project.name} 的详情页`"
                          :class="[
                            'project-list-gantt-dialog__bar',
                            `project-list-gantt-dialog__bar--${getWorkStatusTone(project.status)}`,
                          ]"
                          :style="getBarStyle(project)"
                          type="button"
                          @click="openProject(project)"
                        >
                          <span>{{ project.name }}</span>
                        </button>
                      </div>
                    </div>
                  </div>
                </div>
              </div>
            </div>
          </div>
        </div>
      </section>
    </div>
  </Transition>
</template>

<style scoped>
.project-list-gantt-dialog {
  --project-list-gantt-axis-height: 58px;
  --project-list-gantt-row-height: 72px;
}

.project-list-gantt-dialog-enter-active,
.project-list-gantt-dialog-leave-active {
  transition:
    opacity 420ms ease-out,
    backdrop-filter 420ms ease-out,
    background-color 420ms ease-out;
}

.project-list-gantt-dialog-enter-active .project-list-gantt-dialog,
.project-list-gantt-dialog-leave-active .project-list-gantt-dialog {
  transition:
    opacity 420ms cubic-bezier(0.16, 1, 0.3, 1),
    transform 480ms cubic-bezier(0.16, 1, 0.3, 1),
    box-shadow 460ms ease-out,
    filter 420ms ease-out;
}

.project-list-gantt-dialog-enter-active .project-list-gantt-dialog__glow,
.project-list-gantt-dialog-leave-active .project-list-gantt-dialog__glow {
  transition:
    opacity 520ms ease-out,
    transform 580ms cubic-bezier(0.16, 1, 0.3, 1),
    filter 520ms ease-out;
}

.project-list-gantt-dialog-enter-from,
.project-list-gantt-dialog-leave-to {
  opacity: 0;
  backdrop-filter: blur(0px);
}

.project-list-gantt-dialog-enter-from .project-list-gantt-dialog__glow,
.project-list-gantt-dialog-leave-to .project-list-gantt-dialog__glow {
  opacity: 0;
  transform: translate(-50%, -50%) scale(0.58);
  filter: blur(40px);
}

.project-list-gantt-dialog-enter-from .project-list-gantt-dialog,
.project-list-gantt-dialog-leave-to .project-list-gantt-dialog {
  opacity: 0;
  transform:
    perspective(1320px)
    translate3d(var(--project-list-gantt-translate-x), var(--project-list-gantt-translate-y), 0)
    scale(0.78)
    rotateX(5deg);
  filter: blur(7px);
}

.project-list-gantt-dialog__backdrop {
  --project-list-gantt-origin-x: 50vw;
  --project-list-gantt-origin-y: 18vh;
  --project-list-gantt-translate-x: 0px;
  --project-list-gantt-translate-y: 26px;
  position: fixed;
  inset: 0;
  z-index: 90;
  display: grid;
  padding: 20px;
  isolation: isolate;
  background:
    radial-gradient(circle at 16% 20%, color-mix(in srgb, var(--accent-end) 20%, transparent), transparent 22%),
    radial-gradient(circle at 82% 76%, color-mix(in srgb, var(--accent-start) 14%, transparent), transparent 24%),
    rgba(4, 10, 20, 0.72);
  backdrop-filter: blur(18px);
}

.project-list-gantt-dialog__backdrop::before {
  content: '';
  position: absolute;
  inset: 0;
  background:
    linear-gradient(var(--overlay-grid-line) 1px, transparent 1px),
    linear-gradient(90deg, var(--overlay-grid-line) 1px, transparent 1px);
  background-size: 28px 28px;
  opacity: 0.12;
  pointer-events: none;
}

.project-list-gantt-dialog__glow {
  position: absolute;
  left: var(--project-list-gantt-origin-x);
  top: var(--project-list-gantt-origin-y);
  width: min(62vw, 680px);
  height: min(62vw, 680px);
  border-radius: 999px;
  background:
    radial-gradient(circle, rgba(0, 194, 255, 0.28) 0%, rgba(10, 102, 255, 0.18) 38%, transparent 72%);
  filter: blur(20px);
  opacity: 0.74;
  transform: translate(-50%, -50%);
  pointer-events: none;
  animation: project-list-gantt-glow-drift 8s cubic-bezier(0.45, 0, 0.2, 1) infinite alternate;
}

.project-list-gantt-dialog {
  min-height: 0;
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
  position: relative;
  border: 1px solid color-mix(in srgb, var(--accent-line) 24%, var(--border-soft));
  border-radius: 28px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--panel-bg) 96%, transparent), color-mix(in srgb, var(--panel-bg) 88%, transparent)),
    radial-gradient(circle at top left, color-mix(in srgb, var(--accent-end) 10%, transparent), transparent 38%);
  box-shadow:
    0 26px 60px rgba(3, 10, 24, 0.42),
    inset 0 1px 0 color-mix(in srgb, #ffffff 10%, transparent);
  overflow: hidden;
  transform-origin: var(--project-list-gantt-origin-x) var(--project-list-gantt-origin-y);
}

.project-list-gantt-dialog::before {
  content: '';
  position: absolute;
  inset: 0;
  border-radius: inherit;
  border: 1px solid color-mix(in srgb, var(--dialog-surface-border) 60%, transparent);
  background:
    linear-gradient(135deg, color-mix(in srgb, var(--accent-end) 10%, transparent), transparent 42%),
    linear-gradient(180deg, transparent, color-mix(in srgb, var(--accent-start) 4%, transparent));
  pointer-events: none;
}

.project-list-gantt-dialog::after {
  content: '';
  position: absolute;
  inset: 0 auto auto 0;
  width: 38%;
  height: 1px;
  background: linear-gradient(90deg, rgba(0, 194, 255, 0.84), transparent);
  box-shadow: 0 0 16px rgba(0, 194, 255, 0.34);
  pointer-events: none;
}

.project-list-gantt-dialog__header {
  display: flex;
  align-items: start;
  justify-content: space-between;
  gap: 18px;
  padding: 24px 28px 20px;
  border-bottom: 1px solid color-mix(in srgb, var(--border-soft) 88%, transparent);
}

.project-list-gantt-dialog__copy {
  display: grid;
  gap: 10px;
}

.project-list-gantt-dialog__eyebrow {
  margin: 0;
  color: var(--text-soft);
  font-size: 0.78rem;
  font-weight: 700;
  letter-spacing: 0.16em;
  text-transform: uppercase;
}

.project-list-gantt-dialog__copy h3,
.project-list-gantt-dialog__copy p {
  margin: 0;
}

.project-list-gantt-dialog__copy h3 {
  font-size: clamp(1.5rem, 2.4vw, 2rem);
}

.project-list-gantt-dialog__copy p {
  color: var(--text-soft);
}

.project-list-gantt-dialog__toolbar {
  display: flex;
  align-items: center;
  gap: 14px;
}

.project-list-gantt-dialog__close {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 42px;
  height: 42px;
  border: 1px solid color-mix(in srgb, var(--border-soft) 88%, transparent);
  border-radius: 14px;
  background: color-mix(in srgb, var(--glass-bg) 86%, transparent);
  color: var(--text-main);
  cursor: pointer;
}

.project-list-gantt-dialog__close svg {
  width: 18px;
  height: 18px;
}

.project-list-gantt-dialog__state {
  margin: 24px;
  padding: 20px;
  border: 1px solid color-mix(in srgb, var(--border-soft) 88%, transparent);
  border-radius: 20px;
  background: color-mix(in srgb, var(--glass-bg) 88%, transparent);
  color: var(--text-soft);
}

.project-list-gantt-dialog__state--error {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
}

.project-list-gantt-dialog__state--error strong,
.project-list-gantt-dialog__state--error p {
  margin: 0;
}

.project-list-gantt-dialog__state button {
  min-height: 40px;
  border: 0;
  border-radius: 12px;
  padding: 0 16px;
  background: var(--gradient-primary);
  color: var(--text-inverse);
  font: inherit;
  font-weight: 700;
  cursor: pointer;
}

.project-list-gantt-dialog__body-scroll {
  min-height: 0;
  padding: 0 24px 24px;
  overflow: auto;
}

.project-list-gantt-dialog__body {
  min-height: 100%;
  display: grid;
  grid-template-columns: minmax(220px, 300px) minmax(0, 1fr);
  gap: 16px;
}

.project-list-gantt-dialog__sidebar,
.project-list-gantt-dialog__timeline-shell {
  min-height: 0;
  border: 1px solid color-mix(in srgb, var(--border-soft) 88%, transparent);
  border-radius: 22px;
  background: color-mix(in srgb, var(--glass-bg) 84%, transparent);
  overflow: hidden;
}

.project-list-gantt-dialog__sidebar {
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
}

.project-list-gantt-dialog__sidebar-scroll {
  min-height: 0;
  overflow-y: auto;
  overflow-x: hidden;
}

.project-list-gantt-dialog__sidebar-head,
.project-list-gantt-dialog__sidebar-row {
  box-sizing: border-box;
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  gap: 12px;
  align-items: center;
  padding: 16px 18px;
}

.project-list-gantt-dialog__sidebar-head {
  height: var(--project-list-gantt-axis-height);
  color: var(--text-soft);
  font-size: 0.75rem;
  font-weight: 700;
  letter-spacing: 0.12em;
  text-transform: uppercase;
  border-bottom: 1px solid color-mix(in srgb, var(--border-soft) 88%, transparent);
}

.project-list-gantt-dialog__sidebar-row {
  height: var(--project-list-gantt-row-height);
  border-bottom: 1px solid color-mix(in srgb, var(--border-soft) 74%, transparent);
}

.project-list-gantt-dialog__sidebar-row:last-child {
  border-bottom: 0;
}

.project-list-gantt-dialog__sidebar-row strong {
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.project-list-gantt-dialog__sidebar-meta {
  display: flex;
  align-items: center;
  gap: 8px;
  color: var(--text-soft);
  font-size: 0.82rem;
}

.project-list-gantt-dialog__status-pill {
  display: inline-flex;
  align-items: center;
  min-height: 28px;
  padding: 0 10px;
  border-radius: 999px;
  border: 1px solid transparent;
  font-size: 0.78rem;
  font-weight: 700;
}

.project-list-gantt-dialog__status-pill--pending {
  border-color: var(--work-status-pending-border);
  background: var(--work-status-pending-bg);
  color: var(--work-status-pending-color);
}

.project-list-gantt-dialog__status-pill--active {
  border-color: var(--work-status-active-border);
  background: var(--work-status-active-bg);
  color: var(--work-status-active-color);
}

.project-list-gantt-dialog__status-pill--done {
  border-color: var(--work-status-done-border);
  background: var(--work-status-done-bg);
  color: var(--work-status-done-color);
}

.project-list-gantt-dialog__status-pill--delayed {
  border-color: var(--work-status-delayed-border);
  background: var(--work-status-delayed-bg);
  color: var(--work-status-delayed-color);
}

.project-list-gantt-dialog__timeline-shell {
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
}

.project-list-gantt-dialog__axis-scroll {
  overflow-x: auto;
  overflow-y: hidden;
  border-bottom: 1px solid color-mix(in srgb, var(--border-soft) 88%, transparent);
}

.project-list-gantt-dialog__timeline-scroll {
  min-height: 0;
  overflow: auto;
}

.project-list-gantt-dialog__timeline-canvas {
  min-width: 100%;
}

.project-list-gantt-dialog__axis {
  display: flex;
}

.project-list-gantt-dialog__axis-cell {
  flex: 0 0 auto;
  height: var(--project-list-gantt-axis-height);
  display: flex;
  align-items: center;
  justify-content: center;
  border-right: 1px solid color-mix(in srgb, var(--border-soft) 88%, transparent);
  color: var(--text-soft);
  font-size: 0.82rem;
  font-weight: 700;
}

.project-list-gantt-dialog__axis-cell:nth-child(even) {
  background: color-mix(in srgb, var(--glass-bg) 68%, transparent);
}

.project-list-gantt-dialog__rows {
  display: grid;
}

.project-list-gantt-dialog__row {
  box-sizing: border-box;
  height: var(--project-list-gantt-row-height);
  border-bottom: 1px solid color-mix(in srgb, var(--border-soft) 74%, transparent);
}

.project-list-gantt-dialog__track {
  position: relative;
  height: var(--project-list-gantt-row-height);
}

.project-list-gantt-dialog__track-grid {
  position: absolute;
  inset: 0;
  display: flex;
}

.project-list-gantt-dialog__track-cell {
  flex: 0 0 auto;
  height: 100%;
  border-right: 1px solid color-mix(in srgb, var(--border-soft) 74%, transparent);
}

.project-list-gantt-dialog__track-cell:nth-child(even) {
  background: color-mix(in srgb, var(--glass-bg) 62%, transparent);
}

.project-list-gantt-dialog__bar {
  position: absolute;
  top: 16px;
  height: 40px;
  display: inline-flex;
  align-items: center;
  justify-content: start;
  padding: 0 14px;
  border: 0;
  border-radius: 999px;
  color: #f8fbff;
  font: inherit;
  font-size: 0.84rem;
  font-weight: 700;
  cursor: pointer;
  box-shadow: 0 16px 28px rgba(7, 18, 40, 0.2);
}

.project-list-gantt-dialog__bar span {
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.project-list-gantt-dialog__bar--pending {
  background: linear-gradient(135deg, var(--work-status-pending-color), color-mix(in srgb, var(--work-status-pending-color) 60%, #ffffff));
}

.project-list-gantt-dialog__bar--active {
  background: linear-gradient(135deg, var(--work-status-active-color), color-mix(in srgb, var(--work-status-active-color) 62%, #ffffff));
}

.project-list-gantt-dialog__bar--done {
  background: linear-gradient(135deg, var(--work-status-done-color), color-mix(in srgb, var(--work-status-done-color) 62%, #ffffff));
}

.project-list-gantt-dialog__bar--delayed {
  background: linear-gradient(135deg, var(--work-status-delayed-color), color-mix(in srgb, var(--work-status-delayed-color) 62%, #ffffff));
}

@keyframes project-list-gantt-glow-drift {
  from {
    transform: translate(-52%, -48%) scale(0.98);
  }

  to {
    transform: translate(-46%, -54%) scale(1.05);
  }
}

@media (max-width: 960px) {
  .project-list-gantt-dialog__backdrop {
    padding: 12px;
  }

  .project-list-gantt-dialog__header {
    flex-direction: column;
    align-items: stretch;
  }

  .project-list-gantt-dialog__toolbar {
    justify-content: space-between;
  }

  .project-list-gantt-dialog__body {
    grid-template-columns: 1fr;
  }
}
</style>
