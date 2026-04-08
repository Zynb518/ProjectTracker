<script setup lang="ts">
import type { ProjectNode } from '@/types/node'

defineProps<{
  nodes: ProjectNode[]
  selectedNodeId: number | null
}>()

defineEmits<{
  select: [nodeId: number]
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
</script>

<template>
  <section class="node-rail">
    <header class="node-rail__header">
      <p>阶段节点</p>
      <span>{{ nodes.length }} 个节点</span>
    </header>

    <button
      v-for="node in nodes"
      :key="node.id"
      class="node-rail__item"
      :class="{ 'is-active': node.id === selectedNodeId }"
      type="button"
      @click="$emit('select', node.id)"
    >
      <strong>{{ node.sequence_no }}. {{ node.name }}</strong>
      <span>{{ statusLabel(node.status) }}</span>
      <small>{{ node.completed_sub_task_count }} / {{ node.sub_task_count }} 子任务完成</small>
    </button>
  </section>
</template>

<style scoped>
.node-rail {
  display: grid;
  gap: 12px;
  padding: 18px;
  border: 1px solid var(--border-soft);
  border-radius: 18px;
  background: var(--glass-bg);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
}

.node-rail__header {
  display: flex;
  align-items: baseline;
  justify-content: space-between;
  gap: 12px;
}

.node-rail__header p,
.node-rail__header span {
  margin: 0;
}

.node-rail__header p {
  font-weight: 700;
}

.node-rail__header span {
  color: var(--text-soft);
  font-size: 0.85rem;
}

.node-rail__item {
  display: grid;
  gap: 6px;
  text-align: left;
  border: 1px solid var(--border-soft);
  border-radius: 14px;
  padding: 14px;
  background: color-mix(in srgb, var(--panel-bg) 88%, transparent);
  cursor: pointer;
}

.node-rail__item.is-active {
  border-color: var(--accent-line);
  background: var(--gradient-primary-soft);
  box-shadow: var(--shadow-panel-hover);
  transform: translateY(-2px);
}

small,
span {
  color: var(--text-soft);
}
</style>
