<script setup lang="ts">
import type { Subtask } from '@/types/subtask'
import { getWorkStatusLabel, getWorkStatusTone } from '@/utils/display'

type PriorityTone = 'high' | 'low' | 'medium' | 'unknown'

defineProps<{
  tasks: Subtask[]
}>()

const emit = defineEmits<{
  'open-progress': [subtaskId: number]
}>()

function priorityLabel(priority: number) {
  return (
    {
      1: '低',
      2: '中',
      3: '高',
    }[priority] ?? '未设定'
  )
}

function priorityTone(priority: number): PriorityTone {
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

function formatUpdatedAt(value: string) {
  return value.slice(0, 16).replace('T', ' ')
}

function emitOpenProgress(subtaskId: number) {
  emit('open-progress', subtaskId)
}

function handleCardKeydown(event: KeyboardEvent, subtaskId: number) {
  if (event.key !== 'Enter' && event.key !== ' ') {
    return
  }

  event.preventDefault()
  emitOpenProgress(subtaskId)
}
</script>

<template>
  <section class="my-task-board">
    <article
      v-for="task in tasks"
      :key="task.id"
      :class="['my-task-board__card', `my-task-board__card--${getWorkStatusTone(task.status)}`]"
      :data-testid="`my-task-card-${task.id}`"
      role="button"
      tabindex="0"
      @click="emitOpenProgress(task.id)"
      @keydown="handleCardKeydown($event, task.id)"
    >
      <div class="my-task-board__header">
        <div class="my-task-board__title-stack">
          <p class="my-task-board__project" :title="task.project_name || '未归属项目'">
            {{ task.project_name || '未归属项目' }}
          </p>
          <span class="my-task-board__node" :title="task.node_name || '未归属节点'">
            {{ task.node_name || '未归属节点' }}
          </span>
          <h2>{{ task.name }}</h2>
        </div>
        <span :class="['my-task-board__status', `my-task-board__status--${getWorkStatusTone(task.status)}`]">
          {{ getWorkStatusLabel(task.status) }}
        </span>
      </div>

      <div class="my-task-board__signal-row">
        <span :class="['my-task-board__priority', `my-task-board__priority--${priorityTone(task.priority)}`]">
          {{ priorityLabel(task.priority) }}优先级
        </span>

        <div class="my-task-board__deadline">
          <span>计划截止</span>
          <strong>{{ task.planned_end_date }}</strong>
        </div>
      </div>

      <section class="my-task-board__progress" :aria-label="`进度 ${task.progress_percent}%`">
        <div class="my-task-board__progress-head">
          <span>当前进度</span>
          <strong>{{ task.progress_percent }}%</strong>
        </div>
        <div class="my-task-board__progress-track">
          <span
            class="my-task-board__progress-fill"
            :style="{ width: `${task.progress_percent}%` }"
          />
        </div>
      </section>

      <dl class="my-task-board__meta">
        <div>
          <dt>计划开始</dt>
          <dd>{{ task.planned_start_date }}</dd>
        </div>
        <div>
          <dt>最近更新</dt>
          <dd>{{ formatUpdatedAt(task.updated_at) }}</dd>
        </div>
      </dl>
    </article>

    <p v-if="tasks.length === 0" class="my-task-board__empty">当前筛选条件下没有任务。</p>
  </section>
</template>

<style scoped>
.my-task-board {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(320px, 1fr));
  gap: 18px;
}

.my-task-board__card {
  --task-card-accent: color-mix(in srgb, var(--accent-neutral) 12%, transparent);
  --task-card-accent-line: var(--accent-neutral);
  position: relative;
  overflow: hidden;
  display: grid;
  gap: 16px;
  padding: 20px;
  border: 1px solid color-mix(in srgb, var(--border-soft) 92%, transparent);
  border-radius: 20px;
  cursor: pointer;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--panel-bg) 96%, var(--glass-bg)), color-mix(in srgb, var(--glass-bg) 94%, transparent)),
    radial-gradient(circle at top right, var(--task-card-accent), transparent 42%);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
  transition:
    transform 200ms ease-out,
    box-shadow 200ms ease-out,
    border-color 200ms ease-out,
    background 200ms ease-out;
}

.my-task-board__card::before {
  content: '';
  position: absolute;
  inset: 0 0 auto 0;
  height: 4px;
  background: linear-gradient(90deg, color-mix(in srgb, var(--task-card-accent-line) 84%, #ffffff 16%), transparent);
  opacity: 0.9;
}

.my-task-board__card:hover {
  transform: translateY(-4px);
  border-color: color-mix(in srgb, var(--task-card-accent-line) 26%, var(--accent-line));
  box-shadow:
    0 26px 48px color-mix(in srgb, #0f172a 16%, transparent),
    0 10px 24px color-mix(in srgb, var(--task-card-accent-line) 14%, transparent);
}

.my-task-board__card:focus-visible {
  outline: none;
  border-color: color-mix(in srgb, var(--task-card-accent-line) 32%, var(--accent-line));
  box-shadow:
    0 26px 48px color-mix(in srgb, #0f172a 16%, transparent),
    0 10px 24px color-mix(in srgb, var(--task-card-accent-line) 14%, transparent),
    0 0 0 4px rgba(10, 102, 255, 0.12);
}

.my-task-board__card--pending {
  --task-card-accent: color-mix(in srgb, var(--accent-neutral) 14%, transparent);
  --task-card-accent-line: var(--accent-neutral);
}

.my-task-board__card--active {
  --task-card-accent: color-mix(in srgb, var(--accent-start) 18%, transparent);
  --task-card-accent-line: var(--accent-start);
}

.my-task-board__card--done {
  --task-card-accent: color-mix(in srgb, var(--accent-success) 18%, transparent);
  --task-card-accent-line: var(--accent-success);
}

.my-task-board__card--delayed {
  --task-card-accent: color-mix(in srgb, var(--accent-warning) 18%, transparent);
  --task-card-accent-line: var(--accent-warning);
}

.my-task-board__card--unknown {
  --task-card-accent: color-mix(in srgb, var(--accent-neutral) 12%, transparent);
  --task-card-accent-line: var(--accent-neutral);
}

.my-task-board__header {
  display: flex;
  align-items: start;
  justify-content: space-between;
  gap: 16px;
}

.my-task-board__header h2,
.my-task-board__project,
.my-task-board__node,
.my-task-board__priority,
.my-task-board__deadline span,
.my-task-board__deadline strong,
.my-task-board__progress-head span,
.my-task-board__progress-head strong,
.my-task-board__meta dt,
.my-task-board__meta dd {
  margin: 0;
}

.my-task-board__title-stack {
  min-width: 0;
  display: grid;
  gap: 8px;
}

.my-task-board__project {
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  color: var(--text-soft);
  font-size: 0.85rem;
  text-transform: uppercase;
  letter-spacing: 0.12em;
}

.my-task-board__node {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  justify-self: start;
  min-width: 0;
  max-width: 100%;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  min-height: 30px;
  padding: 0 10px;
  border: 1px solid color-mix(in srgb, var(--border-soft) 92%, transparent);
  border-radius: 999px;
  background: color-mix(in srgb, var(--dialog-control-bg) 98%, transparent);
  color: var(--text-main);
  font-size: 0.76rem;
  font-weight: 700;
}

.my-task-board__header h2 {
  font-size: 1.24rem;
  line-height: 1.26;
  color: color-mix(in srgb, var(--text-main) 94%, var(--accent-end) 6%);
  transition:
    color 200ms ease-out,
    text-shadow 200ms ease-out;
}

.my-task-board__card:hover .my-task-board__header h2 {
  color: color-mix(in srgb, var(--text-main) 98%, #ffffff 2%);
  text-shadow: 0 1px 0 color-mix(in srgb, #ffffff 18%, transparent);
}

.my-task-board__status {
  flex-shrink: 0;
  padding: 8px 12px;
  border-radius: 20px;
  border: 1px solid transparent;
  font-size: 0.78rem;
  font-weight: 600;
  letter-spacing: 0.06em;
  text-transform: uppercase;
  transition:
    transform 200ms ease-out,
    box-shadow 200ms ease-out;
}

.my-task-board__card:hover .my-task-board__status {
  transform: translateY(-1px);
  box-shadow:
    inset 0 1px 0 color-mix(in srgb, #ffffff 18%, transparent),
    0 10px 18px color-mix(in srgb, var(--task-card-accent-line) 16%, transparent);
}

.my-task-board__status--pending {
  border-color: var(--work-status-pending-border);
  background: var(--work-status-pending-bg);
  color: var(--work-status-pending-color);
}

.my-task-board__status--active {
  border-color: var(--work-status-active-border);
  background: var(--work-status-active-bg);
  color: var(--work-status-active-color);
}

.my-task-board__status--done {
  border-color: var(--work-status-done-border);
  background: var(--work-status-done-bg);
  color: var(--work-status-done-color);
}

.my-task-board__status--delayed {
  border-color: var(--work-status-delayed-border);
  background: var(--work-status-delayed-bg);
  color: var(--work-status-delayed-color);
}

.my-task-board__status--unknown {
  border-color: var(--work-status-unknown-border);
  background: var(--work-status-unknown-bg);
  color: var(--work-status-unknown-color);
}

.my-task-board__signal-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.my-task-board__priority {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-height: 34px;
  padding: 0 12px;
  border: 1px solid var(--dialog-control-border);
  border-radius: 999px;
  font-size: 0.8rem;
  font-weight: 700;
  line-height: 1;
  box-shadow: var(--dialog-control-shadow);
}

.my-task-board__priority--low {
  background: color-mix(in srgb, var(--accent-success) 14%, var(--dialog-control-bg));
  border-color: color-mix(in srgb, var(--accent-success) 28%, transparent);
  color: color-mix(in srgb, var(--accent-success) 72%, var(--text-main));
}

.my-task-board__priority--medium {
  background: color-mix(in srgb, var(--accent-start) 16%, var(--dialog-control-bg));
  border-color: color-mix(in srgb, var(--accent-start) 28%, transparent);
  color: color-mix(in srgb, var(--accent-start) 70%, var(--text-main));
}

.my-task-board__priority--high {
  background: color-mix(in srgb, var(--accent-warning) 16%, var(--dialog-control-bg));
  border-color: color-mix(in srgb, var(--accent-warning) 28%, transparent);
  color: color-mix(in srgb, var(--accent-warning) 74%, var(--text-main));
}

.my-task-board__priority--unknown {
  background: var(--dialog-control-bg);
  color: var(--text-soft);
}

.my-task-board__deadline {
  display: grid;
  gap: 4px;
  justify-items: end;
}

.my-task-board__deadline span {
  font-size: 0.72rem;
  letter-spacing: 0.08em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.my-task-board__deadline strong {
  font-size: 0.94rem;
  color: var(--text-main);
}

.my-task-board__progress {
  display: grid;
  gap: 8px;
  padding: 12px 14px;
  border: 1px solid color-mix(in srgb, var(--border-soft) 90%, transparent);
  border-radius: 16px;
  background: color-mix(in srgb, var(--panel-bg) 90%, transparent);
}

.my-task-board__progress-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
}

.my-task-board__progress-head span {
  font-size: 0.72rem;
  letter-spacing: 0.08em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.my-task-board__progress-head strong {
  font-size: 1rem;
  color: var(--text-main);
}

.my-task-board__progress-track {
  position: relative;
  overflow: hidden;
  height: 8px;
  border-radius: 999px;
  background: color-mix(in srgb, var(--panel-bg) 80%, transparent);
  box-shadow: inset 0 0 0 1px color-mix(in srgb, var(--accent-line) 18%, transparent);
}

.my-task-board__progress-fill {
  position: absolute;
  inset: 0 auto 0 0;
  border-radius: inherit;
  background: linear-gradient(90deg, var(--accent-start), var(--accent-end));
  box-shadow:
    0 0 0 1px color-mix(in srgb, #ffffff 18%, transparent),
    0 0 16px color-mix(in srgb, var(--accent-end) 20%, transparent);
}

.my-task-board__meta {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
  margin: 0;
}

.my-task-board__meta div {
  padding: 12px 14px;
  border: 1px solid color-mix(in srgb, var(--border-soft) 90%, transparent);
  border-radius: 14px;
  background: color-mix(in srgb, var(--dialog-control-bg) 96%, transparent);
}

.my-task-board__meta dt {
  font-size: 0.78rem;
  letter-spacing: 0.1em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.my-task-board__meta dd {
  margin: 8px 0 0;
  font-weight: 700;
  color: var(--text-main);
}

.my-task-board__empty {
  margin: 0;
  padding: 18px 20px;
  border: 1px solid var(--border-soft);
  border-radius: 16px;
  background: var(--glass-bg);
  color: var(--text-soft);
}

@media (max-width: 720px) {
  .my-task-board__signal-row {
    align-items: start;
    flex-direction: column;
  }

  .my-task-board__deadline {
    justify-items: start;
  }

  .my-task-board__meta {
    grid-template-columns: 1fr;
  }
}
</style>
