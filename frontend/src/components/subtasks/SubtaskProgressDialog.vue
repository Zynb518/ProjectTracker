<script setup lang="ts">
import { computed, reactive, watch } from 'vue'

import type { Subtask } from '@/types/subtask'

const props = defineProps<{
  modelValue: boolean
  subtask: Subtask | null
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  submit: [payload: { subtaskId: number; progress_percent: number; progress_note: string }]
}>()

const form = reactive({
  progress_percent: 0,
  progress_note: '',
})

const currentProgress = computed(() => props.subtask?.progress_percent ?? 0)

function normalizeProgress(value: number) {
  if (Number.isNaN(value)) {
    return currentProgress.value
  }

  return Math.max(currentProgress.value, Math.min(100, value))
}

watch(
  () => props.subtask,
  (value) => {
    form.progress_percent = value?.progress_percent ?? 0
    form.progress_note = ''
  },
  { immediate: true },
)

watch(
  () => form.progress_percent,
  (value) => {
    const nextValue = normalizeProgress(value)

    if (nextValue !== value) {
      form.progress_percent = nextValue
    }
  },
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
    progress_percent: normalizeProgress(form.progress_percent),
    progress_note: form.progress_note,
  })
}
</script>

<template>
  <Transition appear name="subtask-dialog-fade">
    <div v-if="modelValue && subtask" class="subtask-dialog__backdrop">
      <section class="subtask-dialog">
        <header>
          <div>
            <p>任务进度</p>
            <h2>{{ subtask.name }}</h2>
          </div>
          <button type="button" @click="close">关闭</button>
        </header>

        <label class="subtask-dialog__progress">
          <div class="subtask-dialog__progress-header">
            <span>进度百分比</span>
            <strong>{{ form.progress_percent }}%</strong>
          </div>

          <input
            v-model.number="form.progress_percent"
            :min="currentProgress"
            aria-label="进度百分比"
            data-testid="subtask-progress-range"
            max="100"
            step="10"
            type="range"
          />

          <div class="subtask-dialog__progress-scale">
            <small>当前 {{ currentProgress }}%</small>
            <small>100%</small>
          </div>
        </label>

        <label>
          <span>进度说明</span>
          <textarea v-model="form.progress_note" aria-label="进度说明" rows="4" />
        </label>

        <footer>
          <button type="button" @click="close">取消</button>
          <button type="button" class="primary" @click="submit">提交进度</button>
        </footer>
      </section>
    </div>
  </Transition>
</template>

<style scoped>
.subtask-dialog-fade-enter-active,
.subtask-dialog-fade-leave-active {
  transition:
    opacity 260ms ease-out,
    backdrop-filter 260ms ease-out;
}

.subtask-dialog-fade-enter-active .subtask-dialog,
.subtask-dialog-fade-leave-active .subtask-dialog {
  transition:
    opacity 280ms cubic-bezier(0.16, 1, 0.3, 1),
    transform 320ms cubic-bezier(0.16, 1, 0.3, 1);
}

.subtask-dialog-fade-enter-from,
.subtask-dialog-fade-leave-to {
  opacity: 0;
  backdrop-filter: blur(0px);
}

.subtask-dialog-fade-enter-from .subtask-dialog,
.subtask-dialog-fade-leave-to .subtask-dialog {
  opacity: 0;
  transform: translateY(18px) scale(0.96);
}

.subtask-dialog__backdrop {
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

.subtask-dialog__backdrop::before {
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

.subtask-dialog {
  position: relative;
  width: min(100%, 560px);
  display: grid;
  gap: 16px;
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
textarea,
button {
  border-radius: 8px;
  padding: 10px 14px;
  font: inherit;
}

input,
textarea {
  border: 1px solid var(--border-soft);
  background: color-mix(in srgb, var(--panel-bg) 92%, transparent);
  color: var(--text-main);
}

input[type='range'] {
  appearance: none;
  height: 8px;
  padding: 0;
  border: none;
  border-radius: 999px;
  background:
    linear-gradient(90deg, var(--accent-start), var(--accent-end)),
    color-mix(in srgb, var(--panel-bg) 88%, transparent);
  box-shadow:
    inset 0 0 0 1px color-mix(in srgb, var(--border-soft) 82%, transparent),
    0 0 0 1px rgba(255, 255, 255, 0.03);
}

input[type='range']::-webkit-slider-thumb {
  appearance: none;
  width: 18px;
  height: 18px;
  border: 2px solid rgba(255, 255, 255, 0.72);
  border-radius: 50%;
  background: linear-gradient(135deg, var(--accent-start), var(--accent-end));
  box-shadow:
    0 0 0 4px rgba(10, 102, 255, 0.12),
    0 6px 14px rgba(0, 0, 0, 0.22);
  cursor: pointer;
}

input[type='range']::-moz-range-thumb {
  width: 18px;
  height: 18px;
  border: 2px solid rgba(255, 255, 255, 0.72);
  border-radius: 50%;
  background: linear-gradient(135deg, var(--accent-start), var(--accent-end));
  box-shadow:
    0 0 0 4px rgba(10, 102, 255, 0.12),
    0 6px 14px rgba(0, 0, 0, 0.22);
  cursor: pointer;
}

.subtask-dialog__progress {
  gap: 12px;
}

.subtask-dialog__progress-header,
.subtask-dialog__progress-scale {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.subtask-dialog__progress-header strong {
  color: var(--text-main);
  font-size: 1rem;
}

.subtask-dialog__progress-scale small {
  color: var(--text-soft);
  font-size: 0.8rem;
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
