<script setup lang="ts">
import { computed, reactive, watch } from 'vue'

import type { ProjectFormPayload } from '@/types/project'

type DialogMotionOrigin = {
  x: number
  y: number
  translateX: number
  translateY: number
}

const props = defineProps<{
  modelValue: boolean
  mode: 'create' | 'edit'
  initialValue: ProjectFormPayload
  motionOrigin?: DialogMotionOrigin | null
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

const backdropStyle = computed(() => {
  if (!props.motionOrigin) {
    return undefined
  }

  return {
    '--project-dialog-origin-x': `${props.motionOrigin.x}px`,
    '--project-dialog-origin-y': `${props.motionOrigin.y}px`,
    '--project-dialog-translate-x': `${props.motionOrigin.translateX}px`,
    '--project-dialog-translate-y': `${props.motionOrigin.translateY}px`,
  }
})

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
  <Transition appear name="project-dialog-fade">
    <div v-if="modelValue" :style="backdropStyle" class="project-dialog__backdrop">
      <div aria-hidden="true" class="project-dialog__glow" />

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
  </Transition>
</template>

<style scoped>
.project-dialog-fade-enter-active,
.project-dialog-fade-leave-active {
  transition:
    opacity 420ms ease-out,
    backdrop-filter 420ms ease-out,
    background-color 420ms ease-out;
}

.project-dialog-fade-enter-active .project-dialog,
.project-dialog-fade-leave-active .project-dialog {
  transition:
    opacity 420ms cubic-bezier(0.16, 1, 0.3, 1),
    transform 460ms cubic-bezier(0.16, 1, 0.3, 1),
    box-shadow 460ms ease-out,
    filter 420ms ease-out;
}

.project-dialog-fade-enter-active .project-dialog__glow,
.project-dialog-fade-leave-active .project-dialog__glow {
  transition:
    opacity 520ms ease-out,
    transform 560ms cubic-bezier(0.16, 1, 0.3, 1),
    filter 520ms ease-out;
}

.project-dialog-fade-enter-from,
.project-dialog-fade-leave-to {
  opacity: 0;
  backdrop-filter: blur(0px);
}

.project-dialog-fade-enter-from .project-dialog__glow,
.project-dialog-fade-leave-to .project-dialog__glow {
  opacity: 0;
  transform: translate(-50%, -50%) scale(0.68);
  filter: blur(34px);
}

.project-dialog-fade-enter-from .project-dialog,
.project-dialog-fade-leave-to .project-dialog {
  opacity: 0;
  transform:
    perspective(1120px)
    translate3d(var(--project-dialog-translate-x), var(--project-dialog-translate-y), 0)
    scale(0.72)
    rotateX(4deg);
  filter: blur(6px);
}

.project-dialog__backdrop {
  --project-dialog-origin-x: 50vw;
  --project-dialog-origin-y: 28vh;
  --project-dialog-translate-x: 0px;
  --project-dialog-translate-y: 22px;
  position: fixed;
  inset: 0;
  display: grid;
  place-items: center;
  isolation: isolate;
  padding: 24px;
  overflow: hidden;
  background:
    radial-gradient(circle at 18% 22%, rgba(10, 102, 255, 0.12), transparent 22%),
    radial-gradient(circle at 82% 78%, rgba(0, 194, 255, 0.14), transparent 24%),
    rgba(10, 14, 23, 0.42);
  backdrop-filter: blur(16px);
}

.project-dialog__backdrop::before {
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

.project-dialog__glow {
  position: absolute;
  left: var(--project-dialog-origin-x);
  top: var(--project-dialog-origin-y);
  width: min(56vw, 520px);
  height: min(56vw, 520px);
  border-radius: 999px;
  background:
    radial-gradient(circle, rgba(0, 194, 255, 0.28) 0%, rgba(10, 102, 255, 0.18) 38%, transparent 72%);
  filter: blur(18px);
  opacity: 0.78;
  transform: translate(-50%, -50%);
  pointer-events: none;
  animation: project-dialog-glow-drift 7.8s cubic-bezier(0.45, 0, 0.2, 1) infinite alternate;
}

.project-dialog {
  position: relative;
  overflow: hidden;
  transform-origin: var(--project-dialog-origin-x) var(--project-dialog-origin-y);
  width: min(100%, 640px);
  display: grid;
  gap: 18px;
  padding: 24px;
  border: 1px solid var(--border-soft);
  border-radius: 16px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--glass-bg-strong) 96%, transparent), var(--glass-bg-strong)),
    radial-gradient(circle at top right, rgba(0, 194, 255, 0.14), transparent 34%);
  box-shadow:
    0 24px 56px rgba(3, 10, 24, 0.32),
    0 0 0 1px rgba(255, 255, 255, 0.04),
    0 0 42px rgba(10, 102, 255, 0.08);
  backdrop-filter: var(--backdrop-blur);
}

.project-dialog::before {
  content: '';
  position: absolute;
  inset: 0;
  border-radius: inherit;
  border: 1px solid rgba(255, 255, 255, 0.08);
  background:
    linear-gradient(135deg, rgba(255, 255, 255, 0.18), transparent 34%),
    linear-gradient(180deg, transparent, rgba(0, 194, 255, 0.05));
  pointer-events: none;
}

.project-dialog::after {
  content: '';
  position: absolute;
  inset: 0 auto auto 0;
  width: 42%;
  height: 1px;
  background: linear-gradient(90deg, rgba(0, 194, 255, 0.84), transparent);
  box-shadow: 0 0 16px rgba(0, 194, 255, 0.4);
  pointer-events: none;
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

@keyframes project-dialog-glow-drift {
  from {
    transform: translate(calc(-50% - 10px), calc(-50% - 6px)) scale(0.99);
  }

  to {
    transform: translate(calc(-50% + 12px), calc(-50% + 8px)) scale(1.02);
  }
}
</style>
