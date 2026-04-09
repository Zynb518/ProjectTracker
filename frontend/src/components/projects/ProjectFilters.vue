<script setup lang="ts">
type DialogTriggerOrigin = {
  x: number
  y: number
}

type StatusTone = 'all' | 'pending' | 'active' | 'done' | 'delayed'

type StatusOption = {
  value: string
  label: string
  tone: StatusTone
}

const props = defineProps<{
  keyword: string
  status: string
}>()

const emit = defineEmits<{
  'update:keyword': [value: string]
  'update:status': [value: string]
  submit: []
  create: [origin: DialogTriggerOrigin]
}>()

const statusOptions: StatusOption[] = [
  { value: '', label: '全部状态', tone: 'all' },
  { value: '1', label: '未开始', tone: 'pending' },
  { value: '2', label: '进行中', tone: 'active' },
  { value: '3', label: '已完成', tone: 'done' },
  { value: '4', label: '已延期', tone: 'delayed' },
]

function updateKeyword(event: Event) {
  emit('update:keyword', (event.target as HTMLInputElement).value)
}

function submitKeyword() {
  emit('submit')
}

function selectStatus(nextStatus: string) {
  if (nextStatus === props.status) {
    return
  }

  emit('update:status', nextStatus)
  emit('submit')
}

function emitCreateOrigin(event: MouseEvent) {
  const trigger = event.currentTarget as HTMLElement | null

  if (!trigger) {
    return
  }

  const rect = trigger.getBoundingClientRect()
  emit('create', {
    x: rect.left + rect.width / 2,
    y: rect.top + rect.height / 2,
  })
}
</script>

<template>
  <section class="project-filters">
    <label class="project-filters__field">
      <span>项目关键字</span>
      <input
        :value="keyword"
        class="project-filters__input"
        placeholder="按项目名称搜索"
        type="text"
        @input="updateKeyword"
        @keydown.enter.prevent="submitKeyword"
      />
    </label>

    <div class="project-filters__field">
      <span>项目状态</span>
      <div class="project-filters__status-bar" role="group" aria-label="项目状态">
        <button
          v-for="option in statusOptions"
          :key="option.value || 'all'"
          :aria-pressed="status === option.value"
          :class="[
            'project-filters__status-pill',
            `project-filters__status-pill--${option.tone}`,
            { 'project-filters__status-pill--selected': status === option.value },
          ]"
          :data-testid="`projects-status-${option.value || 'all'}`"
          type="button"
          @click="selectStatus(option.value)"
        >
          {{ option.label }}
        </button>
      </div>
    </div>

    <div class="project-filters__actions">
      <button
        type="button"
        data-testid="create-project"
        aria-label="新建项目"
        class="project-filters__create"
        data-tooltip="新建项目"
        @click="emitCreateOrigin"
      >
        <svg aria-hidden="true" class="project-filters__create-icon" viewBox="0 0 24 24">
          <path
            d="M12 5v14M5 12h14"
            fill="none"
            stroke="currentColor"
            stroke-linecap="round"
            stroke-width="2"
          />
        </svg>
      </button>
    </div>
  </section>
</template>

<style scoped>
.project-filters {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr)) auto;
  gap: 14px;
  padding: 18px;
  border: 1px solid var(--border-soft);
  border-radius: 18px;
  background: var(--glass-bg);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
}

.project-filters__field {
  display: grid;
  gap: 8px;
}

.project-filters__field span {
  font-size: 0.76rem;
  font-weight: 600;
  letter-spacing: 0.08em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.project-filters__input {
  min-height: 46px;
  border: 1px solid var(--border-soft);
  border-radius: 8px;
  padding: 0 14px;
  background: color-mix(in srgb, var(--panel-bg) 92%, transparent);
  color: var(--text-main);
  font: inherit;
}

.project-filters__input::placeholder {
  color: var(--text-muted);
}

.project-filters__input:focus {
  border-color: var(--accent-line);
  box-shadow: 0 0 0 4px rgba(10, 102, 255, 0.12);
}

.project-filters__status-bar {
  min-height: 46px;
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 10px;
  padding: 6px;
  border: 1px solid var(--border-soft);
  border-radius: 14px;
  background: color-mix(in srgb, var(--panel-bg) 92%, transparent);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.05);
}

.project-filters__status-pill {
  min-height: 34px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  padding: 0 14px;
  border: 1px solid transparent;
  border-radius: 999px;
  background: transparent;
  color: var(--text-soft);
  font: inherit;
  font-size: 0.82rem;
  font-weight: 600;
  letter-spacing: 0.04em;
  line-height: 1;
  white-space: nowrap;
  cursor: pointer;
  transition:
    transform 200ms ease-out,
    border-color 200ms ease-out,
    background 200ms ease-out,
    color 200ms ease-out,
    box-shadow 200ms ease-out,
    opacity 200ms ease-out;
}

.project-filters__status-pill::before {
  content: '';
  width: 8px;
  height: 8px;
  border-radius: 999px;
  background: currentColor;
  opacity: 0.88;
}

.project-filters__status-pill:hover {
  transform: translateY(-1px);
}

.project-filters__status-pill:focus-visible {
  outline: none;
  border-color: var(--accent-line);
  box-shadow: 0 0 0 4px rgba(10, 102, 255, 0.12);
}

.project-filters__status-pill--all {
  border-color: var(--border-soft);
  background: color-mix(in srgb, var(--panel-bg) 88%, transparent);
  color: var(--text-soft);
}

.project-filters__status-pill--pending {
  border-color: color-mix(in srgb, var(--accent-neutral) 30%, transparent);
  background: color-mix(in srgb, var(--accent-neutral) 12%, transparent);
  color: var(--text-soft);
}

.project-filters__status-pill--active {
  border-color: rgba(0, 194, 255, 0.22);
  background: var(--gradient-primary-soft);
  color: #53d4ff;
}

.project-filters__status-pill--done {
  border-color: color-mix(in srgb, var(--accent-success) 28%, transparent);
  background: color-mix(in srgb, var(--accent-success) 12%, transparent);
  color: var(--accent-success);
}

.project-filters__status-pill--delayed {
  border-color: color-mix(in srgb, var(--accent-warning) 28%, transparent);
  background: color-mix(in srgb, var(--accent-warning) 12%, transparent);
  color: var(--accent-warning);
}

.project-filters__status-pill--selected {
  box-shadow:
    0 10px 20px rgba(3, 10, 24, 0.12),
    inset 0 1px 0 rgba(255, 255, 255, 0.08);
}

.project-filters__status-pill--all.project-filters__status-pill--selected {
  border-color: var(--accent-line);
  background: color-mix(in srgb, var(--panel-bg) 72%, transparent);
  color: var(--text-main);
}

.project-filters__status-pill--pending.project-filters__status-pill--selected {
  background: color-mix(in srgb, var(--accent-neutral) 18%, transparent);
  color: var(--text-main);
}

.project-filters__status-pill--active.project-filters__status-pill--selected {
  border-color: rgba(0, 194, 255, 0.28);
  background: var(--gradient-primary);
  color: var(--text-inverse);
}

.project-filters__status-pill--done.project-filters__status-pill--selected {
  background: color-mix(in srgb, var(--accent-success) 20%, transparent);
}

.project-filters__status-pill--delayed.project-filters__status-pill--selected {
  background: color-mix(in srgb, var(--accent-warning) 20%, transparent);
}

.project-filters__actions {
  display: flex;
  align-items: end;
  gap: 10px;
}

.project-filters__create {
  position: relative;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 46px;
  height: 46px;
  border: none;
  border-radius: 14px;
  padding: 0;
  background: var(--gradient-primary);
  color: var(--text-inverse);
  cursor: pointer;
  box-shadow: 0 16px 28px rgba(10, 102, 255, 0.2);
  transition:
    transform 200ms ease-out,
    border-color 200ms ease-out,
    box-shadow 200ms ease-out,
    background 200ms ease-out,
    color 200ms ease-out;
}

.project-filters__create:hover {
  transform: translateY(-2px);
  box-shadow: var(--shadow-panel-hover);
}

.project-filters__create:focus-visible {
  outline: none;
  transform: translateY(-2px);
  box-shadow:
    var(--shadow-panel-hover),
    0 0 0 4px color-mix(in srgb, var(--accent-end) 18%, transparent);
}

.project-filters__create-icon {
  width: 18px;
  height: 18px;
}

.project-filters__create[data-tooltip]::before,
.project-filters__create[data-tooltip]::after {
  position: absolute;
  left: 50%;
  opacity: 0;
  pointer-events: none;
  transition:
    opacity 180ms ease-out,
    transform 180ms ease-out;
}

.project-filters__create[data-tooltip]::before {
  content: '';
  bottom: calc(100% + 5px);
  width: 10px;
  height: 10px;
  border-left: 1px solid var(--border-soft);
  border-bottom: 1px solid var(--border-soft);
  background: color-mix(in srgb, var(--panel-bg) 96%, transparent);
  transform: translate(-50%, 6px) rotate(-45deg);
  z-index: 1;
}

.project-filters__create[data-tooltip]::after {
  content: attr(data-tooltip);
  bottom: calc(100% + 11px);
  padding: 7px 10px;
  border: 1px solid var(--border-soft);
  border-radius: 10px;
  background: color-mix(in srgb, var(--panel-bg) 96%, transparent);
  color: var(--text-main);
  font-size: 0.75rem;
  font-weight: 600;
  line-height: 1;
  white-space: nowrap;
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
  transform: translate(-50%, 6px);
  z-index: 2;
}

.project-filters__create[data-tooltip]:hover::before,
.project-filters__create[data-tooltip]:hover::after,
.project-filters__create[data-tooltip]:focus-visible::before,
.project-filters__create[data-tooltip]:focus-visible::after {
  opacity: 1;
  transform: translate(-50%, 0);
}

.project-filters__create[data-tooltip]:hover::before,
.project-filters__create[data-tooltip]:focus-visible::before {
  transform: translate(-50%, 0) rotate(-45deg);
}

@media (max-width: 820px) {
  .project-filters {
    grid-template-columns: 1fr;
  }

  .project-filters__actions {
    align-items: stretch;
  }

  .project-filters__status-bar {
    gap: 8px;
  }
}
</style>
