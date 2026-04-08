<script setup lang="ts">
import { reactive, watch } from 'vue'

import type { ProjectFormPayload } from '@/types/project'

const props = defineProps<{
  modelValue: boolean
  mode: 'create' | 'edit'
  initialValue: ProjectFormPayload
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  submit: [payload: ProjectFormPayload]
}>()

const form = reactive<ProjectFormPayload>({
  name: '',
  description: '',
  planned_start_date: '',
  planned_end_date: '',
})

watch(
  () => props.initialValue,
  (value) => {
    form.name = value.name
    form.description = value.description
    form.planned_start_date = value.planned_start_date
    form.planned_end_date = value.planned_end_date
  },
  { immediate: true, deep: true },
)

function close() {
  emit('update:modelValue', false)
}

function submit() {
  emit('submit', {
    name: form.name,
    description: form.description,
    planned_start_date: form.planned_start_date,
    planned_end_date: form.planned_end_date,
  })
}
</script>

<template>
  <div v-if="modelValue" class="project-dialog__backdrop">
    <section class="project-dialog">
      <header class="project-dialog__header">
        <div>
          <p class="project-dialog__eyebrow">项目操作</p>
          <h2>{{ mode === 'create' ? '新建项目' : '编辑项目' }}</h2>
        </div>

        <button type="button" @click="close">关闭</button>
      </header>

      <div class="project-dialog__grid">
        <label>
          <span>项目名称</span>
          <input v-model="form.name" type="text" />
        </label>

        <label>
          <span>项目描述</span>
          <textarea v-model="form.description" rows="4" />
        </label>

        <label>
          <span>计划开始</span>
          <input v-model="form.planned_start_date" type="date" />
        </label>

        <label>
          <span>计划结束</span>
          <input v-model="form.planned_end_date" type="date" />
        </label>
      </div>

      <footer class="project-dialog__footer">
        <button type="button" @click="close">取消</button>
        <button type="button" class="primary" @click="submit">
          {{ mode === 'create' ? '创建项目' : '保存修改' }}
        </button>
      </footer>
    </section>
  </div>
</template>

<style scoped>
.project-dialog__backdrop {
  position: fixed;
  inset: 0;
  display: grid;
  place-items: center;
  padding: 24px;
  background: rgba(10, 14, 23, 0.38);
  backdrop-filter: blur(16px);
}

.project-dialog {
  width: min(100%, 640px);
  display: grid;
  gap: 18px;
  padding: 24px;
  border: 1px solid var(--border-soft);
  border-radius: 16px;
  background: var(--glass-bg-strong);
  box-shadow: var(--shadow-glass);
  backdrop-filter: var(--backdrop-blur);
}

.project-dialog__header,
.project-dialog__footer {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.project-dialog__header h2,
.project-dialog__header p {
  margin: 0;
}

.project-dialog__eyebrow {
  margin-bottom: 6px;
  letter-spacing: 0.14em;
  text-transform: uppercase;
  color: var(--text-soft);
  font-size: 0.76rem;
}

.project-dialog__grid {
  display: grid;
  gap: 14px;
}

label {
  display: grid;
  gap: 8px;
}

span {
  font-size: 0.78rem;
  font-weight: 600;
  letter-spacing: 0.08em;
  text-transform: uppercase;
  color: var(--text-soft);
}

input,
textarea {
  border: 1px solid var(--border-soft);
  border-radius: 8px;
  padding: 12px 14px;
  background: color-mix(in srgb, var(--panel-bg) 92%, transparent);
  color: var(--text-main);
  font: inherit;
}

input:focus,
textarea:focus {
  border-color: var(--accent-line);
  box-shadow: 0 0 0 4px rgba(10, 102, 255, 0.12);
}

button {
  border: 1px solid var(--border-soft);
  border-radius: 10px;
  padding: 10px 16px;
  background: color-mix(in srgb, var(--panel-bg) 88%, transparent);
  color: var(--text-main);
  cursor: pointer;
}

.primary {
  border: none;
  background: var(--gradient-primary);
  color: var(--text-inverse);
  box-shadow: 0 16px 28px rgba(10, 102, 255, 0.22);
}
</style>
