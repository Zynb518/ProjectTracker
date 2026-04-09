<script setup lang="ts">
import type { Subtask } from '@/types/subtask'
import { getWorkStatusLabel, getWorkStatusTone } from '@/utils/display'

defineProps<{
  tasks: Subtask[]
}>()
</script>

<template>
  <section class="my-task-board">
    <article v-for="task in tasks" :key="task.id" class="my-task-board__card">
      <div class="my-task-board__header">
        <div>
          <p>{{ task.project_name }}</p>
          <h2>{{ task.name }}</h2>
        </div>
        <span :class="['my-task-board__status', `my-task-board__status--${getWorkStatusTone(task.status)}`]">
          {{ getWorkStatusLabel(task.status) }}
        </span>
      </div>

      <dl class="my-task-board__meta">
        <div>
          <dt>阶段节点</dt>
          <dd>{{ task.node_name }}</dd>
        </div>
        <div>
          <dt>优先级</dt>
          <dd>{{ task.priority }}</dd>
        </div>
        <div>
          <dt>进度</dt>
          <dd>{{ task.progress_percent }}%</dd>
        </div>
        <div>
          <dt>计划截止</dt>
          <dd>{{ task.planned_end_date }}</dd>
        </div>
      </dl>
    </article>

    <p v-if="tasks.length === 0" class="my-task-board__empty">当前筛选条件下没有任务。</p>
  </section>
</template>

<style scoped>
.my-task-board {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
  gap: 18px;
}

.my-task-board__card {
  display: grid;
  gap: 16px;
  padding: 20px;
  border: 1px solid var(--border-soft);
  border-radius: 16px;
  background: var(--glass-bg);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
}

.my-task-board__header {
  display: flex;
  align-items: start;
  justify-content: space-between;
  gap: 16px;
}

.my-task-board__header h2,
.my-task-board__header p {
  margin: 0;
}

.my-task-board__header p {
  margin-bottom: 8px;
  color: var(--text-soft);
  font-size: 0.85rem;
  text-transform: uppercase;
  letter-spacing: 0.12em;
}

.my-task-board__status {
  padding: 8px 12px;
  border-radius: 20px;
  border: 1px solid transparent;
  font-size: 0.78rem;
  font-weight: 600;
  letter-spacing: 0.06em;
  text-transform: uppercase;
}

.my-task-board__status--pending {
  border-color: color-mix(in srgb, var(--accent-neutral) 30%, transparent);
  background: color-mix(in srgb, var(--accent-neutral) 18%, transparent);
  color: var(--text-soft);
}

.my-task-board__status--active {
  background: var(--gradient-primary);
  color: var(--text-inverse);
}

.my-task-board__status--done {
  border-color: color-mix(in srgb, var(--accent-success) 28%, transparent);
  background: color-mix(in srgb, var(--accent-success) 18%, transparent);
  color: var(--accent-success);
}

.my-task-board__status--delayed {
  border-color: color-mix(in srgb, var(--accent-warning) 28%, transparent);
  background: color-mix(in srgb, var(--accent-warning) 18%, transparent);
  color: var(--accent-warning);
}

.my-task-board__meta {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
  margin: 0;
}

.my-task-board__meta div {
  padding: 12px 14px;
  border: 1px solid var(--border-soft);
  border-radius: 14px;
  background: color-mix(in srgb, var(--panel-bg) 88%, transparent);
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
}

.my-task-board__empty {
  margin: 0;
  padding: 18px 20px;
  border: 1px solid var(--border-soft);
  border-radius: 16px;
  background: var(--glass-bg);
  color: var(--text-soft);
}
</style>
