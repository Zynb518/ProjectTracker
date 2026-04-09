<script setup lang="ts">
import { computed, ref } from 'vue'

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

const hoveredSubtaskId = ref<number | null>(null)

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

const hoveredSubtask = computed(() =>
  props.gantt?.subtasks.find((subtask) => subtask.id === hoveredSubtaskId.value) ?? null,
)

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

function showSubtaskDetail(subtaskId: number) {
  hoveredSubtaskId.value = subtaskId
}

function clearSubtaskDetail(subtaskId: number) {
  if (hoveredSubtaskId.value === subtaskId) {
    hoveredSubtaskId.value = null
  }
}
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

        <div v-else class="node-gantt-dialog__body">
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
            <div class="node-gantt-dialog__timeline-scroll">
              <div class="node-gantt-dialog__timeline-canvas" :style="{ width: `${canvasWidth}px` }">
                <div class="node-gantt-dialog__axis" data-testid="node-gantt-axis">
                  <div
                    v-for="item in axisItems"
                    :key="item.key"
                    class="node-gantt-dialog__axis-cell"
                    :style="{ width: `${item.widthPx}px` }"
                  >
                    {{ item.label }}
                  </div>
                </div>

                <div class="node-gantt-dialog__rows">
                  <div v-for="subtask in gantt.subtasks" :key="subtask.id" class="node-gantt-dialog__row">
                    <div class="node-gantt-dialog__track">
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
                        @focus="showSubtaskDetail(subtask.id)"
                        @mouseleave="clearSubtaskDetail(subtask.id)"
                        @mouseenter="showSubtaskDetail(subtask.id)"
                      >
                        <span>{{ subtask.name }}</span>
                      </button>
                    </div>
                  </div>
                </div>
              </div>
            </div>

            <aside v-if="hoveredSubtask" class="node-gantt-dialog__detail-card">
              <span class="node-gantt-dialog__detail-eyebrow">子任务详情</span>
              <strong>{{ hoveredSubtask.name }}</strong>
              <p>{{ hoveredSubtask.responsible_real_name }} · {{ hoveredSubtask.progress_percent }}%</p>
              <dl>
                <div>
                  <dt>状态</dt>
                  <dd>{{ getWorkStatusLabel(hoveredSubtask.status) }}</dd>
                </div>
                <div>
                  <dt>周期</dt>
                  <dd>{{ hoveredSubtask.planned_start_date }}<br>{{ hoveredSubtask.planned_end_date }}</dd>
                </div>
                <div>
                  <dt>优先级</dt>
                  <dd>{{ hoveredSubtask.priority }}</dd>
                </div>
              </dl>
            </aside>
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
  background: color-mix(in srgb, var(--bg-main) 54%, transparent);
  backdrop-filter: blur(16px);
}

.node-gantt-dialog {
  width: min(1240px, calc(100vw - 56px));
  max-height: min(820px, calc(100vh - 56px));
  overflow: hidden;
  display: grid;
  gap: 18px;
  padding: 24px;
  border: 1px solid color-mix(in srgb, var(--accent-line) 34%, var(--border-soft));
  border-radius: 28px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--glass-bg) 97%, transparent), color-mix(in srgb, var(--glass-bg) 88%, transparent)),
    radial-gradient(circle at top right, color-mix(in srgb, var(--accent-end) 18%, transparent), transparent 36%);
  box-shadow: var(--shadow-panel-hover);
  backdrop-filter: var(--backdrop-blur);
}

.node-gantt-dialog__header,
.node-gantt-dialog__body,
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
.node-gantt-dialog__copy p,
.node-gantt-dialog__detail-card strong,
.node-gantt-dialog__detail-card p,
.node-gantt-dialog__detail-card dt,
.node-gantt-dialog__detail-card dd {
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
  background: color-mix(in srgb, var(--glass-bg) 84%, transparent);
  box-shadow: inset 0 0 0 1px color-mix(in srgb, var(--border-soft) 84%, transparent);
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

.node-gantt-dialog__body {
  min-height: 0;
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
  border: 1px solid var(--meta-surface-border);
  border-radius: 18px;
  background: var(--meta-surface-bg), var(--meta-surface-glow);
  box-shadow: var(--meta-surface-shadow);
}

.node-gantt-dialog__sidebar-head {
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
}

.node-gantt-dialog__status-pill--pending {
  background: color-mix(in srgb, var(--text-muted) 18%, transparent);
  color: var(--text-soft);
}

.node-gantt-dialog__status-pill--active {
  background: linear-gradient(135deg, color-mix(in srgb, var(--accent-start) 24%, transparent), color-mix(in srgb, var(--accent-end) 26%, transparent));
  color: color-mix(in srgb, var(--accent-end) 84%, #ffffff 16%);
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
  gap: 14px;
}

.node-gantt-dialog__timeline-scroll {
  overflow: auto;
  padding-bottom: 6px;
}

.node-gantt-dialog__timeline-canvas {
  min-width: 100%;
}

.node-gantt-dialog__axis {
  display: flex;
  min-height: 56px;
  margin-bottom: 10px;
}

.node-gantt-dialog__axis-cell {
  flex: 0 0 auto;
  display: flex;
  align-items: center;
  padding: 0 12px;
  border-top: 1px solid color-mix(in srgb, var(--accent-line) 20%, transparent);
  border-bottom: 1px solid color-mix(in srgb, var(--accent-line) 20%, transparent);
  border-right: 1px solid color-mix(in srgb, var(--accent-line) 16%, transparent);
  font-size: 0.8rem;
  color: var(--text-soft);
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
  border: 1px solid color-mix(in srgb, var(--accent-line) 18%, transparent);
  border-radius: 18px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--glass-bg) 74%, transparent), transparent),
    repeating-linear-gradient(90deg, color-mix(in srgb, var(--grid-line) 80%, transparent) 0, color-mix(in srgb, var(--grid-line) 80%, transparent) 1px, transparent 1px, transparent 36px);
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
  transform: translateY(calc(-50% - 1px));
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
  align-self: start;
  display: grid;
  gap: 10px;
  padding: 18px;
  border: 1px solid color-mix(in srgb, var(--accent-line) 24%, var(--border-soft));
  border-radius: 20px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--glass-bg) 96%, transparent), color-mix(in srgb, var(--glass-bg) 86%, transparent)),
    radial-gradient(circle at top right, color-mix(in srgb, var(--accent-end) 14%, transparent), transparent 38%);
  box-shadow: var(--shadow-panel);
}

.node-gantt-dialog__detail-eyebrow {
  font-size: 0.72rem;
  letter-spacing: 0.18em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.node-gantt-dialog__detail-card dl {
  display: grid;
  gap: 8px;
}

.node-gantt-dialog__detail-card dl div {
  display: grid;
  gap: 4px;
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
    max-height: calc(100vh - 24px);
    padding: 18px;
  }

  .node-gantt-dialog__body {
    grid-template-columns: 1fr;
  }
}
</style>
