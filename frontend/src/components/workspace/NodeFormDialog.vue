<script setup lang="ts">
import { reactive, watch } from 'vue'

import type { ProjectNodePayload } from '@/types/node'

const props = defineProps<{
  initialValue: ProjectNodePayload
  maxDate?: string
  minDate?: string
  mode: 'create' | 'edit'
  modelValue: boolean
}>()

const emit = defineEmits<{
  submit: [payload: ProjectNodePayload]
  'update:modelValue': [value: boolean]
}>()

const form = reactive<ProjectNodePayload>({
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
  <Transition appear name="node-dialog-fade">
    <div v-if="modelValue" class="node-dialog__backdrop">
      <section class="node-dialog">
        <header class="node-dialog__header">
          <div>
            <p class="node-dialog__eyebrow">阶段节点</p>
            <h2>{{ mode === 'create' ? '新建节点' : '编辑节点' }}</h2>
          </div>

          <button type="button" @click="close">关闭</button>
        </header>

        <div class="node-dialog__grid">
          <label>
            <span>节点名称</span>
            <input v-model="form.name" aria-label="节点名称" type="text" />
          </label>

          <label class="node-dialog__description">
            <span>节点说明</span>
            <textarea v-model="form.description" aria-label="节点说明" rows="4" />
          </label>

          <label>
            <span>计划开始</span>
            <input
              v-model="form.planned_start_date"
              :max="maxDate"
              :min="minDate"
              aria-label="计划开始"
              type="date"
            />
          </label>

          <label>
            <span>计划结束</span>
            <input
              v-model="form.planned_end_date"
              :max="maxDate"
              :min="minDate"
              aria-label="计划结束"
              type="date"
            />
          </label>
        </div>

        <footer class="node-dialog__footer">
          <button type="button" @click="close">取消</button>
          <button type="button" class="primary" @click="submit">
            {{ mode === 'create' ? '创建节点' : '保存节点' }}
          </button>
        </footer>
      </section>
    </div>
  </Transition>
</template>

<style scoped>
.node-dialog-fade-enter-active,
.node-dialog-fade-leave-active {
  transition:
    opacity 260ms ease-out,
    backdrop-filter 260ms ease-out;
}

.node-dialog-fade-enter-active .node-dialog,
.node-dialog-fade-leave-active .node-dialog {
  transition:
    opacity 280ms cubic-bezier(0.16, 1, 0.3, 1),
    transform 320ms cubic-bezier(0.16, 1, 0.3, 1);
}

.node-dialog-fade-enter-from,
.node-dialog-fade-leave-to {
  opacity: 0;
  backdrop-filter: blur(0px);
}

.node-dialog-fade-enter-from .node-dialog,
.node-dialog-fade-leave-to .node-dialog {
  opacity: 0;
  transform: translateY(18px) scale(0.96);
}

.node-dialog__backdrop {
  position: fixed;
  inset: 0;
  z-index: 40;
  display: grid;
  place-items: center;
  padding: 24px;
  background:
    radial-gradient(circle at 18% 22%, rgba(10, 102, 255, 0.12), transparent 22%),
    radial-gradient(circle at 82% 78%, rgba(0, 194, 255, 0.14), transparent 24%),
    rgba(10, 14, 23, 0.42);
  backdrop-filter: blur(16px);
}

.node-dialog__backdrop::before {
  content: '';
  position: absolute;
  inset: 0;
  background:
    linear-gradient(rgba(255, 255, 255, 0.03) 1px, transparent 1px),
    linear-gradient(90deg, rgba(255, 255, 255, 0.03) 1px, transparent 1px);
  background-size: 28px 28px;
  opacity: 0.18;
  pointer-events: none;
}

.node-dialog {
  position: relative;
  width: min(100%, 620px);
  display: grid;
  gap: 18px;
  padding: 24px;
  border: 1px solid var(--border-soft);
  border-radius: 20px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--glass-bg-strong) 96%, transparent), var(--glass-bg-strong)),
    radial-gradient(circle at top right, rgba(0, 194, 255, 0.14), transparent 34%);
  box-shadow:
    0 24px 56px rgba(3, 10, 24, 0.32),
    0 0 0 1px rgba(255, 255, 255, 0.04),
    0 0 42px rgba(10, 102, 255, 0.08);
  backdrop-filter: var(--backdrop-blur);
}

.node-dialog__header,
.node-dialog__footer {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
}

.node-dialog__header h2,
.node-dialog__header p {
  margin: 0;
}

.node-dialog__eyebrow {
  color: var(--text-soft);
  font-size: 0.78rem;
  font-weight: 600;
  letter-spacing: 0.14em;
  text-transform: uppercase;
}

.node-dialog__grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 16px;
}

.node-dialog__description {
  grid-column: 1 / -1;
}

.node-dialog__grid label {
  display: grid;
  gap: 8px;
}

.node-dialog__grid span {
  color: var(--text-soft);
  font-size: 0.88rem;
  font-weight: 600;
}

.node-dialog__grid input,
.node-dialog__grid textarea,
.node-dialog__header button,
.node-dialog__footer button {
  border: 1px solid var(--border-soft);
  border-radius: 12px;
  padding: 11px 14px;
  font: inherit;
}

.node-dialog__grid input,
.node-dialog__grid textarea {
  background: color-mix(in srgb, var(--panel-bg) 92%, transparent);
  color: var(--text-main);
}

.node-dialog__grid textarea {
  resize: vertical;
  min-height: 116px;
}

.node-dialog__header button,
.node-dialog__footer button {
  background: color-mix(in srgb, var(--panel-bg) 88%, transparent);
  color: var(--text-main);
  cursor: pointer;
}

.node-dialog__footer .primary {
  border-color: color-mix(in srgb, var(--accent-start) 28%, transparent);
  background: linear-gradient(135deg, var(--accent-start), var(--accent-end));
  color: #fff;
}

@media (max-width: 720px) {
  .node-dialog__grid {
    grid-template-columns: 1fr;
  }
}
</style>
