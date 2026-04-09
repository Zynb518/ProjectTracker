<script setup lang="ts">
import type { ProjectNode } from '@/types/node'
import type { Subtask } from '@/types/subtask'

type ActionKey = 'edit' | 'remove' | 'reopen'

interface ManagementAction {
  key: ActionKey
  label: string
  tone: 'danger' | 'disabled' | 'neutral' | 'warning'
  disabled?: boolean
}

type PriorityTone = 'high' | 'low' | 'medium' | 'unknown'

withDefaults(defineProps<{
  canManage: boolean
  embedded?: boolean
  selectedNode: ProjectNode | null
  subtasks: Subtask[]
}>(), {
  embedded: false,
})

const emit = defineEmits<{
  create: []
  edit: [subtaskId: number]
  history: [subtaskId: number]
  progress: [subtaskId: number]
  remove: [subtaskId: number]
  reopen: [subtaskId: number]
}>()

function priorityLabel(priority: number) {
  return (
    {
      1: '低',
      2: '中',
      3: '高',
    }[priority] ?? '未设定'
  )
}

function priorityTone(priority: number): PriorityTone {
  if (priority === 1) {
    return 'low'
  }

  if (priority === 2) {
    return 'medium'
  }

  if (priority === 3) {
    return 'high'
  }

  return 'unknown'
}

function managementActions(subtask: Subtask): ManagementAction[] {
  const isCompleted = subtask.progress_percent >= 100

  return [
    {
      key: 'edit',
      label: '编辑',
      tone: isCompleted ? 'disabled' : 'neutral',
      disabled: isCompleted,
    },
    {
      key: 'reopen',
      label: '撤销完成',
      tone: isCompleted ? 'warning' : 'disabled',
      disabled: !isCompleted,
    },
    { key: 'remove', label: '删除', tone: 'danger' },
  ]
}

function emitAction(key: ActionKey, subtaskId: number) {
  if (key === 'edit') {
    emit('edit', subtaskId)
    return
  }

  if (key === 'reopen') {
    emit('reopen', subtaskId)
    return
  }

  emit('remove', subtaskId)
}
</script>

<template>
  <section :class="['subtask-table', { 'subtask-table--embedded': embedded }]">
    <header class="subtask-table__header">
      <div>
        <p>任务列表</p>
      </div>
      <div class="subtask-table__header-actions">
        <strong>{{ subtasks.length }} 项</strong>
        <button
          v-if="canManage"
          data-testid="create-subtask"
          aria-label="新建子任务"
          class="subtask-table__create"
          data-tooltip="新建子任务"
          type="button"
          @click="$emit('create')"
        >
          <svg aria-hidden="true" class="subtask-table__create-icon" viewBox="0 0 24 24">
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
    </header>

    <article v-for="subtask in subtasks" :key="subtask.id" class="subtask-table__row">
      <div class="subtask-table__content">
        <strong class="subtask-table__title" :title="subtask.name">{{ subtask.name }}</strong>
        <p class="subtask-table__description" :title="subtask.description || '当前子任务未填写说明。'">
          {{ subtask.description || '当前子任务未填写说明。' }}
        </p>
      </div>

      <div class="subtask-table__metrics">
        <span class="subtask-table__metric-chip">
          {{ subtask.responsible_real_name || '未分配' }}
        </span>
        <span
          :class="[
            'subtask-table__priority',
            `subtask-table__priority--${priorityTone(subtask.priority)}`,
          ]"
        >
          {{ priorityLabel(subtask.priority) }}优先级
        </span>
        <div class="subtask-table__progress" :aria-label="`进度 ${subtask.progress_percent}%`">
          <span class="subtask-table__progress-label">进度</span>
          <div class="subtask-table__progress-track">
            <span
              class="subtask-table__progress-fill"
              :style="{ width: `${subtask.progress_percent}%` }"
            />
          </div>
          <strong class="subtask-table__progress-value">{{ subtask.progress_percent }}%</strong>
        </div>
      </div>

      <div class="subtask-table__actions">
        <div class="subtask-table__flow-actions">
          <button
            :data-testid="`subtask-progress-${subtask.id}`"
            class="subtask-table__flow-button"
            type="button"
            @click="$emit('progress', subtask.id)"
          >
            更新进度
          </button>
          <button
            :data-testid="`subtask-history-${subtask.id}`"
            class="subtask-table__flow-button"
            type="button"
            @click="$emit('history', subtask.id)"
          >
            历史记录
          </button>
        </div>

        <div v-if="canManage" class="subtask-table__management-actions">
          <button
            v-for="action in managementActions(subtask)"
            :key="action.key"
            :data-testid="`subtask-${action.key}-${subtask.id}`"
            :aria-label="action.label"
            :class="['subtask-table__icon-action', `subtask-table__icon-action--${action.tone}`]"
            :data-tooltip="action.label"
            :disabled="action.disabled"
            type="button"
            @click="emitAction(action.key, subtask.id)"
          >
            <template v-if="action.key === 'reopen'">
              <svg aria-hidden="true" class="subtask-table__icon" viewBox="0 0 24 24">
                <path
                  d="M8 9H4V5"
                  fill="none"
                  stroke="currentColor"
                  stroke-linecap="round"
                  stroke-linejoin="round"
                  stroke-width="1.7"
                />
                <path
                  d="M5.2 12a6.8 6.8 0 1 0 2-4.8L4 9"
                  fill="none"
                  stroke="currentColor"
                  stroke-linecap="round"
                  stroke-linejoin="round"
                  stroke-width="1.7"
                />
              </svg>
            </template>
            <template v-else-if="action.key === 'edit'">
              <svg aria-hidden="true" class="subtask-table__icon" viewBox="0 0 24 24">
                <path
                  d="M4 20h4l10.5-10.5a2.12 2.12 0 0 0-3-3L5.5 17v3Z"
                  fill="none"
                  stroke="currentColor"
                  stroke-linecap="round"
                  stroke-linejoin="round"
                  stroke-width="1.7"
                />
                <path
                  d="m13.5 6.5 4 4"
                  fill="none"
                  stroke="currentColor"
                  stroke-linecap="round"
                  stroke-linejoin="round"
                  stroke-width="1.7"
                />
              </svg>
            </template>
            <template v-else>
              <svg aria-hidden="true" class="subtask-table__icon" viewBox="0 0 24 24">
                <path
                  d="M4 7h16"
                  fill="none"
                  stroke="currentColor"
                  stroke-linecap="round"
                  stroke-width="1.7"
                />
                <path
                  d="M9 7V5.5A1.5 1.5 0 0 1 10.5 4h3A1.5 1.5 0 0 1 15 5.5V7"
                  fill="none"
                  stroke="currentColor"
                  stroke-linecap="round"
                  stroke-linejoin="round"
                  stroke-width="1.7"
                />
                <path
                  d="M7.5 7.5 8.4 19a2 2 0 0 0 2 1.85h3.2a2 2 0 0 0 2-1.85l.9-11.5"
                  fill="none"
                  stroke="currentColor"
                  stroke-linecap="round"
                  stroke-linejoin="round"
                  stroke-width="1.7"
                />
                <path
                  d="M10 11v5M14 11v5"
                  fill="none"
                  stroke="currentColor"
                  stroke-linecap="round"
                  stroke-width="1.7"
                />
              </svg>
            </template>
          </button>
        </div>
      </div>
    </article>

    <p v-if="subtasks.length === 0" class="subtask-table__empty">当前节点下还没有子任务。</p>
  </section>
</template>

<style scoped>
.subtask-table {
  display: grid;
  gap: 12px;
  min-width: 0;
  padding: 18px;
  border: 1px solid var(--border-soft);
  border-radius: 18px;
  background: var(--glass-bg);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
  overflow-x: hidden;
}

.subtask-table--embedded {
  padding: 0;
  border: none;
  border-radius: 0;
  background: none;
  box-shadow: none;
  backdrop-filter: none;
}

.subtask-table__header {
  display: flex;
  align-items: start;
  justify-content: space-between;
  gap: 16px;
}

.subtask-table__header-actions {
  display: grid;
  justify-items: end;
  gap: 10px;
}

.subtask-table__create {
  position: relative;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 44px;
  height: 44px;
  border: 1px solid color-mix(in srgb, var(--accent-start) 28%, transparent);
  border-radius: 14px;
  padding: 0;
  background: var(--gradient-primary);
  color: var(--text-inverse);
  box-shadow: 0 16px 28px rgba(10, 102, 255, 0.2);
}

.subtask-table__create:hover {
  transform: translateY(-2px);
  box-shadow: var(--shadow-panel-hover);
}

.subtask-table__create:focus-visible {
  outline: none;
  transform: translateY(-2px);
  box-shadow:
    var(--shadow-panel-hover),
    0 0 0 4px color-mix(in srgb, var(--accent-end) 18%, transparent);
}

.subtask-table__create-icon {
  width: 18px;
  height: 18px;
}

.subtask-table__create[data-tooltip]::before,
.subtask-table__create[data-tooltip]::after {
  position: absolute;
  left: 50%;
  opacity: 0;
  pointer-events: none;
  transition:
    opacity 180ms ease-out,
    transform 180ms ease-out;
}

.subtask-table__create[data-tooltip]::before {
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

.subtask-table__create[data-tooltip]::after {
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

.subtask-table__create[data-tooltip]:hover::before,
.subtask-table__create[data-tooltip]:hover::after,
.subtask-table__create[data-tooltip]:focus-visible::before,
.subtask-table__create[data-tooltip]:focus-visible::after {
  opacity: 1;
  transform: translate(-50%, 0);
}

.subtask-table__create[data-tooltip]:hover::before,
.subtask-table__create[data-tooltip]:focus-visible::before {
  transform: translate(-50%, 0) rotate(-45deg);
}

.subtask-table__header p,
.subtask-table__header span {
  margin: 0;
}

.subtask-table__header p {
  font-weight: 700;
}

.subtask-table__row {
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  grid-template-areas:
    'content content'
    'metrics actions';
  gap: 14px 16px;
  min-width: 0;
  padding: 14px;
  border: 1px solid var(--border-soft);
  border-radius: 14px;
  background: var(--drawer-item-bg);
  box-shadow: var(--dialog-control-shadow);
}

.subtask-table__row strong,
.subtask-table__row p {
  margin: 0;
}

.subtask-table__content {
  grid-area: content;
  min-width: 0;
}

.subtask-table__title {
  display: block;
  overflow: hidden;
  font-size: 1.04rem;
  font-weight: 700;
  line-height: 1.25;
  white-space: nowrap;
  text-overflow: ellipsis;
  color: color-mix(in srgb, var(--text-main) 92%, var(--accent-end) 8%);
}

.subtask-table__description {
  margin-top: 6px;
  overflow: hidden;
  color: var(--text-soft);
  line-height: 1.56;
  display: -webkit-box;
  -webkit-line-clamp: 2;
  -webkit-box-orient: vertical;
}

.subtask-table__metrics {
  grid-area: metrics;
  min-width: 0;
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 10px;
}

.subtask-table__metric-chip,
.subtask-table__priority {
  min-width: 0;
  display: inline-flex;
  align-items: center;
  padding: 9px 12px;
  border: 1px solid var(--dialog-control-border);
  border-radius: 12px;
  font-size: 0.9rem;
  font-weight: 600;
  line-height: 1.1;
  box-shadow: var(--dialog-control-shadow);
}

.subtask-table__metric-chip {
  background: var(--dialog-control-bg);
  color: var(--text-soft);
}

.subtask-table__priority {
  justify-content: center;
}

.subtask-table__priority--low {
  background: color-mix(in srgb, var(--accent-success) 14%, var(--dialog-control-bg));
  border-color: color-mix(in srgb, var(--accent-success) 28%, transparent);
  color: color-mix(in srgb, var(--accent-success) 72%, var(--text-main));
}

.subtask-table__priority--medium {
  background: color-mix(in srgb, var(--accent-start) 16%, var(--dialog-control-bg));
  border-color: color-mix(in srgb, var(--accent-start) 28%, transparent);
  color: color-mix(in srgb, var(--accent-start) 70%, var(--text-main));
}

.subtask-table__priority--high {
  background: color-mix(in srgb, var(--accent-warning) 16%, var(--dialog-control-bg));
  border-color: color-mix(in srgb, var(--accent-warning) 28%, transparent);
  color: color-mix(in srgb, var(--accent-warning) 74%, var(--text-main));
}

.subtask-table__priority--unknown {
  background: var(--dialog-control-bg);
  color: var(--text-soft);
}

.subtask-table__progress {
  grid-column: 1 / -1;
  display: grid;
  grid-template-columns: auto minmax(0, 1fr) auto;
  align-items: center;
  gap: 10px;
  min-width: 0;
  padding: 10px 12px;
  border: 1px solid var(--dialog-control-border);
  border-radius: 12px;
  background: var(--dialog-control-bg);
  box-shadow: var(--dialog-control-shadow);
}

.subtask-table__progress-label {
  color: var(--text-soft);
  font-size: 0.82rem;
  font-weight: 600;
  letter-spacing: 0.06em;
  text-transform: uppercase;
}

.subtask-table__progress-track {
  position: relative;
  height: 10px;
  overflow: hidden;
  border-radius: 999px;
  background: color-mix(in srgb, var(--panel-bg) 76%, var(--dialog-control-bg));
  box-shadow: inset 0 0 0 1px color-mix(in srgb, var(--dialog-control-border) 92%, transparent);
}

.subtask-table__progress-fill {
  display: block;
  height: 100%;
  min-width: 0;
  border-radius: inherit;
  background: linear-gradient(90deg, var(--accent-start), var(--accent-end));
  box-shadow: 0 0 16px color-mix(in srgb, var(--accent-end) 28%, transparent);
}

.subtask-table__progress-value {
  color: var(--text-main);
  font-size: 0.92rem;
  font-weight: 700;
}

.subtask-table__actions {
  grid-area: actions;
  min-width: 0;
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  justify-content: flex-end;
  gap: 10px;
}

.subtask-table__flow-actions,
.subtask-table__management-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.subtask-table__flow-button {
  border: 1px solid var(--border-soft);
  border-radius: 10px;
  padding: 8px 12px;
  background: color-mix(in srgb, var(--accent-start) 12%, transparent);
  color: var(--text-main);
  cursor: pointer;
}

.subtask-table__flow-button:hover,
.subtask-table__flow-button:focus-visible {
  transform: translateY(-1px);
  box-shadow: var(--shadow-panel-hover);
}

.subtask-table__flow-button:focus-visible {
  outline: none;
}

.subtask-table__icon-action {
  position: relative;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 40px;
  min-height: 40px;
  border: 1px solid transparent;
  border-radius: 12px;
  padding: 0;
  background: color-mix(in srgb, var(--panel-bg) 94%, transparent);
  color: var(--text-main);
  cursor: pointer;
  transition:
    transform 200ms ease-out,
    box-shadow 200ms ease-out,
    border-color 200ms ease-out,
    background-color 200ms ease-out,
    color 200ms ease-out;
}

.subtask-table__icon-action:hover,
.subtask-table__icon-action:focus-visible {
  transform: translateY(-2px);
  box-shadow: var(--shadow-panel-hover);
}

.subtask-table__icon-action:hover:disabled,
.subtask-table__icon-action:focus-visible:disabled {
  transform: none;
  box-shadow: none;
}

.subtask-table__icon-action:focus-visible {
  outline: none;
}

.subtask-table__icon {
  width: 18px;
  height: 18px;
}

.subtask-table__icon-action--neutral {
  border-color: color-mix(in srgb, var(--accent-end) 18%, transparent);
  background: color-mix(in srgb, var(--accent-end) 10%, transparent);
  color: color-mix(in srgb, var(--text-main) 92%, var(--accent-end));
}

.subtask-table__icon-action--warning {
  border-color: color-mix(in srgb, var(--accent-warning) 30%, transparent);
  background: color-mix(in srgb, var(--accent-warning) 14%, transparent);
  color: var(--accent-warning);
}

.subtask-table__icon-action--danger {
  border-color: color-mix(in srgb, #ff6b7a 34%, transparent);
  background: color-mix(in srgb, #ff6b7a 14%, transparent);
  color: #ff6b7a;
}

.subtask-table__icon-action--disabled,
.subtask-table__icon-action:disabled {
  border-color: color-mix(in srgb, var(--border-soft) 92%, transparent);
  background: color-mix(in srgb, var(--panel-bg) 82%, transparent);
  color: var(--text-soft);
  opacity: 0.42;
  cursor: not-allowed;
}

.subtask-table__icon-action[data-tooltip]::before,
.subtask-table__icon-action[data-tooltip]::after {
  position: absolute;
  left: 50%;
  opacity: 0;
  pointer-events: none;
  transition:
    opacity 180ms ease-out,
    transform 180ms ease-out;
}

.subtask-table__icon-action[data-tooltip]::before {
  content: '';
  bottom: calc(100% + 4px);
  width: 10px;
  height: 10px;
  border-left: 1px solid var(--border-soft);
  border-bottom: 1px solid var(--border-soft);
  background: color-mix(in srgb, var(--panel-bg) 96%, transparent);
  transform: translate(-50%, 6px) rotate(-45deg);
  z-index: 1;
}

.subtask-table__icon-action[data-tooltip]::after {
  content: attr(data-tooltip);
  bottom: calc(100% + 10px);
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

.subtask-table__icon-action[data-tooltip]:hover::before,
.subtask-table__icon-action[data-tooltip]:hover::after,
.subtask-table__icon-action[data-tooltip]:focus-visible::before,
.subtask-table__icon-action[data-tooltip]:focus-visible::after {
  opacity: 1;
  transform: translate(-50%, 0);
}

.subtask-table__icon-action[data-tooltip]:hover::before,
.subtask-table__icon-action[data-tooltip]:focus-visible::before {
  transform: translate(-50%, 0) rotate(-45deg);
}

.subtask-table__empty {
  margin: 0;
  color: var(--text-soft);
}

@media (max-width: 900px) {
  .subtask-table__row {
    grid-template-columns: minmax(0, 1fr);
    grid-template-areas:
      'content'
      'metrics'
      'actions';
  }

  .subtask-table__actions {
    justify-content: flex-start;
  }
}

@media (max-width: 640px) {
  .subtask-table__metrics,
  .subtask-table__progress {
    grid-template-columns: 1fr;
  }

  .subtask-table__flow-actions,
  .subtask-table__management-actions {
    width: 100%;
  }
}
</style>
