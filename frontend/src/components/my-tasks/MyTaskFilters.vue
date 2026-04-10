<script setup lang="ts">
const statusOptions = [
  { value: '', label: '全部状态', tone: 'all' },
  { value: '1', label: '未开始', tone: 'pending' },
  { value: '2', label: '进行中', tone: 'active' },
  { value: '3', label: '已完成', tone: 'done' },
  { value: '4', label: '已延期', tone: 'delayed' },
] as const

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
    <label class="my-task-filters__field">
      <span>项目 ID</span>
      <input
        :value="projectId"
        placeholder="例如 1001"
        type="text"
        @input="$emit('update:projectId', ($event.target as HTMLInputElement).value)"
      />
    </label>

    <div class="my-task-filters__field my-task-filters__field--status">
      <span>任务状态</span>
      <div
        data-testid="my-tasks-status"
        class="my-task-filters__status-group"
        role="group"
        aria-label="任务状态"
      >
        <button
          v-for="option in statusOptions"
          :key="option.value || 'all'"
          :aria-pressed="status === option.value"
          :class="[
            'my-task-filters__status-pill',
            `my-task-filters__status-pill--${option.tone}`,
            { 'is-active': status === option.value },
          ]"
          :data-testid="`my-task-filter-status-${option.value || 'all'}`"
          type="button"
          @click="$emit('update:status', option.value)"
        >
          {{ option.label }}
        </button>
      </div>
    </div>

    <button class="my-task-filters__submit" type="button" @click="$emit('submit')">
      <svg viewBox="0 0 24 24" aria-hidden="true">
        <path
          d="M20 12a8 8 0 1 1-2.34-5.66"
          fill="none"
          stroke="currentColor"
          stroke-linecap="round"
          stroke-width="1.8"
        />
        <path
          d="M20 4v5h-5"
          fill="none"
          stroke="currentColor"
          stroke-linecap="round"
          stroke-linejoin="round"
          stroke-width="1.8"
        />
      </svg>
      <span>刷新任务</span>
    </button>
  </section>
</template>

<style scoped>
.my-task-filters {
  display: grid;
  grid-template-columns: minmax(0, 220px) minmax(0, 1fr) auto;
  align-items: end;
  gap: 14px;
  padding: 18px;
  border: 1px solid var(--border-soft);
  border-radius: 18px;
  background: var(--glass-bg);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
}

.my-task-filters__field {
  display: grid;
  gap: 8px;
}

.my-task-filters__field--status {
  min-width: 0;
}

.my-task-filters__field > span {
  font-size: 0.78rem;
  letter-spacing: 0.12em;
  text-transform: uppercase;
  color: var(--text-soft);
}

input {
  min-height: 46px;
  border: 1px solid var(--border-soft);
  border-radius: 12px;
  padding: 0 14px;
  background: color-mix(in srgb, var(--panel-bg) 92%, transparent);
  color: var(--text-main);
  font: inherit;
}

.my-task-filters__status-group {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  padding: 0;
  border: 1px solid transparent;
  border-radius: 0;
  background: transparent;
  box-shadow: none;
}

.my-task-filters__status-pill {
  min-height: 42px;
  padding: 0 14px;
  border: 1px solid transparent;
  border-radius: 999px;
  background: var(--dialog-control-bg);
  color: var(--text-soft);
  font: inherit;
  font-size: 0.84rem;
  font-weight: 700;
  cursor: pointer;
  transition:
    transform 180ms ease-out,
    box-shadow 180ms ease-out,
    border-color 180ms ease-out,
    background-color 180ms ease-out,
    color 180ms ease-out;
}

.my-task-filters__status-pill:hover {
  transform: translateY(-1px);
  box-shadow:
    inset 0 1px 0 color-mix(in srgb, #ffffff 16%, transparent),
    0 8px 16px color-mix(in srgb, var(--accent-end) 10%, transparent);
}

.my-task-filters__status-pill--all {
  border-color: var(--work-status-unknown-border);
  background: var(--work-status-unknown-bg);
  color: var(--work-status-unknown-color);
}

.my-task-filters__status-pill--pending {
  border-color: var(--work-status-pending-border);
  background: var(--work-status-pending-bg);
  color: var(--work-status-pending-color);
}

.my-task-filters__status-pill--active {
  border-color: var(--work-status-active-border);
  background: var(--work-status-active-bg);
  color: var(--work-status-active-color);
}

.my-task-filters__status-pill--done {
  border-color: var(--work-status-done-border);
  background: var(--work-status-done-bg);
  color: var(--work-status-done-color);
}

.my-task-filters__status-pill--delayed {
  border-color: var(--work-status-delayed-border);
  background: var(--work-status-delayed-bg);
  color: var(--work-status-delayed-color);
}

.my-task-filters__status-pill.is-active {
  transform: translateY(-1px);
  box-shadow:
    inset 0 1px 0 color-mix(in srgb, #ffffff 18%, transparent),
    0 10px 20px color-mix(in srgb, var(--accent-end) 14%, transparent);
}

.my-task-filters__submit {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 10px;
  min-height: 52px;
  border: none;
  border-radius: 12px;
  padding: 12px 18px;
  background: linear-gradient(135deg, color-mix(in srgb, var(--accent-start) 88%, #0d2754 12%), color-mix(in srgb, var(--accent-end) 78%, #0f2c5b 22%));
  color: var(--text-inverse);
  cursor: pointer;
  box-shadow:
    inset 0 1px 0 color-mix(in srgb, #ffffff 18%, transparent),
    0 18px 32px color-mix(in srgb, var(--accent-end) 22%, transparent);
  transition:
    transform 180ms ease-out,
    box-shadow 180ms ease-out,
    filter 180ms ease-out;
}

.my-task-filters__submit svg {
  width: 18px;
  height: 18px;
  flex: none;
}

.my-task-filters__submit:hover {
  transform: translateY(-2px);
  box-shadow:
    inset 0 1px 0 color-mix(in srgb, #ffffff 22%, transparent),
    0 22px 38px color-mix(in srgb, var(--accent-end) 26%, transparent);
  filter: saturate(1.06);
}

@media (max-width: 820px) {
  .my-task-filters {
    grid-template-columns: 1fr;
  }
}
</style>
