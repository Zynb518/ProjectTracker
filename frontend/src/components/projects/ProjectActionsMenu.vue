<script setup lang="ts">
import { computed } from 'vue'

import type { ProjectListItem } from '@/types/project'

const props = defineProps<{
  project: ProjectListItem
}>()

const emit = defineEmits<{
  edit: []
  start: []
  complete: []
  reopen: []
  remove: []
}>()

type ActionKey = 'edit' | 'start' | 'complete' | 'reopen' | 'remove'

interface ActionButton {
  key: ActionKey
  label: string
  tone: 'primary' | 'neutral' | 'danger' | 'disabled'
  disabled?: boolean
}

const actions = computed<ActionButton[]>(() => {
  if (props.project.status === 1) {
    return [
      { key: 'start', label: '开始', tone: 'primary' },
      { key: 'edit', label: '编辑', tone: 'neutral' },
      { key: 'remove', label: '删除', tone: 'danger' },
    ]
  }

  if (props.project.status === 2 || props.project.status === 4) {
    return [
      { key: 'complete', label: '完成', tone: 'primary' },
      { key: 'edit', label: '编辑', tone: 'neutral' },
      { key: 'remove', label: '删除', tone: 'danger' },
    ]
  }

  if (props.project.status === 3) {
    return [
      { key: 'reopen', label: '撤销完成', tone: 'primary' },
      { key: 'edit', label: '编辑', tone: 'disabled', disabled: true },
      { key: 'remove', label: '删除', tone: 'danger' },
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
  <div
    :class="['project-actions', { 'project-actions--pair': actions.length === 2 }]"
    @click.stop
  >
    <button
      v-for="(action, index) in actions"
      :key="action.key"
      :class="[
        'project-actions__button',
        `project-actions__button--${action.tone}`,
        { 'project-actions__button--primary-row': index === 0 },
      ]"
      :disabled="action.disabled"
      type="button"
      @click.stop="emitAction(action.key)"
    >
      {{ action.label }}
    </button>
  </div>
</template>

<style scoped>
.project-actions {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 8px;
  margin-top: auto;
}

.project-actions__button {
  border: 1px solid transparent;
  border-radius: 10px;
  padding: 10px 12px;
  font: inherit;
  font-weight: 600;
  cursor: pointer;
  transition:
    transform 200ms ease-out,
    background-color 200ms ease-out,
    color 200ms ease-out,
    box-shadow 200ms ease-out,
    opacity 200ms ease-out,
    border-color 200ms ease-out;
}

.project-actions__button:hover:not(:disabled) {
  transform: translateY(-2px);
  box-shadow: var(--shadow-panel-hover);
}

.project-actions__button--primary-row {
  grid-column: 1 / -1;
}

.project-actions--pair .project-actions__button:not(.project-actions__button--primary-row) {
  grid-column: 1 / -1;
}

.project-actions__button--primary {
  background: var(--gradient-primary);
  color: var(--text-inverse);
  box-shadow: 0 14px 24px rgba(10, 102, 255, 0.2);
}

.project-actions__button--neutral {
  border-color: var(--border-soft);
  background: color-mix(in srgb, var(--panel-bg) 88%, transparent);
  color: var(--text-main);
}

.project-actions__button--danger {
  background: color-mix(in srgb, var(--accent-danger) 16%, transparent);
  color: var(--accent-danger);
}

.project-actions__button--disabled {
  border-color: var(--border-soft);
  background: color-mix(in srgb, var(--accent-neutral) 16%, transparent);
  color: var(--text-muted);
}

.project-actions__button:disabled {
  cursor: not-allowed;
  box-shadow: none;
  opacity: 0.72;
}
</style>
