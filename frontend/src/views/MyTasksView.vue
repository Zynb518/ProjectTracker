<script setup lang="ts">
import { computed, onMounted, reactive, ref } from 'vue'

import { getErrorMessage } from '@/api/http'
import MyTaskBoard from '@/components/my-tasks/MyTaskBoard.vue'
import MyTaskFilters from '@/components/my-tasks/MyTaskFilters.vue'
import MyTaskProgressDialog from '@/components/my-tasks/MyTaskProgressDialog.vue'
import { listMySubtasks, listSubtaskProgressRecords, submitSubtaskProgress } from '@/api/subtasks'
import { useNotificationStore } from '@/stores/notifications'
import type { Subtask, SubtaskProgressRecord } from '@/types/subtask'

const filters = reactive({
  projectKeyword: '',
  appliedProjectKeyword: '',
  status: '',
})

const tasks = ref<Subtask[]>([])
const projectCandidates = ref<string[]>([])
const isLoading = ref(false)
const hasLoadError = ref(false)
const showProgressDialog = ref(false)
const selectedTaskId = ref<number | null>(null)
const progressRecords = ref<SubtaskProgressRecord[]>([])
const isHistoryLoading = ref(false)
const historyLoadError = ref<string | null>(null)
const isProgressSubmitting = ref(false)
const notificationStore = useNotificationStore()
let latestHistoryRequestId = 0

const selectedTask = computed(
  () => tasks.value.find((task) => task.id === selectedTaskId.value) ?? null,
)

const filteredProjectCandidates = computed(() => {
  const keyword = filters.projectKeyword.trim().toLocaleLowerCase()
  if (keyword === '') {
    return projectCandidates.value
  }

  return projectCandidates.value.filter((projectName) =>
    projectName.toLocaleLowerCase().includes(keyword),
  )
})

function extractProjectCandidates(taskList: Subtask[]) {
  const uniqueProjectNames = new Set<string>()
  for (const task of taskList) {
    const projectName = task.project_name?.trim()
    if (!projectName) {
      continue
    }

    uniqueProjectNames.add(projectName)
  }

  return [...uniqueProjectNames]
}

async function loadTasks() {
  isLoading.value = true
  hasLoadError.value = false
  const normalizedProjectKeyword = filters.appliedProjectKeyword.trim()

  try {
    const response = await listMySubtasks({
      project_keyword: normalizedProjectKeyword || undefined,
      status: filters.status ? Number(filters.status) : undefined,
    })
    tasks.value = response.list

    if (normalizedProjectKeyword === '') {
      projectCandidates.value = extractProjectCandidates(response.list)
    }
  } catch (error) {
    hasLoadError.value = true
    notificationStore.notifyError(getErrorMessage(error, '任务加载失败'))
  } finally {
    isLoading.value = false
  }
}

async function loadProjectCandidates() {
  try {
    const response = await listMySubtasks({
      status: filters.status ? Number(filters.status) : undefined,
    })
    projectCandidates.value = extractProjectCandidates(response.list)
  } catch {
    projectCandidates.value = []
  }
}

async function handleStatusSubmit() {
  if (filters.appliedProjectKeyword) {
    await Promise.all([loadTasks(), loadProjectCandidates()])
    return
  }

  await loadTasks()
}

async function handleProjectKeywordApply(value: string) {
  const normalizedValue = value.trim()
  filters.projectKeyword = normalizedValue
  filters.appliedProjectKeyword = normalizedValue
  await loadTasks()
}

function closeProgressDialog() {
  latestHistoryRequestId += 1
  showProgressDialog.value = false
  selectedTaskId.value = null
  progressRecords.value = []
  historyLoadError.value = null
  isHistoryLoading.value = false
}

function updateProgressDialogVisibility(value: boolean) {
  if (value) {
    showProgressDialog.value = true
    return
  }

  closeProgressDialog()
}

async function openProgressDialog(subtaskId: number) {
  selectedTaskId.value = subtaskId
  showProgressDialog.value = true
  progressRecords.value = []
  historyLoadError.value = null
  isHistoryLoading.value = true

  const requestId = ++latestHistoryRequestId

  try {
    const response = await listSubtaskProgressRecords(subtaskId, {
      page: 1,
      page_size: 20,
    })

    if (requestId !== latestHistoryRequestId || selectedTaskId.value !== subtaskId) {
      return
    }

    progressRecords.value = response.list
  } catch (error) {
    if (requestId !== latestHistoryRequestId || selectedTaskId.value !== subtaskId) {
      return
    }

    historyLoadError.value = getErrorMessage(error, '历史记录加载失败')
  } finally {
    if (requestId === latestHistoryRequestId && selectedTaskId.value === subtaskId) {
      isHistoryLoading.value = false
    }
  }
}

async function handleSubmitProgress(payload: {
  subtaskId: number
  progress_percent: number
  progress_note: string
}) {
  isProgressSubmitting.value = true

  try {
    await submitSubtaskProgress(payload.subtaskId, {
      progress_percent: payload.progress_percent,
      progress_note: payload.progress_note,
    })

    closeProgressDialog()
    await loadTasks()
  } catch (error) {
    notificationStore.notifyError(getErrorMessage(error, '进度提交失败'))
  } finally {
    isProgressSubmitting.value = false
  }
}

onMounted(loadTasks)
</script>

<template>
  <section class="my-tasks-view">
    <section class="my-tasks-view__hero-shell" data-testid="my-tasks-hero-shell">
      <header class="my-tasks-view__hero">
        <div>
          <p class="my-tasks-view__eyebrow">Execution Desk</p>
          <h2>我的任务</h2>
        </div>
        <p>聚焦当前负责人任务，快速查看所属项目、节点与截止日期。</p>
      </header>

      <div class="my-tasks-view__hero-filters">
        <MyTaskFilters
          :embedded="true"
          :project-candidates="filteredProjectCandidates"
          :project-keyword="filters.projectKeyword"
          :status="filters.status"
          @apply-project-keyword="handleProjectKeywordApply"
          @submit="handleStatusSubmit"
          @update:project-keyword="filters.projectKeyword = $event"
          @update:status="filters.status = $event"
        />
      </div>
    </section>

    <p v-if="isLoading && tasks.length === 0" class="my-tasks-view__state loading-panel">
      任务加载中...
    </p>

    <p
      v-if="isLoading && tasks.length > 0"
      aria-live="polite"
      class="my-tasks-view__refreshing"
    >
      任务列表刷新中...
    </p>

    <MyTaskBoard
      v-if="tasks.length > 0 || (!isLoading && !hasLoadError)"
      :tasks="tasks"
      @open-progress="openProgressDialog"
    />

    <MyTaskProgressDialog
      :model-value="showProgressDialog"
      :subtask="selectedTask"
      :records="progressRecords"
      :is-history-loading="isHistoryLoading"
      :history-load-error="historyLoadError"
      :is-submitting="isProgressSubmitting"
      @submit="handleSubmitProgress"
      @update:model-value="updateProgressDialogVisibility"
    />
  </section>
</template>

<style scoped>
.my-tasks-view {
  display: grid;
  gap: 18px;
  padding: 0 8px 8px;
}

.my-tasks-view__hero-shell {
  display: grid;
  gap: 20px;
  padding: 24px 26px;
  border: 1px solid var(--border-soft);
  border-radius: 20px;
  background: var(--gradient-surface), var(--project-card-glow);
  box-shadow: var(--shadow-panel-hover);
}

.my-tasks-view__hero {
  display: grid;
  grid-template-columns: minmax(0, 1fr) minmax(280px, 420px);
  gap: 18px;
}

.my-tasks-view__hero h2,
.my-tasks-view__hero p {
  margin: 0;
}

.my-tasks-view__eyebrow {
  margin-bottom: 8px;
  font-size: 0.8rem;
  letter-spacing: 0.18em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.my-tasks-view__hero > p {
  max-width: 420px;
  align-self: end;
  color: var(--text-soft);
}

.my-tasks-view__hero-filters {
  padding-top: 20px;
  border-top: 1px solid color-mix(in srgb, var(--border-soft) 92%, transparent);
}

.my-tasks-view__state {
  margin: 0;
  padding: 18px 20px;
  border: 1px solid var(--border-soft);
  border-radius: 18px;
  background: var(--meta-surface-bg), var(--meta-surface-glow);
  color: var(--text-soft);
  box-shadow: var(--meta-surface-shadow);
}

.my-tasks-view__refreshing {
  margin: 0;
  padding: 12px 16px;
  border: 1px solid var(--border-soft);
  border-radius: 16px;
  background: var(--meta-surface-bg), var(--meta-surface-glow);
  color: var(--text-soft);
  box-shadow: var(--meta-surface-shadow);
}

@media (max-width: 900px) {
  .my-tasks-view__hero {
    grid-template-columns: 1fr;
  }
}
</style>
