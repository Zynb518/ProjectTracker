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
  --task-card-border: color-mix(in srgb, var(--border-soft) 92%, transparent);
  --task-card-hover-border: color-mix(in srgb, var(--task-card-accent-line) 26%, var(--accent-line));
  --task-card-bg:
    linear-gradient(
      180deg,
      color-mix(in srgb, var(--panel-bg) 96%, var(--glass-bg)),
      color-mix(in srgb, var(--glass-bg) 94%, transparent)
    ),
    radial-gradient(circle at top right, var(--task-card-accent), transparent 42%);
  --task-card-shadow: var(--shadow-panel);
  --task-card-hover-shadow:
    0 26px 48px color-mix(in srgb, #0f172a 16%, transparent),
    0 10px 24px color-mix(in srgb, var(--task-card-accent-line) 14%, transparent);
  --task-card-pill-bg: color-mix(in srgb, var(--dialog-control-bg) 98%, transparent);
  --task-card-pill-border: color-mix(in srgb, var(--border-soft) 92%, transparent);
  --task-card-subsurface-bg: color-mix(in srgb, var(--panel-bg) 90%, transparent);
  --task-card-subsurface-border: color-mix(in srgb, var(--border-soft) 90%, transparent);
  --task-card-sheen-opacity: 0;
  position: relative;
  isolation: isolate;
  overflow: hidden;
  display: grid;
  gap: 16px;
  padding: 20px;
  border: 1px solid var(--task-card-border);
  border-radius: 20px;
  cursor: pointer;
  background: var(--task-card-bg);
  box-shadow: var(--task-card-shadow);
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
  pointer-events: none;
  z-index: 2;
}

.my-task-board__card::after {
  content: '';
  position: absolute;
  inset: 0;
  background:
    linear-gradient(135deg, color-mix(in srgb, #ffffff 54%, transparent), transparent 38%),
    radial-gradient(
      circle at top right,
      color-mix(in srgb, var(--task-card-accent-line) 24%, #ffffff 76%),
      transparent 34%
    );
  opacity: var(--task-card-sheen-opacity);
  pointer-events: none;
  z-index: 0;
}

.my-task-board__card > * {
  position: relative;
  z-index: 1;
}

:global(html.light) .my-task-board__card {
  --task-card-border: rgba(108, 140, 186, 0.18);
  --task-card-hover-border: color-mix(in srgb, var(--task-card-accent-line) 28%, rgba(255, 255, 255, 0.42));
  --task-card-bg:
    linear-gradient(160deg, rgba(255, 255, 255, 0.96) 0%, rgba(239, 246, 254, 0.94) 54%, rgba(232, 241, 251, 0.9) 100%),
    linear-gradient(180deg, rgba(255, 255, 255, 0.4), rgba(255, 255, 255, 0) 30%),
    radial-gradient(circle at 100% 0%, color-mix(in srgb, var(--task-card-accent-line) 22%, #7ddcff 78%), transparent 36%);
  --task-card-shadow:
    0 22px 40px rgba(49, 76, 116, 0.11),
    inset 0 1px 0 rgba(255, 255, 255, 0.86),
    0 0 0 1px rgba(108, 140, 186, 0.08);
  --task-card-hover-shadow:
    0 30px 52px rgba(35, 65, 103, 0.16),
    0 14px 26px color-mix(in srgb, var(--task-card-accent-line) 18%, transparent),
    inset 0 1px 0 rgba(255, 255, 255, 0.92);
  --task-card-pill-bg: linear-gradient(180deg, rgba(247, 250, 255, 0.88), rgba(238, 245, 252, 0.76));
  --task-card-pill-border: rgba(110, 140, 184, 0.18);
  --task-card-subsurface-bg: linear-gradient(180deg, rgba(248, 251, 255, 0.86), rgba(239, 246, 253, 0.74));
  --task-card-subsurface-border: rgba(119, 146, 184, 0.13);
  --task-card-sheen-opacity: 1;
  backdrop-filter: blur(18px);
}

.my-task-board__card:hover {
  transform: translateY(-4px);
  border-color: var(--task-card-hover-border);
  box-shadow: var(--task-card-hover-shadow);
}

.my-task-board__card:focus-visible {
  outline: none;
  border-color: color-mix(in srgb, var(--task-card-accent-line) 32%, var(--task-card-hover-border));
  box-shadow:
    var(--task-card-hover-shadow),
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
  border: 1px solid var(--task-card-pill-border);
  border-radius: 999px;
  background: var(--task-card-pill-bg);
  box-shadow: inset 0 1px 0 color-mix(in srgb, #ffffff 52%, transparent);
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
  box-shadow: inset 0 1px 0 color-mix(in srgb, #ffffff 26%, transparent);
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
  border: 1px solid var(--task-card-pill-border);
  border-radius: 999px;
  font-size: 0.8rem;
  font-weight: 700;
  line-height: 1;
  box-shadow:
    inset 0 1px 0 color-mix(in srgb, #ffffff 44%, transparent),
    0 10px 18px color-mix(in srgb, #0f172a 6%, transparent);
}

.my-task-board__priority--low {
  background: linear-gradient(
    180deg,
    color-mix(in srgb, var(--accent-success) 10%, #ffffff 90%),
    color-mix(in srgb, var(--accent-success) 14%, var(--task-card-pill-bg))
  );
  border-color: color-mix(in srgb, var(--accent-success) 28%, transparent);
  color: color-mix(in srgb, var(--accent-success) 72%, var(--text-main));
}

.my-task-board__priority--medium {
  background: linear-gradient(
    180deg,
    color-mix(in srgb, var(--accent-start) 12%, #ffffff 88%),
    color-mix(in srgb, var(--accent-start) 16%, var(--task-card-pill-bg))
  );
  border-color: color-mix(in srgb, var(--accent-start) 28%, transparent);
  color: color-mix(in srgb, var(--accent-start) 70%, var(--text-main));
}

.my-task-board__priority--high {
  background: linear-gradient(
    180deg,
    color-mix(in srgb, var(--accent-warning) 10%, #ffffff 90%),
    color-mix(in srgb, var(--accent-warning) 16%, var(--task-card-pill-bg))
  );
  border-color: color-mix(in srgb, var(--accent-warning) 28%, transparent);
  color: color-mix(in srgb, var(--accent-warning) 74%, var(--text-main));
}

.my-task-board__priority--unknown {
  background: var(--task-card-pill-bg);
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
  border: 1px solid var(--task-card-subsurface-border);
  border-radius: 16px;
  background: var(--task-card-subsurface-bg);
  box-shadow:
    inset 0 1px 0 color-mix(in srgb, #ffffff 52%, transparent),
    0 12px 20px color-mix(in srgb, #0f172a 4%, transparent);
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
  background: color-mix(in srgb, var(--panel-bg) 80%, rgba(255, 255, 255, 0.4));
  box-shadow:
    inset 0 0 0 1px color-mix(in srgb, var(--accent-line) 18%, transparent),
    inset 0 1px 0 color-mix(in srgb, #ffffff 44%, transparent);
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
  border: 1px solid var(--task-card-subsurface-border);
  border-radius: 14px;
  background: var(--task-card-subsurface-bg);
  box-shadow:
    inset 0 1px 0 color-mix(in srgb, #ffffff 52%, transparent),
    0 10px 18px color-mix(in srgb, #0f172a 4%, transparent);
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
