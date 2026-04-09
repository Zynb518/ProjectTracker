<script setup lang="ts">
import type { SubtaskProgressRecord } from '@/types/subtask'

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
}>()

defineProps<{
  modelValue: boolean
  records: SubtaskProgressRecord[]
  title: string
}>()
</script>

<template>
  <Transition appear name="history-drawer-slide">
    <div
      v-if="modelValue"
      class="history-drawer__backdrop"
      @click.self="emit('update:modelValue', false)"
    >
      <aside class="history-drawer">
        <header>
          <div>
            <p>进度历史</p>
            <h2>{{ title }}</h2>
          </div>
          <button type="button" @click="emit('update:modelValue', false)">关闭</button>
        </header>

        <article v-for="record in records" :key="record.id" class="history-drawer__item">
          <strong>{{ record.operator_real_name || `用户 ${record.operator_user_id}` }}</strong>
          <p>{{ record.progress_note || '无补充说明' }}</p>
          <span>{{ record.progress_percent }}% · 状态 {{ record.status }}</span>
        </article>
      </aside>
    </div>
  </Transition>
</template>

<style scoped>
.history-drawer-slide-enter-active,
.history-drawer-slide-leave-active {
  transition: opacity 240ms ease-out;
}

.history-drawer-slide-enter-active .history-drawer,
.history-drawer-slide-leave-active .history-drawer {
  transition:
    transform 320ms cubic-bezier(0.16, 1, 0.3, 1),
    opacity 240ms ease-out;
}

.history-drawer-slide-enter-from,
.history-drawer-slide-leave-to {
  opacity: 0;
}

.history-drawer-slide-enter-from .history-drawer,
.history-drawer-slide-leave-to .history-drawer {
  opacity: 0;
  transform: translateX(24px);
}

.history-drawer__backdrop {
  position: fixed;
  inset: 0;
  z-index: 42;
  display: flex;
  justify-content: flex-end;
  padding-left: 40px;
  background:
    linear-gradient(90deg, rgba(10, 14, 23, 0), rgba(10, 14, 23, 0.14) 22%, rgba(10, 14, 23, 0.28));
  backdrop-filter: blur(6px);
}

.history-drawer {
  height: 100%;
  width: min(100%, 420px);
  display: grid;
  align-content: start;
  gap: 12px;
  padding: 24px;
  border-left: 1px solid var(--border-soft);
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--glass-bg-strong) 96%, transparent), var(--glass-bg-strong)),
    radial-gradient(circle at top left, color-mix(in srgb, var(--accent-end) 12%, transparent), transparent 32%);
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
