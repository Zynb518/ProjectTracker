<script setup lang="ts">
import type { SubtaskProgressRecord } from '@/types/subtask'

defineProps<{
  modelValue: boolean
  records: SubtaskProgressRecord[]
  title: string
}>()

defineEmits<{
  'update:modelValue': [value: boolean]
}>()
</script>

<template>
  <aside v-if="modelValue" class="history-drawer">
    <header>
      <div>
        <p>进度历史</p>
        <h2>{{ title }}</h2>
      </div>
      <button type="button" @click="$emit('update:modelValue', false)">关闭</button>
    </header>

    <article v-for="record in records" :key="record.id" class="history-drawer__item">
      <strong>{{ record.operator_real_name || `用户 ${record.operator_user_id}` }}</strong>
      <p>{{ record.progress_note || '无补充说明' }}</p>
      <span>{{ record.progress_percent }}% · 状态 {{ record.status }}</span>
    </article>
  </aside>
</template>

<style scoped>
.history-drawer {
  position: fixed;
  top: 0;
  right: 0;
  bottom: 0;
  width: min(100%, 420px);
  display: grid;
  align-content: start;
  gap: 12px;
  padding: 24px;
  border-left: 1px solid var(--border-soft);
  background: var(--glass-bg-strong);
  box-shadow: -24px 0 48px rgba(0, 0, 0, 0.22);
  backdrop-filter: var(--backdrop-blur);
}

header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

header h2,
header p {
  margin: 0;
}

.history-drawer__item {
  display: grid;
  gap: 8px;
  padding: 14px;
  border: 1px solid var(--border-soft);
  border-radius: 14px;
  background: color-mix(in srgb, var(--panel-bg) 88%, transparent);
}

.history-drawer__item p,
.history-drawer__item span {
  margin: 0;
  color: var(--text-soft);
}

button {
  border: 1px solid var(--border-soft);
  border-radius: 10px;
  padding: 10px 14px;
  background: color-mix(in srgb, var(--panel-bg) 88%, transparent);
  color: var(--text-main);
  cursor: pointer;
}
</style>
