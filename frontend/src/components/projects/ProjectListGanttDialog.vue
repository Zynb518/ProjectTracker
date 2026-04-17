<script setup lang="ts">
import { computed, onBeforeUnmount, watch } from 'vue'

import GanttScaleSwitcher from '@/components/workspace/GanttScaleSwitcher.vue'
import type { GanttScale } from '@/types/gantt'
import type { ProjectListItem } from '@/types/project'
import { getWorkStatusLabel, getWorkStatusTone } from '@/utils/display'
import { buildGanttAxisItems, getGanttBarLayout } from '@/utils/gantt'

let activePageScrollLocks = 0
let previousHtmlOverflow = ''
let previousBodyOverflow = ''

type DialogMotionOrigin = {
  x: number
  y: number
  translateX: number
  translateY: number
}

const PROJECT_COLUMN_WIDTH_PX = 248
const STATUS_COLUMN_WIDTH_PX = 112

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

const tableStyle = computed(() => ({
  '--project-list-gantt-project-column-width': `${PROJECT_COLUMN_WIDTH_PX}px`,
  '--project-list-gantt-status-column-width': `${STATUS_COLUMN_WIDTH_PX}px`,
  '--project-list-gantt-summary-width': `${PROJECT_COLUMN_WIDTH_PX + STATUS_COLUMN_WIDTH_PX}px`,
  '--project-list-gantt-timeline-width': `${canvasWidth.value}px`,
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

function lockPageScroll() {
  if (typeof document === 'undefined') {
    return
  }

  if (activePageScrollLocks === 0) {
    previousHtmlOverflow = document.documentElement.style.overflow
    previousBodyOverflow = document.body.style.overflow
    document.documentElement.style.overflow = 'hidden'
    document.body.style.overflow = 'hidden'
  }

  activePageScrollLocks += 1
}

function unlockPageScroll() {
  if (typeof document === 'undefined' || activePageScrollLocks === 0) {
    return
  }

  activePageScrollLocks -= 1

  if (activePageScrollLocks === 0) {
    document.documentElement.style.overflow = previousHtmlOverflow
    document.body.style.overflow = previousBodyOverflow
  }
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

watch(() => props.modelValue, (isOpen, wasOpen) => {
  if (isOpen === wasOpen) {
    return
  }

  if (isOpen) {
    lockPageScroll()
    return
  }

  unlockPageScroll()
}, { immediate: true })

onBeforeUnmount(() => {
  if (props.modelValue) {
    unlockPageScroll()
  }
})
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
              <span>关闭</span>
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

        <div v-else class="project-list-gantt-dialog__body-scroll">
          <div
            data-testid="project-list-gantt-table-scroll"
            class="project-list-gantt-dialog__table-scroll smooth-scroll-surface"
          >
            <div class="project-list-gantt-dialog__table" :style="tableStyle">
              <div class="project-list-gantt-dialog__table-header">
                <div class="project-list-gantt-dialog__header-summary">
                  <div class="project-list-gantt-dialog__header-cell project-list-gantt-dialog__header-cell--project">
                    项目
                  </div>
                  <div class="project-list-gantt-dialog__header-cell project-list-gantt-dialog__header-cell--status">
                    状态
                  </div>
                </div>
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

              <div class="project-list-gantt-dialog__table-body">
                <div
                  v-for="project in projects"
                  :key="project.id"
                  :data-testid="`project-list-gantt-row-${project.id}`"
                  class="project-list-gantt-dialog__table-row"
                >
                  <div class="project-list-gantt-dialog__row-summary">
                    <div class="project-list-gantt-dialog__row-label">
                      <strong>{{ project.name }}</strong>
                    </div>

                    <div class="project-list-gantt-dialog__row-status">
                      <span
                        :class="[
                          'project-list-gantt-dialog__status-pill',
                          `project-list-gantt-dialog__status-pill--${getWorkStatusTone(project.status)}`,
                        ]"
                      >
                        {{ getWorkStatusLabel(project.status) }}
                      </span>
                    </div>
                  </div>

                  <div class="project-list-gantt-dialog__timeline-cell">
                    <div class="project-list-gantt-dialog__track">
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
      </section>
    </div>
  </Transition>
</template>

<style scoped>
.project-list-gantt-dialog {
  --project-list-gantt-axis-height: 58px;
  --project-list-gantt-row-height: 72px;
  --project-list-gantt-project-column-width: 248px;
  --project-list-gantt-status-column-width: 112px;
  --project-list-gantt-summary-width: 360px;
  --project-list-gantt-timeline-width: 0px;
  --project-list-gantt-divider: color-mix(in srgb, var(--border-soft) 78%, transparent);
  min-height: 0;
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
  position: relative;
  border: 1px solid color-mix(in srgb, var(--accent-line) 18%, var(--border-soft));
  border-radius: 24px;
  background: linear-gradient(180deg, color-mix(in srgb, var(--panel-bg) 98%, transparent), color-mix(in srgb, var(--panel-bg) 94%, transparent));
  box-shadow: 0 18px 40px rgba(3, 10, 24, 0.28);
  overflow: hidden;
  transform-origin: var(--project-list-gantt-origin-x) var(--project-list-gantt-origin-y);
}

.project-list-gantt-dialog-enter-active,
.project-list-gantt-dialog-leave-active {
  transition:
    opacity 220ms ease-out,
    background-color 220ms ease-out;
}

.project-list-gantt-dialog-enter-active .project-list-gantt-dialog,
.project-list-gantt-dialog-leave-active .project-list-gantt-dialog {
  transition:
    opacity 240ms ease-out,
    transform 260ms cubic-bezier(0.16, 1, 0.3, 1);
}

.project-list-gantt-dialog-enter-from,
.project-list-gantt-dialog-leave-to {
  opacity: 0;
}

.project-list-gantt-dialog-enter-from .project-list-gantt-dialog,
.project-list-gantt-dialog-leave-to .project-list-gantt-dialog {
  opacity: 0;
  transform:
    translate3d(var(--project-list-gantt-translate-x), var(--project-list-gantt-translate-y), 0)
    scale(0.98);
}

.project-list-gantt-dialog__backdrop {
  --project-list-gantt-origin-x: 50vw;
  --project-list-gantt-origin-y: 18vh;
  --project-list-gantt-translate-x: 0px;
  --project-list-gantt-translate-y: 18px;
  position: fixed;
  inset: 0;
  z-index: 90;
  display: grid;
  padding: 20px;
  background: rgba(4, 10, 20, 0.68);
}

.project-list-gantt-dialog__header {
  display: flex;
  align-items: start;
  justify-content: space-between;
  gap: 18px;
  padding: 24px 24px 18px;
  border-bottom: 1px solid var(--project-list-gantt-divider);
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
  font-size: clamp(1.4rem, 2.2vw, 1.9rem);
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
  gap: 8px;
  min-width: 76px;
  height: 40px;
  padding: 0 14px;
  border: 1px solid var(--project-list-gantt-divider);
  border-radius: 12px;
  background: color-mix(in srgb, var(--panel-bg) 92%, #ffffff 8%);
  color: var(--text-main);
  cursor: pointer;
  flex-shrink: 0;
  font: inherit;
  font-size: 0.82rem;
  font-weight: 700;
}

.project-list-gantt-dialog__close svg {
  width: 18px;
  height: 18px;
  flex-shrink: 0;
}

.project-list-gantt-dialog__state {
  margin: 24px;
  padding: 20px;
  border: 1px solid var(--project-list-gantt-divider);
  border-radius: 18px;
  background: var(--panel-bg);
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
  padding: 0;
  overflow: hidden;
}

.project-list-gantt-dialog__table-scroll {
  min-height: 0;
  height: 100%;
  overflow-x: scroll;
  overflow-y: scroll;
  scrollbar-gutter: stable;
  scrollbar-width: auto;
  scrollbar-color: color-mix(in srgb, var(--text-soft) 68%, transparent) color-mix(in srgb, var(--panel-bg) 90%, transparent);
  border-radius: 0 0 24px 24px;
  background: var(--panel-bg);
  overscroll-behavior: contain;
}

.project-list-gantt-dialog__table-scroll::-webkit-scrollbar {
  width: 12px;
  height: 12px;
}

.project-list-gantt-dialog__table-scroll::-webkit-scrollbar-track {
  background: color-mix(in srgb, var(--panel-bg) 92%, transparent);
}

.project-list-gantt-dialog__table-scroll::-webkit-scrollbar-thumb {
  border: 2px solid transparent;
  border-radius: 999px;
  background: color-mix(in srgb, var(--text-soft) 68%, transparent);
  background-clip: padding-box;
}

.project-list-gantt-dialog__table-scroll::-webkit-scrollbar-thumb:hover {
  background: color-mix(in srgb, var(--text-main) 72%, transparent);
  background-clip: padding-box;
}

.project-list-gantt-dialog__table {
  width: calc(
    var(--project-list-gantt-project-column-width)
    + var(--project-list-gantt-status-column-width)
    + var(--project-list-gantt-timeline-width)
  );
  min-width: 100%;
}

.project-list-gantt-dialog__table-header,
.project-list-gantt-dialog__table-row {
  display: grid;
  grid-template-columns:
    var(--project-list-gantt-summary-width)
    var(--project-list-gantt-timeline-width);
}

.project-list-gantt-dialog__table-header {
  position: sticky;
  top: 0;
  z-index: 6;
  min-height: var(--project-list-gantt-axis-height);
  border-bottom: 1px solid var(--project-list-gantt-divider);
  background: var(--panel-bg);
}

.project-list-gantt-dialog__header-summary,
.project-list-gantt-dialog__row-summary {
  display: grid;
  grid-template-columns:
    var(--project-list-gantt-project-column-width)
    var(--project-list-gantt-status-column-width);
  min-width: 0;
  position: sticky;
  left: 0;
  background: var(--panel-bg);
  border-right: 1px solid var(--project-list-gantt-divider);
}

.project-list-gantt-dialog__header-summary {
  z-index: 8;
}

.project-list-gantt-dialog__row-summary {
  z-index: 3;
}

.project-list-gantt-dialog__header-cell,
.project-list-gantt-dialog__row-label,
.project-list-gantt-dialog__row-status {
  display: flex;
  align-items: center;
  min-width: 0;
  padding: 0 16px;
  border-right: 1px solid var(--project-list-gantt-divider);
  background: var(--panel-bg);
}

.project-list-gantt-dialog__header-cell {
  color: var(--text-soft);
  font-size: 0.76rem;
  font-weight: 700;
  letter-spacing: 0.12em;
  text-transform: uppercase;
}

.project-list-gantt-dialog__row-status {
  justify-content: center;
  border-right: 0;
}

.project-list-gantt-dialog__row-label {
  min-width: 0;
}

.project-list-gantt-dialog__axis {
  display: flex;
  min-height: var(--project-list-gantt-axis-height);
  background: var(--panel-bg);
}

.project-list-gantt-dialog__axis-cell {
  flex: 0 0 auto;
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: var(--project-list-gantt-axis-height);
  border-right: 1px solid var(--project-list-gantt-divider);
  color: var(--text-soft);
  font-size: 0.8rem;
  font-weight: 700;
  white-space: nowrap;
}

.project-list-gantt-dialog__table-body {
  display: grid;
}

.project-list-gantt-dialog__table-row {
  min-height: var(--project-list-gantt-row-height);
  border-bottom: 1px solid var(--project-list-gantt-divider);
}

.project-list-gantt-dialog__table-row:last-child {
  border-bottom: 0;
}

.project-list-gantt-dialog__row-label strong {
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.project-list-gantt-dialog__timeline-cell {
  min-width: 0;
  background: var(--panel-bg);
  contain: paint;
}

.project-list-gantt-dialog__track {
  position: relative;
  height: var(--project-list-gantt-row-height);
  background: var(--panel-bg);
}

.project-list-gantt-dialog__status-pill {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-height: 28px;
  padding: 0 10px;
  border-radius: 999px;
  border: 1px solid transparent;
  font-size: 0.78rem;
  font-weight: 700;
  white-space: nowrap;
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

.project-list-gantt-dialog__bar {
  position: absolute;
  top: 50%;
  height: 36px;
  display: inline-flex;
  align-items: center;
  justify-content: start;
  padding: 0 14px;
  border: 0;
  border-radius: 10px;
  font: inherit;
  font-size: 0.84rem;
  font-weight: 700;
  cursor: pointer;
  transform: translateY(-50%);
}

.project-list-gantt-dialog__bar span {
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.project-list-gantt-dialog__bar--pending {
  background: var(--work-status-pending-strong);
  color: var(--work-status-pending-contrast);
}

.project-list-gantt-dialog__bar--active {
  background: var(--work-status-active-strong);
  color: var(--work-status-active-contrast);
}

.project-list-gantt-dialog__bar--done {
  background: var(--work-status-done-strong);
  color: var(--work-status-done-contrast);
}

.project-list-gantt-dialog__bar--delayed {
  background: var(--work-status-delayed-strong);
  color: var(--work-status-delayed-contrast);
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

  .project-list-gantt-dialog {
    --project-list-gantt-project-column-width: 210px;
    --project-list-gantt-status-column-width: 96px;
  }

  .project-list-gantt-dialog__body-scroll {
    padding: 0;
  }

  .project-list-gantt-dialog__header {
    padding: 20px 16px 16px;
  }
}
</style>
