<script setup lang="ts">
import { computed, reactive, watch } from 'vue'

import type { Subtask, SubtaskProgressRecord } from '@/types/subtask'
import { formatDisplayDateTime, getWorkStatusLabel, getWorkStatusTone } from '@/utils/display'

const props = defineProps<{
  modelValue: boolean
  subtask: Subtask | null
  records: SubtaskProgressRecord[]
  isHistoryLoading: boolean
  historyLoadError: string | null
  isSubmitting: boolean
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
const sliderStyle = computed(() => ({
  '--my-task-progress-current': `${currentProgress.value}%`,
  '--my-task-progress-value': `${form.progress_percent}%`,
}))

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

function syncProgressInput(event: Event) {
  const target = event.target as HTMLInputElement
  const nextValue = normalizeProgress(Number(target.value))

  form.progress_percent = nextValue
  target.value = String(nextValue)
}

function submit() {
  if (!props.subtask || props.isSubmitting) {
    return
  }

  emit('submit', {
    subtaskId: props.subtask.id,
    progress_percent: normalizeProgress(form.progress_percent),
    progress_note: form.progress_note,
  })
}

function formatRecordTime(value: string) {
  return formatDisplayDateTime(value)
}
</script>

<template>
  <Transition appear name="my-task-progress-dialog-fade">
    <div v-if="modelValue && subtask" class="my-task-progress-dialog__backdrop">
      <section class="my-task-progress-dialog" role="dialog" aria-modal="true">
        <header class="my-task-progress-dialog__header">
          <div class="my-task-progress-dialog__hero">
            <p class="my-task-progress-dialog__eyebrow">Execution Desk</p>
            <h2>{{ subtask.name }}</h2>

            <div class="my-task-progress-dialog__chips">
              <span class="my-task-progress-dialog__chip my-task-progress-dialog__chip--project">
                {{ subtask.project_name || '未归属项目' }}
              </span>
              <span class="my-task-progress-dialog__chip my-task-progress-dialog__chip--node">
                {{ subtask.node_name || '未归属节点' }}
              </span>
              <span
                :class="[
                  'my-task-progress-dialog__chip',
                  'my-task-progress-dialog__chip--status',
                  `my-task-progress-dialog__chip--${getWorkStatusTone(subtask.status)}`,
                ]"
              >
                {{ getWorkStatusLabel(subtask.status) }}
              </span>
            </div>
          </div>

          <button type="button" class="my-task-progress-dialog__close" @click="close">关闭</button>
        </header>

        <div class="my-task-progress-dialog__body">
          <form class="my-task-progress-dialog__editor" @submit.prevent="submit">
            <label class="my-task-progress-dialog__panel my-task-progress-dialog__panel--progress">
              <div class="my-task-progress-dialog__panel-head">
                <span>进度滑杆</span>
                <strong>{{ form.progress_percent }}%</strong>
              </div>

              <input
                :value="form.progress_percent"
                :min="0"
                :style="sliderStyle"
                aria-label="进度百分比"
                data-testid="my-task-progress-range"
                max="100"
                step="10"
                type="range"
                @input="syncProgressInput"
              />

              <div class="my-task-progress-dialog__progress-scale">
                <small>0%</small>
                <small>不可低于当前值 {{ currentProgress }}%</small>
                <small>100%</small>
              </div>
            </label>

            <label class="my-task-progress-dialog__panel my-task-progress-dialog__panel--note">
              <span>进展说明</span>
              <textarea
                v-model="form.progress_note"
                aria-label="进展说明"
                placeholder="填写本次推进内容、阻塞点或下一步安排"
                rows="7"
              />
            </label>

            <p class="my-task-progress-dialog__hint">
              这里只提交进度百分比和进展说明，不修改任务状态。
            </p>

            <footer class="my-task-progress-dialog__footer">
              <button type="button" @click="close">取消</button>
              <button type="submit" class="primary" :disabled="isSubmitting">
                {{ isSubmitting ? '提交中...' : '提交进度' }}
              </button>
            </footer>
          </form>

          <aside class="my-task-progress-dialog__history">
            <header class="my-task-progress-dialog__history-head">
              <div>
                <p>Progress History</p>
                <h3>最近 20 条提交</h3>
              </div>
              <span>独立滚动</span>
            </header>

            <div class="my-task-progress-dialog__history-list">
              <p
                v-if="isHistoryLoading"
                class="my-task-progress-dialog__history-state my-task-progress-dialog__history-state--loading"
              >
                历史记录加载中...
              </p>

              <p
                v-else-if="historyLoadError"
                class="my-task-progress-dialog__history-state my-task-progress-dialog__history-state--error"
              >
                {{ historyLoadError }}
              </p>

              <p
                v-else-if="records.length === 0"
                class="my-task-progress-dialog__history-state"
              >
                还没有进度提交记录。
              </p>

              <template v-else>
                <article
                  v-for="record in records"
                  :key="record.id"
                  class="my-task-progress-dialog__history-item"
                >
                  <div class="my-task-progress-dialog__history-top">
                    <strong>{{ record.operator_real_name || `用户 ${record.operator_user_id}` }}</strong>
                    <span class="my-task-progress-dialog__history-progress">{{ record.progress_percent }}%</span>
                  </div>

                  <p>{{ record.progress_note || '无补充说明' }}</p>

                  <div class="my-task-progress-dialog__history-meta">
                    <span>{{ formatRecordTime(record.created_at) }}</span>
                    <span
                      :class="[
                        'my-task-progress-dialog__history-status',
                        `my-task-progress-dialog__history-status--${getWorkStatusTone(record.status)}`,
                      ]"
                    >
                      {{ getWorkStatusLabel(record.status) }}
                    </span>
                  </div>
                </article>
              </template>
            </div>
          </aside>
        </div>
      </section>
    </div>
  </Transition>
</template>

<style scoped>
.my-task-progress-dialog-fade-enter-active,
.my-task-progress-dialog-fade-leave-active {
  transition:
    opacity 260ms ease-out,
    backdrop-filter 260ms ease-out;
}

.my-task-progress-dialog-fade-enter-active .my-task-progress-dialog,
.my-task-progress-dialog-fade-leave-active .my-task-progress-dialog {
  transition:
    opacity 300ms cubic-bezier(0.16, 1, 0.3, 1),
    transform 340ms cubic-bezier(0.16, 1, 0.3, 1);
}

.my-task-progress-dialog-fade-enter-from,
.my-task-progress-dialog-fade-leave-to {
  opacity: 0;
  backdrop-filter: blur(0px);
}

.my-task-progress-dialog-fade-enter-from .my-task-progress-dialog,
.my-task-progress-dialog-fade-leave-to .my-task-progress-dialog {
  opacity: 0;
  transform: translateY(18px) scale(0.97);
}

.my-task-progress-dialog__backdrop {
  position: fixed;
  inset: 0;
  z-index: 42;
  display: grid;
  place-items: center;
  padding: 24px;
  background:
    radial-gradient(circle at 18% 22%, rgba(10, 102, 255, 0.12), transparent 22%),
    radial-gradient(circle at 82% 78%, rgba(26, 184, 255, 0.14), transparent 24%),
    var(--overlay-backdrop);
  backdrop-filter: blur(12px);
}

.my-task-progress-dialog__backdrop::before {
  content: '';
  position: absolute;
  inset: 0;
  background:
    linear-gradient(var(--overlay-grid-line) 1px, transparent 1px),
    linear-gradient(90deg, var(--overlay-grid-line) 1px, transparent 1px);
  background-size: 28px 28px;
  opacity: 0.18;
  pointer-events: none;
}

.my-task-progress-dialog {
  position: relative;
  width: min(100%, 1040px);
  max-height: min(88vh, 780px);
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
  border: 1px solid var(--dialog-surface-border);
  border-radius: 28px;
  background: var(--dialog-surface-glow), var(--dialog-surface-bg), var(--card-sheen);
  box-shadow: var(--dialog-surface-shadow);
  overflow: hidden;
}

.my-task-progress-dialog__header {
  display: flex;
  align-items: start;
  justify-content: space-between;
  gap: 18px;
  padding: 22px 26px;
  border-bottom: 1px solid color-mix(in srgb, var(--dialog-surface-border) 88%, transparent);
  background:
    linear-gradient(135deg, color-mix(in srgb, var(--accent-start) 8%, transparent), color-mix(in srgb, var(--accent-end) 4%, transparent)),
    linear-gradient(180deg, color-mix(in srgb, var(--panel-bg) 94%, transparent), transparent);
}

.my-task-progress-dialog__hero {
  min-width: 0;
  display: grid;
  gap: 10px;
}

.my-task-progress-dialog__eyebrow,
.my-task-progress-dialog__history-head p {
  margin: 0;
  font-size: 0.74rem;
  letter-spacing: 0.18em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.my-task-progress-dialog__hero h2,
.my-task-progress-dialog__history-head h3 {
  margin: 0;
}

.my-task-progress-dialog__hero h2 {
  font-size: 1.65rem;
  line-height: 1.18;
  color: var(--text-main);
}

.my-task-progress-dialog__chips {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.my-task-progress-dialog__chip {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-height: 32px;
  padding: 0 12px;
  border: 1px solid transparent;
  border-radius: 999px;
  font-size: 0.78rem;
  font-weight: 700;
}

.my-task-progress-dialog__chip--project {
  border-color: color-mix(in srgb, var(--accent-start) 24%, transparent);
  background: color-mix(in srgb, var(--accent-start) 12%, transparent);
  color: color-mix(in srgb, var(--accent-start) 72%, var(--text-main));
}

.my-task-progress-dialog__chip--node {
  border-color: color-mix(in srgb, var(--border-soft) 92%, transparent);
  background: color-mix(in srgb, var(--dialog-control-bg) 98%, transparent);
  color: var(--text-main);
}

.my-task-progress-dialog__chip--pending {
  border-color: var(--work-status-pending-border);
  background: var(--work-status-pending-bg);
  color: var(--work-status-pending-color);
}

.my-task-progress-dialog__chip--active {
  border-color: var(--work-status-active-border);
  background: var(--work-status-active-bg);
  color: var(--work-status-active-color);
}

.my-task-progress-dialog__chip--done {
  border-color: var(--work-status-done-border);
  background: var(--work-status-done-bg);
  color: var(--work-status-done-color);
}

.my-task-progress-dialog__chip--delayed {
  border-color: var(--work-status-delayed-border);
  background: var(--work-status-delayed-bg);
  color: var(--work-status-delayed-color);
}

.my-task-progress-dialog__chip--unknown {
  border-color: var(--work-status-unknown-border);
  background: var(--work-status-unknown-bg);
  color: var(--work-status-unknown-color);
}

.my-task-progress-dialog__body {
  min-height: 0;
  display: grid;
  grid-template-columns: minmax(0, 1.35fr) minmax(320px, 0.85fr);
}

.my-task-progress-dialog__editor,
.my-task-progress-dialog__history {
  min-height: 0;
}

.my-task-progress-dialog__editor {
  display: grid;
  align-content: start;
  gap: 18px;
  padding: 26px;
  background: var(--meta-surface-bg), var(--meta-surface-glow), var(--card-sheen);
  overflow: auto;
}

.my-task-progress-dialog__panel {
  display: grid;
  gap: 12px;
  padding: 18px;
  border: 1px solid color-mix(in srgb, var(--dialog-control-border) 96%, transparent);
  border-radius: 20px;
  background: var(--dialog-control-bg), var(--card-sheen);
  box-shadow: var(--dialog-control-shadow);
}

.my-task-progress-dialog__panel-head,
.my-task-progress-dialog__progress-scale,
.my-task-progress-dialog__history-top,
.my-task-progress-dialog__history-meta {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.my-task-progress-dialog__panel span,
.my-task-progress-dialog__hint,
.my-task-progress-dialog__history-head span,
.my-task-progress-dialog__history-item p,
.my-task-progress-dialog__history-meta span {
  color: var(--text-soft);
}

.my-task-progress-dialog__panel-head span,
.my-task-progress-dialog__panel > span {
  font-size: 0.8rem;
  letter-spacing: 0.12em;
  text-transform: uppercase;
}

.my-task-progress-dialog__panel-head strong {
  color: var(--accent-start);
  font-size: 1.8rem;
  line-height: 1;
}

.my-task-progress-dialog__panel input,
.my-task-progress-dialog__panel textarea,
.my-task-progress-dialog__close,
.my-task-progress-dialog__footer button {
  font: inherit;
}

.my-task-progress-dialog__panel textarea,
.my-task-progress-dialog__close,
.my-task-progress-dialog__footer button {
  border-radius: 14px;
}

.my-task-progress-dialog__panel textarea,
.my-task-progress-dialog__close,
.my-task-progress-dialog__footer button {
  border: 1px solid var(--dialog-control-border);
  background: var(--dialog-control-bg);
  box-shadow: var(--dialog-control-shadow);
  color: var(--text-main);
}

.my-task-progress-dialog__panel textarea {
  min-height: 176px;
  padding: 14px 16px;
  resize: vertical;
}

.my-task-progress-dialog__panel textarea::placeholder {
  color: var(--text-muted);
}

.my-task-progress-dialog__panel textarea:focus,
.my-task-progress-dialog__close:focus-visible,
.my-task-progress-dialog__footer button:focus-visible {
  outline: none;
  border-color: var(--accent-line);
  box-shadow:
    var(--dialog-control-shadow),
    0 0 0 4px rgba(10, 102, 255, 0.12);
}

.my-task-progress-dialog__panel input[type='range'] {
  --my-task-progress-current: 0%;
  --my-task-progress-value: 0%;
  --my-task-progress-locked-color: color-mix(in srgb, var(--accent-neutral) 68%, var(--text-soft));
  --my-task-progress-remaining-color: color-mix(in srgb, var(--panel-bg) 92%, var(--text-soft));
  --my-task-progress-track-background: linear-gradient(
    90deg,
    var(--my-task-progress-locked-color) 0%,
    var(--my-task-progress-locked-color) var(--my-task-progress-current),
    var(--accent-start) var(--my-task-progress-current),
    var(--accent-end) var(--my-task-progress-value),
    var(--my-task-progress-remaining-color) var(--my-task-progress-value),
    var(--my-task-progress-remaining-color) 100%
  );
  appearance: none;
  height: 18px;
  padding: 0;
  border: none;
  background: transparent;
}

.my-task-progress-dialog__panel input[type='range']::-webkit-slider-runnable-track {
  height: 10px;
  border-radius: 999px;
  background: var(--my-task-progress-track-background);
  box-shadow:
    inset 0 0 0 1px var(--dialog-control-border),
    0 0 0 1px color-mix(in srgb, var(--dialog-surface-border) 18%, transparent);
}

.my-task-progress-dialog__panel input[type='range']::-moz-range-track {
  height: 10px;
  border: none;
  border-radius: 999px;
  background: var(--my-task-progress-track-background);
  box-shadow:
    inset 0 0 0 1px var(--dialog-control-border),
    0 0 0 1px color-mix(in srgb, var(--dialog-surface-border) 18%, transparent);
}

.my-task-progress-dialog__panel input[type='range']::-webkit-slider-thumb {
  appearance: none;
  width: 20px;
  height: 20px;
  margin-top: -5px;
  border: 2px solid color-mix(in srgb, var(--text-inverse) 58%, var(--accent-end) 42%);
  border-radius: 50%;
  background: linear-gradient(135deg, var(--accent-start), var(--accent-end));
  box-shadow:
    0 0 0 4px rgba(10, 102, 255, 0.12),
    0 6px 14px rgba(0, 0, 0, 0.22);
  cursor: pointer;
}

.my-task-progress-dialog__panel input[type='range']::-moz-range-thumb {
  width: 20px;
  height: 20px;
  border: 2px solid color-mix(in srgb, var(--text-inverse) 58%, var(--accent-end) 42%);
  border-radius: 50%;
  background: linear-gradient(135deg, var(--accent-start), var(--accent-end));
  box-shadow:
    0 0 0 4px rgba(10, 102, 255, 0.12),
    0 6px 14px rgba(0, 0, 0, 0.22);
  cursor: pointer;
}

.my-task-progress-dialog__progress-scale small {
  font-size: 0.8rem;
  color: var(--text-soft);
}

.my-task-progress-dialog__hint {
  margin: 0;
  padding: 0 4px;
  font-size: 0.92rem;
  line-height: 1.6;
}

.my-task-progress-dialog__footer {
  display: flex;
  justify-content: flex-end;
  gap: 12px;
}

.my-task-progress-dialog__close,
.my-task-progress-dialog__footer button {
  min-height: 46px;
  padding: 0 16px;
  cursor: pointer;
}

.my-task-progress-dialog__footer .primary {
  border: none;
  background: linear-gradient(135deg, color-mix(in srgb, var(--accent-start) 88%, #0d2754 12%), color-mix(in srgb, var(--accent-end) 78%, #0f2c5b 22%));
  color: var(--text-inverse);
  box-shadow:
    inset 0 1px 0 color-mix(in srgb, #ffffff 18%, transparent),
    0 18px 32px color-mix(in srgb, var(--accent-end) 22%, transparent);
}

.my-task-progress-dialog__footer .primary:disabled {
  cursor: progress;
  opacity: 0.72;
}

.my-task-progress-dialog__history {
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
  gap: 18px;
  padding: 24px;
  border-left: 1px solid color-mix(in srgb, var(--dialog-surface-border) 88%, transparent);
  background: var(--meta-surface-bg), var(--meta-surface-glow), var(--card-sheen);
}

.my-task-progress-dialog__history-head {
  display: flex;
  align-items: end;
  justify-content: space-between;
  gap: 14px;
}

.my-task-progress-dialog__history-head h3 {
  font-size: 1.25rem;
  color: var(--text-main);
}

.my-task-progress-dialog__history-head span {
  font-size: 0.78rem;
  letter-spacing: 0.08em;
  text-transform: uppercase;
}

.my-task-progress-dialog__history-list {
  min-height: 0;
  display: grid;
  align-content: start;
  gap: 12px;
  overflow: auto;
  padding-right: 4px;
}

.my-task-progress-dialog__history-state {
  margin: 0;
  padding: 18px;
  border: 1px dashed color-mix(in srgb, var(--dialog-control-border) 92%, transparent);
  border-radius: 18px;
  background: var(--dialog-control-bg);
}

.my-task-progress-dialog__history-state--error {
  border-style: solid;
  border-color: color-mix(in srgb, var(--accent-warning) 22%, transparent);
  color: color-mix(in srgb, var(--accent-warning) 76%, var(--text-main));
}

.my-task-progress-dialog__history-item {
  display: grid;
  gap: 10px;
  padding: 16px;
  border: 1px solid color-mix(in srgb, var(--dialog-control-border) 96%, transparent);
  border-radius: 18px;
  background: var(--dialog-control-bg), var(--card-sheen);
  box-shadow: var(--dialog-control-shadow);
}

.my-task-progress-dialog__history-item p {
  margin: 0;
  font-size: 0.92rem;
  line-height: 1.6;
}

.my-task-progress-dialog__history-progress,
.my-task-progress-dialog__history-status {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-height: 28px;
  padding: 0 10px;
  border: 1px solid transparent;
  border-radius: 999px;
  font-size: 0.76rem;
  font-weight: 700;
}

.my-task-progress-dialog__history-progress {
  border-color: color-mix(in srgb, var(--accent-start) 24%, transparent);
  background: color-mix(in srgb, var(--accent-start) 12%, transparent);
  color: color-mix(in srgb, var(--accent-start) 72%, var(--text-main));
}

.my-task-progress-dialog__history-status--pending {
  border-color: var(--work-status-pending-border);
  background: var(--work-status-pending-bg);
  color: var(--work-status-pending-color);
}

.my-task-progress-dialog__history-status--active {
  border-color: var(--work-status-active-border);
  background: var(--work-status-active-bg);
  color: var(--work-status-active-color);
}

.my-task-progress-dialog__history-status--done {
  border-color: var(--work-status-done-border);
  background: var(--work-status-done-bg);
  color: var(--work-status-done-color);
}

.my-task-progress-dialog__history-status--delayed {
  border-color: var(--work-status-delayed-border);
  background: var(--work-status-delayed-bg);
  color: var(--work-status-delayed-color);
}

.my-task-progress-dialog__history-status--unknown {
  border-color: var(--work-status-unknown-border);
  background: var(--work-status-unknown-bg);
  color: var(--work-status-unknown-color);
}

@media (max-width: 920px) {
  .my-task-progress-dialog {
    width: min(100%, 820px);
  }

  .my-task-progress-dialog__body {
    grid-template-columns: 1fr;
  }

  .my-task-progress-dialog__history {
    border-left: 0;
    border-top: 1px solid color-mix(in srgb, var(--dialog-surface-border) 88%, transparent);
    max-height: 320px;
  }
}

@media (max-width: 640px) {
  .my-task-progress-dialog__backdrop {
    padding: 14px;
  }

  .my-task-progress-dialog {
    max-height: 92vh;
    border-radius: 22px;
  }

  .my-task-progress-dialog__header,
  .my-task-progress-dialog__editor,
  .my-task-progress-dialog__history {
    padding: 18px;
  }

  .my-task-progress-dialog__header {
    grid-template-columns: 1fr;
  }

  .my-task-progress-dialog__header,
  .my-task-progress-dialog__history-head,
  .my-task-progress-dialog__footer {
    flex-direction: column;
    align-items: stretch;
  }
}
</style>
