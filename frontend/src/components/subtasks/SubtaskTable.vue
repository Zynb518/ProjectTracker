<script setup lang="ts">
import type { ProjectNode } from '@/types/node'
import type { Subtask } from '@/types/subtask'

defineProps<{
  canManage: boolean
  selectedNode: ProjectNode | null
  subtasks: Subtask[]
}>()

defineEmits<{
  create: []
  edit: [subtaskId: number]
  history: [subtaskId: number]
  progress: [subtaskId: number]
  remove: [subtaskId: number]
  reopen: [subtaskId: number]
  start: [subtaskId: number]
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
</script>

<template>
  <section class="subtask-table">
    <header class="subtask-table__header">
      <div>
        <p>节点任务</p>
        <span v-if="selectedNode">{{ selectedNode.name }}</span>
      </div>
      <div class="subtask-table__header-actions">
        <strong>{{ subtasks.length }} 项</strong>
        <button v-if="canManage" type="button" @click="$emit('create')">新建子任务</button>
      </div>
    </header>

    <article v-for="subtask in subtasks" :key="subtask.id" class="subtask-table__row">
      <div>
        <strong>{{ subtask.name }}</strong>
        <p>{{ subtask.description }}</p>
      </div>

      <div class="subtask-table__metrics">
        <span>{{ subtask.responsible_real_name || '未分配' }}</span>
        <span>{{ priorityLabel(subtask.priority) }}优先级</span>
        <span>{{ subtask.progress_percent }}%</span>
      </div>

      <div class="subtask-table__actions">
        <button
          :data-testid="`subtask-progress-${subtask.id}`"
          type="button"
          @click="$emit('progress', subtask.id)"
        >
          更新进度
        </button>
        <button
          :data-testid="`subtask-history-${subtask.id}`"
          type="button"
          @click="$emit('history', subtask.id)"
        >
          历史记录
        </button>
        <button v-if="canManage" type="button" @click="$emit('edit', subtask.id)">编辑</button>
        <button v-if="canManage" type="button" @click="$emit('start', subtask.id)">开始</button>
        <button v-if="canManage" type="button" @click="$emit('reopen', subtask.id)">撤销完成</button>
        <button v-if="canManage" type="button" @click="$emit('remove', subtask.id)">删除</button>
      </div>
    </article>

    <p v-if="subtasks.length === 0" class="subtask-table__empty">当前节点下还没有子任务。</p>
  </section>
</template>

<style scoped>
.subtask-table {
  display: grid;
  gap: 12px;
  padding: 18px;
  border: 1px solid var(--border-soft);
  border-radius: 18px;
  background: var(--glass-bg);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
}

.subtask-table__header {
  display: flex;
  align-items: start;
  justify-content: space-between;
  gap: 16px;
}

.subtask-table__header-actions {
  display: grid;
  justify-items: end;
  gap: 10px;
}

.subtask-table__header p,
.subtask-table__header span {
  margin: 0;
}

.subtask-table__header p {
  font-weight: 700;
}

.subtask-table__header span {
  color: var(--text-soft);
}

.subtask-table__row {
  display: grid;
  gap: 10px;
  padding: 14px;
  border: 1px solid var(--border-soft);
  border-radius: 14px;
  background: color-mix(in srgb, var(--panel-bg) 88%, transparent);
}

.subtask-table__row strong,
.subtask-table__row p {
  margin: 0;
}

.subtask-table__row p {
  color: var(--text-soft);
}

.subtask-table__metrics {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  color: var(--text-soft);
  font-size: 0.92rem;
}

.subtask-table__actions {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

button {
  border: 1px solid var(--border-soft);
  border-radius: 10px;
  padding: 8px 12px;
  background: color-mix(in srgb, var(--accent-start) 12%, transparent);
  color: var(--text-main);
  cursor: pointer;
}

.subtask-table__empty {
  margin: 0;
  color: var(--text-soft);
}
</style>
