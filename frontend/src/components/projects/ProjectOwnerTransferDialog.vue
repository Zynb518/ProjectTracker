<script setup lang="ts">
import { ref, watch } from 'vue'

import { getErrorMessage } from '@/api/http'
import { listProjectOwnerCandidates } from '@/api/projects'
import { useNotificationStore } from '@/stores/notifications'
import type { ProjectOwnerCandidate } from '@/types/project'
import { getSystemRoleLabel } from '@/utils/display'

const props = withDefaults(defineProps<{
  modelValue: boolean
  projectId: number
  isSubmitting?: boolean
}>(), {
  isSubmitting: false,
})

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  submit: [userId: number]
}>()

const keyword = ref('')
const candidates = ref<ProjectOwnerCandidate[]>([])
const selectedUserId = ref<number | null>(null)
const isLoading = ref(false)
const notificationStore = useNotificationStore()

async function loadCandidates() {
  isLoading.value = true

  try {
    const response = await listProjectOwnerCandidates(props.projectId, {
      keyword: keyword.value.trim(),
      page: 1,
      page_size: 20,
    })

    candidates.value = response.list

    if (!response.list.some((candidate) => candidate.id === selectedUserId.value)) {
      selectedUserId.value = null
    }
  } catch (error) {
    candidates.value = []
    selectedUserId.value = null
    notificationStore.notifyError(getErrorMessage(error, '负责人候选列表加载失败'))
  } finally {
    isLoading.value = false
  }
}

function closeDialog() {
  if (props.isSubmitting) {
    return
  }

  emit('update:modelValue', false)
}

function submitTransfer() {
  if (selectedUserId.value === null || props.isSubmitting) {
    return
  }

  emit('submit', selectedUserId.value)
}

watch(
  () => props.modelValue,
  (value) => {
    if (value) {
      keyword.value = ''
      selectedUserId.value = null
      void loadCandidates()
      return
    }

    keyword.value = ''
    candidates.value = []
    selectedUserId.value = null
  },
  { immediate: true },
)
</script>

<template>
  <div
    v-if="modelValue"
    class="owner-transfer__backdrop"
    data-testid="owner-transfer-dialog-backdrop"
    @click.self="closeDialog"
  >
    <section
      aria-labelledby="owner-transfer-title"
      aria-modal="true"
      class="owner-transfer"
      role="dialog"
    >
      <header class="owner-transfer__header">
        <div>
          <p>Owner Transfer</p>
          <h2 id="owner-transfer-title">转交项目负责人</h2>
        </div>
        <button type="button" @click="closeDialog">取消</button>
      </header>

      <p class="owner-transfer__copy">
        选择新的项目负责人。若目标用户当前不在项目成员中，系统会自动把对方加入项目成员。
      </p>

      <div class="owner-transfer__search">
        <input
          v-model="keyword"
          :disabled="isLoading || isSubmitting"
          placeholder="搜索用户名或姓名"
          type="text"
          @keydown.enter.prevent="loadCandidates"
        />
        <button :disabled="isLoading || isSubmitting" type="button" @click="loadCandidates">
          搜索
        </button>
      </div>

      <p v-if="isLoading" class="owner-transfer__state">候选负责人加载中...</p>
      <p v-else-if="candidates.length === 0" class="owner-transfer__state">暂无可转交的负责人候选</p>
      <div v-else class="owner-transfer__list">
        <button
          v-for="candidate in candidates"
          :key="candidate.id"
          :class="[
            'owner-transfer__candidate',
            { 'owner-transfer__candidate--selected': selectedUserId === candidate.id },
          ]"
          :data-testid="`owner-candidate-${candidate.id}`"
          :disabled="isSubmitting"
          type="button"
          @click="selectedUserId = candidate.id"
        >
          <div class="owner-transfer__candidate-copy">
            <strong>{{ candidate.real_name }}</strong>
            <span>@{{ candidate.username }}</span>
          </div>

          <div class="owner-transfer__candidate-meta">
            <span class="owner-transfer__pill">{{ getSystemRoleLabel(candidate.system_role) }}</span>
            <span class="owner-transfer__hint">
              {{ candidate.is_project_member ? '已是项目成员' : '将自动加入项目' }}
            </span>
          </div>
        </button>
      </div>

      <footer class="owner-transfer__footer">
        <button :disabled="isSubmitting" type="button" @click="closeDialog">取消</button>
        <button
          :disabled="selectedUserId === null || isSubmitting"
          class="owner-transfer__confirm"
          type="button"
          @click="submitTransfer"
        >
          确认转交
        </button>
      </footer>
    </section>
  </div>
</template>

<style scoped>
.owner-transfer__backdrop {
  position: fixed;
  inset: 0;
  z-index: 40;
  display: grid;
  place-items: center;
  padding: 24px;
  overflow: hidden;
  background:
    radial-gradient(circle at 16% 18%, rgba(10, 102, 255, 0.14), transparent 24%),
    radial-gradient(circle at 84% 76%, rgba(0, 194, 255, 0.12), transparent 26%),
    var(--overlay-backdrop);
  backdrop-filter: blur(16px);
}

.owner-transfer__backdrop::before {
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

.owner-transfer {
  position: relative;
  width: min(100%, 640px);
  max-height: min(760px, calc(100vh - 48px));
  display: grid;
  grid-template-rows: auto auto auto minmax(0, 1fr) auto;
  gap: 16px;
  padding: 24px;
  border: 1px solid var(--dialog-surface-border);
  border-radius: 18px;
  background: var(--dialog-surface-glow), var(--dialog-surface-bg);
  box-shadow: var(--dialog-surface-shadow);
  backdrop-filter: var(--backdrop-blur);
}

.owner-transfer__header,
.owner-transfer__search,
.owner-transfer__footer {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.owner-transfer__header h2,
.owner-transfer__header p,
.owner-transfer__copy,
.owner-transfer__state {
  margin: 0;
}

.owner-transfer__header div {
  display: grid;
  gap: 4px;
}

.owner-transfer__copy,
.owner-transfer__state,
.owner-transfer__hint {
  color: var(--text-soft);
}

.owner-transfer__search input {
  flex: 1;
}

.owner-transfer__list {
  min-height: 0;
  display: grid;
  align-content: start;
  gap: 10px;
  overflow-y: auto;
  padding-right: 6px;
}

.owner-transfer__candidate {
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  gap: 12px;
  align-items: center;
  padding: 14px 16px;
  border: 1px solid color-mix(in srgb, var(--border-soft) 82%, transparent);
  border-radius: 14px;
  background: color-mix(in srgb, var(--panel-bg) 90%, transparent);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.08);
  cursor: pointer;
  text-align: left;
  transition:
    transform 180ms ease,
    border-color 180ms ease,
    box-shadow 180ms ease,
    background 180ms ease;
}

.owner-transfer__candidate:hover {
  transform: translateY(-1px);
  border-color: color-mix(in srgb, var(--accent-start) 40%, var(--border-soft));
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.12),
    0 12px 24px rgba(9, 22, 43, 0.16);
}

.owner-transfer__candidate--selected {
  border-color: color-mix(in srgb, var(--accent-start) 62%, var(--border-soft));
  background: color-mix(in srgb, var(--accent-start) 12%, var(--panel-bg));
  box-shadow:
    inset 0 0 0 1px color-mix(in srgb, var(--accent-start) 30%, transparent),
    0 14px 28px rgba(9, 22, 43, 0.18);
}

.owner-transfer__candidate-copy,
.owner-transfer__candidate-meta {
  display: grid;
  gap: 4px;
}

.owner-transfer__candidate-copy strong,
.owner-transfer__candidate-copy span {
  margin: 0;
}

.owner-transfer__candidate-meta {
  justify-items: end;
}

.owner-transfer__pill {
  display: inline-flex;
  align-items: center;
  min-height: 28px;
  padding: 0 10px;
  border: 1px solid color-mix(in srgb, var(--accent-start) 28%, var(--border-soft));
  border-radius: 999px;
  background: color-mix(in srgb, var(--accent-start) 14%, transparent);
  font-size: 12px;
  font-weight: 600;
}

.owner-transfer__confirm {
  border-color: color-mix(in srgb, var(--accent-start) 36%, var(--dialog-control-border));
  background:
    linear-gradient(135deg, rgba(10, 102, 255, 0.22), rgba(0, 194, 255, 0.2)),
    var(--dialog-control-bg-strong);
}

input,
button {
  border-radius: 10px;
  padding: 10px 14px;
  font: inherit;
}

input {
  border: 1px solid var(--dialog-control-border);
  background: var(--dialog-control-bg);
  box-shadow: var(--dialog-control-shadow);
  color: var(--text-main);
}

button {
  border: 1px solid var(--dialog-control-border);
  background: var(--dialog-control-bg-strong);
  box-shadow: var(--dialog-control-shadow);
  color: var(--text-main);
}

button:disabled {
  cursor: not-allowed;
  opacity: 0.56;
}

@media (max-width: 720px) {
  .owner-transfer {
    grid-template-rows: auto auto auto minmax(0, 1fr) auto;
    padding: 20px;
  }

  .owner-transfer__candidate {
    grid-template-columns: 1fr;
  }

  .owner-transfer__candidate-meta {
    justify-items: start;
  }
}
</style>
