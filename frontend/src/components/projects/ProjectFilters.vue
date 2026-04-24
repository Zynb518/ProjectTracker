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
  embedded?: boolean
  keyword: string
  status: string
}>()

const emit = defineEmits<{
  'update:keyword': [value: string]
  'update:status': [value: string]
  submit: []
  'create-ai': [origin: DialogTriggerOrigin]
  create: [origin: DialogTriggerOrigin]
  'open-gantt': [origin: DialogTriggerOrigin]
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

function emitAiCreateOrigin(event: MouseEvent) {
  const trigger = event.currentTarget as HTMLElement | null

  if (!trigger) {
    return
  }

  const rect = trigger.getBoundingClientRect()
  emit('create-ai', {
    x: rect.left + rect.width / 2,
    y: rect.top + rect.height / 2,
  })
}

function emitGanttOrigin(event: MouseEvent) {
  const trigger = event.currentTarget as HTMLElement | null

  if (!trigger) {
    return
  }

  const rect = trigger.getBoundingClientRect()
  emit('open-gantt', {
    x: rect.left + rect.width / 2,
    y: rect.top + rect.height / 2,
  })
}
</script>

<template>
  <section :class="['project-filters', { 'project-filters--embedded': embedded }]">
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
        aria-label="AI 创建项目"
        class="project-filters__ai-create"
        data-testid="open-ai-project-create"
        @click="emitAiCreateOrigin"
      >
        <span class="project-filters__ai-create-mark">AI</span>
        <span>AI 创建</span>
      </button>

      <button
        type="button"
        aria-label="打开项目甘特图"
        class="project-filters__overview"
        data-testid="open-project-gantt"
        @click="emitGanttOrigin"
      >
        <svg aria-hidden="true" class="project-filters__overview-icon" viewBox="0 0 24 24">
          <path
            d="M5 6.5h14M5 12h14M5 17.5h14M7.5 8v8M12 10v6M16.5 7v9"
            fill="none"
            stroke="currentColor"
            stroke-linecap="round"
            stroke-linejoin="round"
            stroke-width="1.8"
          />
        </svg>
        <span>项目甘特图</span>
      </button>

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
  grid-template-columns: minmax(220px, 280px) minmax(0, 1fr) auto;
  gap: 14px;
  padding: 18px;
  border: 1px solid var(--meta-surface-border);
  border-radius: 18px;
  background: var(--meta-surface-bg), var(--meta-surface-glow), var(--card-sheen);
  box-shadow: var(--meta-surface-shadow);
}

.project-filters--embedded {
  padding: 0;
  border: 0;
  border-radius: 0;
  background: none;
  box-shadow: none;
  backdrop-filter: none;
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
  border: 1px solid var(--dialog-control-border);
  border-radius: 12px;
  padding: 0 14px;
  background: var(--dialog-control-bg);
  box-shadow: var(--dialog-control-shadow);
  color: var(--text-main);
  font: inherit;
}

.project-filters__input::placeholder {
  color: var(--text-muted);
}

.project-filters__input:focus {
  border-color: var(--accent-line);
  box-shadow:
    var(--dialog-control-shadow),
    0 0 0 4px color-mix(in srgb, var(--accent-start) 10%, transparent);
}

.project-filters__status-bar {
  min-height: 46px;
  display: flex;
  flex-wrap: nowrap;
  align-items: center;
  gap: 10px;
  padding: 0;
  overflow-x: auto;
  overflow-y: hidden;
  border: 1px solid transparent;
  border-radius: 14px;
  background: transparent;
  box-shadow: none;
}

.project-filters__status-pill {
  flex: 0 0 auto;
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
  border-color: var(--dialog-control-border);
  background: var(--dialog-control-bg);
  color: var(--text-soft);
}

.project-filters__status-pill--pending {
  border-color: var(--work-status-pending-border);
  background: var(--work-status-pending-bg);
  color: var(--work-status-pending-color);
}

.project-filters__status-pill--active {
  border-color: var(--work-status-active-border);
  background: var(--work-status-active-bg);
  color: var(--work-status-active-color);
}

.project-filters__status-pill--done {
  border-color: var(--work-status-done-border);
  background: var(--work-status-done-bg);
  color: var(--work-status-done-color);
}

.project-filters__status-pill--delayed {
  border-color: var(--work-status-delayed-border);
  background: var(--work-status-delayed-bg);
  color: var(--work-status-delayed-color);
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
  border-color: var(--work-status-pending-border);
  background: var(--work-status-pending-strong);
  color: var(--work-status-pending-contrast);
}

.project-filters__status-pill--active.project-filters__status-pill--selected {
  border-color: var(--work-status-active-border);
  background: var(--work-status-active-strong);
  color: var(--work-status-active-contrast);
}

.project-filters__status-pill--done.project-filters__status-pill--selected {
  border-color: var(--work-status-done-border);
  background: var(--work-status-done-strong);
  color: var(--work-status-done-contrast);
}

.project-filters__status-pill--delayed.project-filters__status-pill--selected {
  border-color: var(--work-status-delayed-border);
  background: var(--work-status-delayed-strong);
  color: var(--work-status-delayed-contrast);
}

.project-filters__actions {
  display: flex;
  align-items: end;
  gap: 10px;
}

.project-filters__ai-create,
.project-filters__overview {
  min-height: 46px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 10px;
  padding: 0 16px;
  border: 1px solid var(--dialog-control-border);
  border-radius: 14px;
  background: var(--dialog-control-bg);
  box-shadow: var(--dialog-control-shadow);
  color: var(--text-main);
  font: inherit;
  font-size: 0.88rem;
  font-weight: 700;
  letter-spacing: 0.04em;
  cursor: pointer;
  transition:
    transform 200ms ease-out,
    border-color 200ms ease-out,
    box-shadow 200ms ease-out,
    background 200ms ease-out,
    color 200ms ease-out;
}

.project-filters__ai-create {
  border-color: color-mix(in srgb, var(--accent-line) 32%, var(--dialog-control-border));
  background:
    linear-gradient(135deg, color-mix(in srgb, var(--accent-start) 10%, var(--dialog-control-bg)), color-mix(in srgb, var(--accent-end) 12%, var(--dialog-control-bg))),
    var(--dialog-control-bg);
}

.project-filters__overview {
  min-height: 46px;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 10px;
  padding: 0 16px;
  border: 1px solid var(--dialog-control-border);
  border-radius: 14px;
  background: var(--dialog-control-bg);
  box-shadow: var(--dialog-control-shadow);
  color: var(--text-main);
  font: inherit;
  font-size: 0.88rem;
  font-weight: 700;
  letter-spacing: 0.04em;
  cursor: pointer;
  transition:
    transform 200ms ease-out,
    border-color 200ms ease-out,
    box-shadow 200ms ease-out,
    background 200ms ease-out,
    color 200ms ease-out;
}

.project-filters__ai-create:hover,
.project-filters__overview:hover {
  transform: translateY(-2px);
  border-color: var(--accent-line);
  box-shadow: var(--shadow-panel-hover);
}

.project-filters__ai-create:focus-visible,
.project-filters__overview:focus-visible {
  outline: none;
  transform: translateY(-2px);
  box-shadow:
    var(--shadow-panel-hover),
    0 0 0 4px color-mix(in srgb, var(--accent-end) 12%, transparent);
}

.project-filters__overview-icon {
  width: 18px;
  height: 18px;
  color: var(--accent-start);
}

.project-filters__ai-create-mark {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 30px;
  height: 30px;
  padding: 0 8px;
  border-radius: 999px;
  background: linear-gradient(135deg, color-mix(in srgb, var(--accent-start) 78%, #183b7a), color-mix(in srgb, var(--accent-end) 72%, #08182f));
  color: #f7fbff;
  font-size: 0.74rem;
  letter-spacing: 0.08em;
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
  border: 1px solid var(--meta-surface-border);
  border-radius: 10px;
  background: var(--meta-surface-bg), var(--meta-surface-glow), var(--card-sheen);
  color: var(--text-main);
  font-size: 0.75rem;
  font-weight: 600;
  line-height: 1;
  white-space: nowrap;
  box-shadow: var(--meta-surface-shadow);
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
    flex-wrap: wrap;
  }

  .project-filters__status-bar {
    gap: 8px;
  }

  .project-filters__ai-create,
  .project-filters__overview {
    flex: 1 1 180px;
  }
}
</style>
