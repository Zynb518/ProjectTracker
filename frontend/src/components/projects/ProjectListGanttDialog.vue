<script setup lang="ts">
import { computed, onBeforeUnmount, ref, watch } from 'vue'

import GanttScaleSwitcher from '@/components/workspace/GanttScaleSwitcher.vue'
import type { GanttScale } from '@/types/gantt'
import type { ProjectListItem } from '@/types/project'
import { getWorkStatusLabel, getWorkStatusTone } from '@/utils/display'
import { buildGanttAxisItems, getGanttBarLayout } from '@/utils/gantt'
import { normalizeWheelDelta } from '@/utils/smoothWheel'

let activePageScrollLocks = 0
let previousHtmlOverflow = ''
let previousBodyOverflow = ''

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

const sidebarScrollRef = ref<HTMLElement | null>(null)
const sidebarRowsRailRef = ref<HTMLElement | null>(null)
const axisScrollRef = ref<HTMLElement | null>(null)
const topScrollRef = ref<HTMLElement | null>(null)
const timelineScrollRef = ref<HTMLElement | null>(null)
const ignoredHorizontalScroll = new WeakMap<HTMLElement, number>()

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
  if (event.defaultPrevented || event.ctrlKey || timelineScrollRef.value === null) {
    return
  }

  const delta = normalizeWheelDelta(event, timelineScrollRef.value.clientHeight).y

  if (Math.abs(delta) < 0.1) {
    return
  }

  const maxScrollTop = Math.max(timelineScrollRef.value.scrollHeight - timelineScrollRef.value.clientHeight, 0)
  const unclampedScrollTop = timelineScrollRef.value.scrollTop + delta
  const nextScrollTop = maxScrollTop > 0
    ? Math.min(Math.max(unclampedScrollTop, 0), maxScrollTop)
    : unclampedScrollTop

  if (nextScrollTop === timelineScrollRef.value.scrollTop) {
    return
  }

  event.preventDefault()
  timelineScrollRef.value.scrollTop = nextScrollTop
  syncSidebarOffset(nextScrollTop)
}

function handleTopHorizontalScroll() {
  if (!shouldIgnoreProgrammaticScroll(ignoredHorizontalScroll, topScrollRef.value, 'scrollLeft')) {
    syncScrollPosition(ignoredHorizontalScroll, topScrollRef.value, timelineScrollRef.value, 'scrollLeft')
    syncScrollPosition(ignoredHorizontalScroll, topScrollRef.value, axisScrollRef.value, 'scrollLeft')
  }
}

function handleTimelineScroll() {
  if (timelineScrollRef.value === null) {
    return
  }

  syncSidebarOffset(timelineScrollRef.value.scrollTop)

  if (shouldIgnoreProgrammaticScroll(ignoredHorizontalScroll, timelineScrollRef.value, 'scrollLeft')) {
    return
  }

  syncScrollPosition(ignoredHorizontalScroll, timelineScrollRef.value, topScrollRef.value, 'scrollLeft')
  syncScrollPosition(ignoredHorizontalScroll, timelineScrollRef.value, axisScrollRef.value, 'scrollLeft')
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

        <div v-else class="project-list-gantt-dialog__body-scroll smooth-scroll-surface">
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
                @wheel="handleSidebarWheel"
              >
                <div ref="sidebarRowsRailRef" class="project-list-gantt-dialog__sidebar-rows-rail">
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
              </div>
            </aside>

            <div class="project-list-gantt-dialog__timeline-shell">
              <div
                ref="topScrollRef"
                data-testid="project-list-gantt-top-scroll"
                class="project-list-gantt-dialog__top-scroll smooth-scroll-surface"
                @scroll="handleTopHorizontalScroll"
              >
                <div
                  aria-hidden="true"
                  data-testid="project-list-gantt-top-scroll-content"
                  class="project-list-gantt-dialog__top-scroll-content"
                  :style="timelineCanvasStyle"
                />
              </div>

              <div ref="axisScrollRef" class="project-list-gantt-dialog__axis-scroll-viewport">
                <div class="project-list-gantt-dialog__axis" data-testid="project-list-gantt-axis" :style="timelineCanvasStyle">
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

              <div
                ref="timelineScrollRef"
                data-testid="project-list-gantt-timeline-scroll"
                class="project-list-gantt-dialog__timeline-scroll-host smooth-scroll-surface"
                @scroll="handleTimelineScroll"
              >
                <div class="project-list-gantt-dialog__timeline-canvas" :style="timelineCanvasStyle">
                  <div data-testid="project-list-gantt-rows-scroll" class="project-list-gantt-dialog__rows-viewport">
                    <div class="project-list-gantt-dialog__rows">
                      <div
                        v-for="project in projects"
                        :key="project.id"
                        class="project-list-gantt-dialog__row"
                      >
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
  opacity: 0.42;
  transform: translate(-50%, -50%);
  pointer-events: none;
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
  overflow: hidden;
}

.project-list-gantt-dialog__sidebar-rows-rail {
  transform: translateY(0);
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
  contain: layout paint;
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
  min-height: 0;
  display: grid;
  grid-template-rows: auto auto minmax(0, 1fr);
  background: var(--panel-bg);
  overflow: hidden;
}

.project-list-gantt-dialog__top-scroll {
  height: 18px;
  overflow-x: auto;
  overflow-y: hidden;
  background: var(--panel-bg);
  border-bottom: 1px solid color-mix(in srgb, var(--border-soft) 88%, transparent);
}

.project-list-gantt-dialog__top-scroll-content {
  height: 1px;
}

.project-list-gantt-dialog__axis-scroll-viewport {
  overflow: hidden;
  background: var(--panel-bg);
  border-bottom: 1px solid color-mix(in srgb, var(--border-soft) 88%, transparent);
}

.project-list-gantt-dialog__timeline-scroll-host {
  min-height: 0;
  height: 100%;
  background: var(--panel-bg);
  overflow-x: auto;
  overflow-y: auto;
}

.project-list-gantt-dialog__timeline-canvas {
  min-width: 100%;
  min-height: 100%;
}

.project-list-gantt-dialog__axis {
  display: flex;
  background: var(--panel-bg);
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

.project-list-gantt-dialog__rows {
  display: grid;
}

.project-list-gantt-dialog__rows-viewport {
  min-height: 0;
}

.project-list-gantt-dialog__row {
  box-sizing: border-box;
  height: var(--project-list-gantt-row-height);
  border-bottom: 1px solid color-mix(in srgb, var(--border-soft) 74%, transparent);
  contain: layout paint;
}

.project-list-gantt-dialog__track {
  position: relative;
  height: var(--project-list-gantt-row-height);
  background: var(--panel-bg);
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

  .project-list-gantt-dialog__body {
    grid-template-columns: 1fr;
  }
}
</style>
