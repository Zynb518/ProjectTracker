<script setup lang="ts">
defineProps<{
  projectId: string
  status: string
}>()

defineEmits<{
  'update:projectId': [value: string]
  'update:status': [value: string]
  submit: []
}>()
</script>

<template>
  <section class="my-task-filters">
    <label>
      <span>项目 ID</span>
      <input
        :value="projectId"
        placeholder="例如 1001"
        type="text"
        @input="$emit('update:projectId', ($event.target as HTMLInputElement).value)"
      />
    </label>

    <label>
      <span>任务状态</span>
      <select
        :value="status"
        data-testid="my-tasks-status"
        @change="$emit('update:status', ($event.target as HTMLSelectElement).value)"
      >
        <option value="">全部状态</option>
        <option value="1">未开始</option>
        <option value="2">进行中</option>
        <option value="3">已完成</option>
        <option value="4">已延期</option>
      </select>
    </label>

    <button type="button" @click="$emit('submit')">刷新任务</button>
  </section>
</template>

<style scoped>
.my-task-filters {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 220px)) auto;
  gap: 14px;
  padding: 18px;
  border: 1px solid var(--border-soft);
  border-radius: 18px;
  background: var(--glass-bg);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
}

label {
  display: grid;
  gap: 8px;
}

span {
  font-size: 0.78rem;
  letter-spacing: 0.12em;
  text-transform: uppercase;
  color: var(--text-soft);
}

input,
select {
  min-height: 46px;
  border: 1px solid var(--border-soft);
  border-radius: 8px;
  padding: 0 14px;
  background: color-mix(in srgb, var(--panel-bg) 92%, transparent);
  color: var(--text-main);
  font: inherit;
}

button {
  align-self: end;
  border: none;
  border-radius: 10px;
  padding: 12px 16px;
  background: var(--gradient-primary);
  color: var(--text-inverse);
  cursor: pointer;
  box-shadow: 0 16px 28px rgba(10, 102, 255, 0.22);
}

@media (max-width: 820px) {
  .my-task-filters {
    grid-template-columns: 1fr;
  }
}
</style>
