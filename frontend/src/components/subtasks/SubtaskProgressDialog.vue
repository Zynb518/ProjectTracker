<script setup lang="ts">
import { reactive, watch } from 'vue'

import type { Subtask } from '@/types/subtask'

const props = defineProps<{
  modelValue: boolean
  subtask: Subtask | null
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  submit: [payload: { subtaskId: number; status: number; progress_percent: number; progress_note: string }]
}>()

const form = reactive({
  status: '2',
  progress_percent: 0,
  progress_note: '',
})

watch(
  () => props.subtask,
  (value) => {
    form.status = String(value?.status ?? 2)
    form.progress_percent = value?.progress_percent ?? 0
    form.progress_note = ''
  },
  { immediate: true },
)

function close() {
  emit('update:modelValue', false)
}

function submit() {
  if (!props.subtask) {
    return
  }

  emit('submit', {
    subtaskId: props.subtask.id,
    status: Number(form.status),
    progress_percent: form.progress_percent,
    progress_note: form.progress_note,
  })
}
</script>

<template>
  <div v-if="modelValue && subtask" class="subtask-dialog__backdrop">
    <section class="subtask-dialog">
      <header>
        <div>
          <p>任务进度</p>
          <h2>{{ subtask.name }}</h2>
        </div>
        <button type="button" @click="close">关闭</button>
      </header>

      <label>
        <span>状态</span>
        <select v-model="form.status">
          <option value="1">未开始</option>
          <option value="2">进行中</option>
          <option value="3">已完成</option>
          <option value="4">已延期</option>
        </select>
      </label>

      <label>
        <span>进度百分比</span>
        <input v-model.number="form.progress_percent" max="100" min="0" step="10" type="number" />
      </label>

      <label>
        <span>进度说明</span>
        <textarea v-model="form.progress_note" rows="4" />
      </label>

      <footer>
        <button type="button" @click="close">取消</button>
        <button type="button" class="primary" @click="submit">提交进度</button>
      </footer>
    </section>
  </div>
</template>

<style scoped>
.subtask-dialog__backdrop {
  position: fixed;
  inset: 0;
  display: grid;
  place-items: center;
  padding: 24px;
  background: rgba(10, 14, 23, 0.38);
  backdrop-filter: blur(16px);
}

.subtask-dialog {
  width: min(100%, 560px);
  display: grid;
  gap: 16px;
  padding: 24px;
  border: 1px solid var(--border-soft);
  border-radius: 16px;
  background: var(--glass-bg-strong);
  box-shadow: var(--shadow-glass);
  backdrop-filter: var(--backdrop-blur);
}

header,
footer {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

header h2,
header p {
  margin: 0;
}

label {
  display: grid;
  gap: 8px;
}

span {
  font-size: 0.8rem;
  letter-spacing: 0.12em;
  text-transform: uppercase;
  color: var(--text-soft);
}

input,
select,
textarea,
button {
  border-radius: 8px;
  padding: 10px 14px;
  font: inherit;
}

input,
select,
textarea {
  border: 1px solid var(--border-soft);
  background: color-mix(in srgb, var(--panel-bg) 92%, transparent);
  color: var(--text-main);
}

button {
  border: 1px solid var(--border-soft);
  background: color-mix(in srgb, var(--panel-bg) 88%, transparent);
  color: var(--text-main);
}

.primary {
  border: none;
  background: var(--gradient-primary);
  color: var(--text-inverse);
}
</style>
