<script setup lang="ts">
import { computed } from 'vue'

import type { ProjectNode } from '@/types/node'

const props = defineProps<{
  node: ProjectNode
}>()

const emit = defineEmits<{
  complete: []
  edit: []
  remove: []
  reopen: []
  start: []
}>()

type ActionKey = 'complete' | 'edit' | 'remove' | 'reopen' | 'start'

interface ActionButton {
  key: ActionKey
  tone: 'start' | 'success' | 'warning' | 'neutral' | 'danger' | 'disabled'
  disabled?: boolean
  label: string
}

const actions = computed<ActionButton[]>(() => {
  if (props.node.status === 1) {
    return [
      { key: 'start', label: '开始', tone: 'start' },
      { key: 'edit', label: '编辑', tone: 'neutral' },
      { key: 'remove', label: '删除', tone: 'danger' },
    ]
  }

  if (props.node.status === 2 || props.node.status === 4) {
    return [
      { key: 'complete', label: '完成', tone: 'success' },
      { key: 'edit', label: '编辑', tone: 'neutral' },
      { key: 'remove', label: '删除', tone: 'danger' },
    ]
  }

  if (props.node.status === 3) {
    return [
      { key: 'reopen', label: '撤销完成', tone: 'warning' },
      { key: 'edit', label: '编辑', tone: 'disabled', disabled: true },
      { key: 'remove', label: '删除', tone: 'disabled', disabled: true },
    ]
  }

  return [
    { key: 'edit', label: '编辑', tone: 'neutral' },
    { key: 'remove', label: '删除', tone: 'danger' },
  ]
})

function emitAction(key: ActionKey) {
  if (key === 'edit') {
    emit('edit')
    return
  }

  if (key === 'start') {
    emit('start')
    return
  }

  if (key === 'complete') {
    emit('complete')
    return
  }

  if (key === 'reopen') {
    emit('reopen')
    return
  }

  emit('remove')
}
</script>

<template>
  <div class="node-actions__main" @click.stop>
    <button
      v-for="action in actions"
      :key="action.key"
      :data-testid="`node-action-${action.key}-${node.id}`"
      :aria-label="action.label"
      :class="['node-actions__button', `node-actions__button--${action.tone}`]"
      :data-tooltip="action.label"
      :disabled="action.disabled"
      type="button"
      @click.stop="emitAction(action.key)"
    >
      <template v-if="action.key === 'start'">
        <svg aria-hidden="true" class="node-actions__icon" viewBox="0 0 24 24">
          <path d="M8 6.5v11l8-5.5-8-5.5Z" fill="currentColor" />
        </svg>
      </template>
      <template v-else-if="action.key === 'complete'">
        <svg aria-hidden="true" class="node-actions__icon" viewBox="0 0 24 24">
          <path
            d="M12 21a9 9 0 1 0 0-18 9 9 0 0 0 0 18Z"
            fill="none"
            stroke="currentColor"
            stroke-width="1.7"
          />
          <path
            d="m8.5 12.2 2.3 2.4 4.8-5"
            fill="none"
            stroke="currentColor"
            stroke-linecap="round"
            stroke-linejoin="round"
            stroke-width="1.9"
          />
        </svg>
      </template>
      <template v-else-if="action.key === 'reopen'">
        <svg aria-hidden="true" class="node-actions__icon" viewBox="0 0 24 24">
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
        <svg aria-hidden="true" class="node-actions__icon" viewBox="0 0 24 24">
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
        <svg aria-hidden="true" class="node-actions__icon" viewBox="0 0 24 24">
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
</template>

<style scoped>
.node-actions__main {
  display: grid;
  width: max-content;
  gap: 8px;
  grid-template-columns: repeat(3, minmax(0, 40px));
}

.node-actions__button {
  position: relative;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-height: 40px;
  min-width: 40px;
  border: 1px solid transparent;
  border-radius: 12px;
  padding: 0;
  background: color-mix(in srgb, var(--panel-bg) 94%, transparent);
  color: var(--text-main);
  cursor: pointer;
  transition:
    transform 200ms ease-out,
    box-shadow 200ms ease-out,
    opacity 200ms ease-out,
    border-color 200ms ease-out,
    background-color 200ms ease-out,
    color 200ms ease-out;
}

.node-actions__button:hover:not(:disabled) {
  transform: translateY(-2px);
  box-shadow: var(--shadow-panel-hover);
}

.node-actions__button[data-tooltip]::before,
.node-actions__button[data-tooltip]::after {
  position: absolute;
  left: 50%;
  opacity: 0;
  pointer-events: none;
  transition:
    opacity 180ms ease-out,
    transform 180ms ease-out;
}

.node-actions__button[data-tooltip]::before {
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

.node-actions__button[data-tooltip]::after {
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

.node-actions__button[data-tooltip]:hover::before,
.node-actions__button[data-tooltip]:hover::after,
.node-actions__button[data-tooltip]:focus-visible::before,
.node-actions__button[data-tooltip]:focus-visible::after {
  opacity: 1;
  transform: translate(-50%, 0);
}

.node-actions__button[data-tooltip]:hover::before,
.node-actions__button[data-tooltip]:focus-visible::before {
  transform: translate(-50%, 0) rotate(-45deg);
}

.node-actions__button--start {
  border-color: color-mix(in srgb, var(--accent-start) 28%, transparent);
  background: color-mix(in srgb, var(--accent-start) 12%, transparent);
  color: var(--accent-start);
}

.node-actions__button--success {
  border-color: color-mix(in srgb, var(--accent-success) 28%, transparent);
  background: color-mix(in srgb, var(--accent-success) 12%, transparent);
  color: var(--accent-success);
}

.node-actions__button--warning {
  border-color: color-mix(in srgb, var(--accent-warning) 28%, transparent);
  background: color-mix(in srgb, var(--accent-warning) 16%, transparent);
  color: var(--accent-warning);
}

.node-actions__button--neutral {
  border-color: var(--border-soft);
  background: color-mix(in srgb, var(--panel-bg) 90%, transparent);
  color: var(--text-main);
}

.node-actions__button--danger {
  border-color: color-mix(in srgb, var(--accent-danger) 28%, transparent);
  background: color-mix(in srgb, var(--accent-danger) 12%, transparent);
  color: var(--accent-danger);
}

.node-actions__button--disabled,
.node-actions__button:disabled {
  border-color: var(--border-soft);
  background: color-mix(in srgb, var(--panel-bg) 90%, transparent);
  color: var(--text-soft);
  cursor: not-allowed;
  opacity: 0.42;
  box-shadow: none;
  transform: none;
}

.node-actions__icon {
  width: 18px;
  height: 18px;
}
</style>
