<script setup lang="ts">
import SubtaskTable from '@/components/subtasks/SubtaskTable.vue'
import type { ProjectNode } from '@/types/node'
import type { Subtask } from '@/types/subtask'

withDefaults(defineProps<{
  canManage: boolean
  embedded?: boolean
  isLoading: boolean
  node: ProjectNode
  subtasks: Subtask[]
}>(), {
  embedded: false,
})

defineEmits<{
  create: []
  edit: [subtaskId: number]
  history: [subtaskId: number]
  progress: [subtaskId: number]
  remove: [subtaskId: number]
  reopen: [subtaskId: number]
}>()

function statusLabel(status: number) {
  return (
    {
      1: '未开始',
      2: '进行中',
      3: '已完成',
      4: '已延期',
    }[status] ?? '未知状态'
  )
}

function statusTone(status: number) {
  return (
    {
      1: 'pending',
      2: 'active',
      3: 'done',
      4: 'delayed',
    }[status] ?? 'unknown'
  )
}

function progressPercent(node: ProjectNode) {
  if (node.sub_task_count === 0) {
    return 0
  }

  return Math.round((node.completed_sub_task_count / node.sub_task_count) * 100)
}
</script>

<template>
  <section
    :class="['node-drawer', { 'node-drawer--embedded': embedded }]"
    data-testid="node-drawer"
  >
    <header class="node-drawer__hero">
      <div class="node-drawer__eyebrow">
        <span>阶段详情</span>
        <span class="node-drawer__phase">Phase {{ node.sequence_no }}</span>
      </div>

      <div class="node-drawer__title-row">
        <div>
          <h3>{{ node.name }}</h3>
          <p>{{ node.description || '当前阶段暂无补充说明。' }}</p>
        </div>
        <span :class="['node-drawer__status', `node-drawer__status--${statusTone(node.status)}`]">
          {{ statusLabel(node.status) }}
        </span>
      </div>

      <dl class="node-drawer__metrics">
        <div>
          <dt>计划周期</dt>
          <dd>{{ node.planned_start_date }} - {{ node.planned_end_date }}</dd>
        </div>
        <div>
          <dt>子任务完成</dt>
          <dd>{{ node.completed_sub_task_count }} / {{ node.sub_task_count }}</dd>
        </div>
        <div>
          <dt>完成率</dt>
          <dd>{{ progressPercent(node) }}%</dd>
        </div>
      </dl>
    </header>

    <div class="node-drawer__content" data-testid="node-drawer-content">
      <p v-if="isLoading" class="node-drawer__state loading-panel">节点任务加载中...</p>

      <SubtaskTable
        v-else
        :can-manage="canManage"
        :embedded="embedded"
        :selected-node="node"
        :subtasks="subtasks"
        @create="$emit('create')"
        @edit="$emit('edit', $event)"
        @history="$emit('history', $event)"
        @progress="$emit('progress', $event)"
        @remove="$emit('remove', $event)"
        @reopen="$emit('reopen', $event)"
      />
    </div>
  </section>
</template>

<style scoped>
.node-drawer {
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
  gap: 20px;
  min-height: 0;
  height: 100%;
  padding: 22px;
  border: 1px solid var(--border-soft);
  border-radius: 24px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--glass-bg) 96%, transparent), var(--glass-bg)),
    radial-gradient(circle at top right, color-mix(in srgb, var(--accent-start) 14%, transparent), transparent 34%);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
}

.node-drawer--embedded {
  padding: 0;
  border: none;
  border-radius: 0;
  background: none;
  box-shadow: none;
  backdrop-filter: none;
}

.node-drawer__hero {
  display: grid;
  gap: 18px;
  padding-bottom: 18px;
  border-bottom: 1px solid var(--border-soft);
}

.node-drawer__eyebrow {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  color: var(--text-soft);
  font-size: 0.82rem;
  font-weight: 600;
  letter-spacing: 0.08em;
  text-transform: uppercase;
}

.node-drawer__phase {
  padding: 7px 12px;
  border: 1px solid var(--border-soft);
  border-radius: 999px;
  background: color-mix(in srgb, var(--panel-bg) 86%, transparent);
}

.node-drawer__title-row {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 18px;
}

.node-drawer__title-row h3,
.node-drawer__title-row p,
.node-drawer__metrics dt,
.node-drawer__metrics dd {
  margin: 0;
}

.node-drawer__title-row h3 {
  font-size: clamp(1.35rem, 2vw, 1.8rem);
  line-height: 1.2;
}

.node-drawer__title-row p {
  margin-top: 8px;
  color: var(--text-soft);
  line-height: 1.7;
}

.node-drawer__status {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  padding: 8px 14px;
  border-radius: 999px;
  font-size: 0.82rem;
  font-weight: 700;
  white-space: nowrap;
}

.node-drawer__status--pending {
  background: color-mix(in srgb, var(--text-soft) 18%, transparent);
  color: var(--text-soft);
}

.node-drawer__status--active {
  background: color-mix(in srgb, var(--accent-start) 18%, transparent);
  color: var(--accent-start);
}

.node-drawer__status--done {
  background: color-mix(in srgb, var(--text-soft) 18%, transparent);
  color: color-mix(in srgb, var(--text-main) 66%, var(--text-soft));
}

.node-drawer__status--delayed {
  background: color-mix(in srgb, var(--accent-warning) 18%, transparent);
  color: var(--accent-warning);
}

.node-drawer__status--unknown {
  background: color-mix(in srgb, var(--text-soft) 18%, transparent);
  color: var(--text-soft);
}

.node-drawer__metrics {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 12px;
}

.node-drawer__metrics div {
  display: grid;
  gap: 6px;
  padding: 12px 14px;
  border: 1px solid var(--border-soft);
  border-radius: 16px;
  background: color-mix(in srgb, var(--panel-bg) 84%, transparent);
}

.node-drawer__metrics dt {
  color: var(--text-soft);
  font-size: 0.76rem;
}

.node-drawer__metrics dd {
  font-size: 0.96rem;
  font-weight: 700;
  line-height: 1.5;
}

.node-drawer__content {
  min-width: 0;
  min-height: 0;
  overflow-y: auto;
  padding-right: 8px;
}

.node-drawer__state {
  margin: 0;
}

@media (max-width: 860px) {
  .node-drawer__title-row {
    display: grid;
  }

  .node-drawer__metrics {
    grid-template-columns: 1fr;
  }
}
</style>
