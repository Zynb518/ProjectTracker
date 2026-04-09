<script setup lang="ts">
import { computed, reactive, watch } from 'vue'

import type { ProjectMember } from '@/types/member'
import type { SubtaskPayload } from '@/types/subtask'

const props = defineProps<{
  assignees: ProjectMember[]
  defaultResponsibleUserId?: number
  initialValue: SubtaskPayload
  maxDate?: string
  minDate?: string
  mode?: 'create' | 'edit'
  modelValue: boolean
}>()

const emit = defineEmits<{
  submit: [payload: SubtaskPayload]
  'update:modelValue': [value: boolean]
}>()

const form = reactive<SubtaskPayload>({
  name: '',
  description: '',
  planned_start_date: '',
  planned_end_date: '',
  priority: 2,
  responsible_user_id: undefined,
})

function ensureResponsibleUser() {
  if (form.responsible_user_id !== undefined) {
    return
  }

  const defaultAssignee = props.assignees.find(
    (assignee) => assignee.user_id === props.defaultResponsibleUserId,
  )

  form.responsible_user_id = defaultAssignee?.user_id ?? props.assignees[0]?.user_id
}

const responsibleUserValue = computed({
  get: () => {
    ensureResponsibleUser()

    return String(form.responsible_user_id ?? '')
  },
  set: (value: string) => {
    form.responsible_user_id = Number(value)
  },
})

watch(
  () => props.initialValue,
  (value) => {
    form.name = value.name
    form.description = value.description
    form.planned_start_date = value.planned_start_date
    form.planned_end_date = value.planned_end_date
    form.priority = value.priority ?? 2
    form.responsible_user_id = value.responsible_user_id ?? props.defaultResponsibleUserId
  },
  { immediate: true, deep: true },
)

watch(
  () => props.assignees,
  () => {
    ensureResponsibleUser()
  },
  { immediate: true, deep: true },
)

function close() {
  emit('update:modelValue', false)
}

function submit() {
  ensureResponsibleUser()

  if (form.responsible_user_id === undefined) {
    return
  }

  emit('submit', {
    name: form.name,
    description: form.description,
    planned_start_date: form.planned_start_date,
    planned_end_date: form.planned_end_date,
    priority: form.priority,
    responsible_user_id: form.responsible_user_id,
  })
}
</script>

<template>
  <Transition appear name="subtask-dialog-fade">
    <div v-if="modelValue" class="subtask-form__backdrop">
      <section class="subtask-form">
        <header class="subtask-form__header">
          <div>
            <p class="subtask-form__eyebrow">节点任务</p>
            <h2>{{ mode === 'edit' ? '编辑子任务' : '新建子任务' }}</h2>
          </div>

          <button type="button" @click="close">关闭</button>
        </header>

        <div class="subtask-form__grid">
          <label>
            <span>子任务名称</span>
            <input v-model="form.name" aria-label="子任务名称" type="text" />
          </label>

          <label>
            <span>优先级</span>
            <select v-model.number="form.priority" aria-label="优先级">
              <option :value="1">低</option>
              <option :value="2">中</option>
              <option :value="3">高</option>
            </select>
          </label>

          <label>
            <span>负责人</span>
            <select v-model="responsibleUserValue" aria-label="负责人">
              <option
                v-for="member in assignees"
                :key="member.user_id"
                :value="String(member.user_id)"
              >
                {{ member.real_name }}
              </option>
            </select>
          </label>

          <label class="subtask-form__description">
            <span>子任务说明</span>
            <textarea v-model="form.description" aria-label="子任务说明" rows="4" />
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

        <footer class="subtask-form__footer">
          <button type="button" @click="close">取消</button>
          <button type="button" class="primary" @click="submit">
            {{ mode === 'edit' ? '保存子任务' : '创建子任务' }}
          </button>
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

.subtask-dialog-fade-enter-active .subtask-form,
.subtask-dialog-fade-leave-active .subtask-form {
  transition:
    opacity 280ms cubic-bezier(0.16, 1, 0.3, 1),
    transform 320ms cubic-bezier(0.16, 1, 0.3, 1);
}

.subtask-dialog-fade-enter-from,
.subtask-dialog-fade-leave-to {
  opacity: 0;
  backdrop-filter: blur(0px);
}

.subtask-dialog-fade-enter-from .subtask-form,
.subtask-dialog-fade-leave-to .subtask-form {
  opacity: 0;
  transform: translateY(18px) scale(0.96);
}

.subtask-form__backdrop {
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

.subtask-form__backdrop::before {
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

.subtask-form {
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

.subtask-form__header,
.subtask-form__footer {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
}

.subtask-form__header h2,
.subtask-form__header p {
  margin: 0;
}

.subtask-form__eyebrow {
  color: var(--text-soft);
  font-size: 0.78rem;
  font-weight: 600;
  letter-spacing: 0.14em;
  text-transform: uppercase;
}

.subtask-form__grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 16px;
}

.subtask-form__description {
  grid-column: 1 / -1;
}

.subtask-form__grid label {
  display: grid;
  gap: 8px;
}

.subtask-form__grid span {
  color: var(--text-soft);
  font-size: 0.88rem;
  font-weight: 600;
}

.subtask-form__grid input,
.subtask-form__grid select,
.subtask-form__grid textarea,
.subtask-form__header button,
.subtask-form__footer button {
  border: 1px solid var(--border-soft);
  border-radius: 12px;
  padding: 11px 14px;
  font: inherit;
}

.subtask-form__grid input,
.subtask-form__grid select,
.subtask-form__grid textarea {
  background: color-mix(in srgb, var(--panel-bg) 92%, transparent);
  color: var(--text-main);
}

.subtask-form__grid textarea {
  resize: vertical;
  min-height: 116px;
}

.subtask-form__header button,
.subtask-form__footer button {
  background: color-mix(in srgb, var(--panel-bg) 88%, transparent);
  color: var(--text-main);
  cursor: pointer;
}

.subtask-form__footer .primary {
  border-color: color-mix(in srgb, var(--accent-start) 28%, transparent);
  background: linear-gradient(135deg, var(--accent-start), var(--accent-end));
  color: #fff;
}

@media (max-width: 720px) {
  .subtask-form__grid {
    grid-template-columns: 1fr;
  }
}
</style>
