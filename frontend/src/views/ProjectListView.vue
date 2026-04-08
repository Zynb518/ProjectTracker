<script setup lang="ts">
import { computed, onMounted, reactive, ref } from 'vue'
import { useRouter } from 'vue-router'

import ProjectFilters from '@/components/projects/ProjectFilters.vue'
import ProjectFormDialog from '@/components/projects/ProjectFormDialog.vue'
import ProjectGrid from '@/components/projects/ProjectGrid.vue'
import {
  completeProject,
  createProject,
  deleteProject,
  listProjects,
  reopenProject,
  startProject,
  updateProject,
} from '@/api/projects'
import { getErrorMessage } from '@/api/http'
import { useNotificationStore } from '@/stores/notifications'
import type { ProjectFormPayload, ProjectListItem } from '@/types/project'

const router = useRouter()

const filters = reactive({
  keyword: '',
  status: '',
})

const defaultFormValue = (): ProjectFormPayload => ({
  name: '',
  description: '',
  planned_start_date: '',
  planned_end_date: '',
})

const projects = ref<ProjectListItem[]>([])
const isLoading = ref(false)
const hasLoadError = ref(false)
const dialogOpen = ref(false)
const dialogMode = ref<'create' | 'edit'>('create')
const formValue = ref<ProjectFormPayload>(defaultFormValue())
const editingProjectId = ref<number | null>(null)
const notificationStore = useNotificationStore()
const pagination = reactive({
  page: 1,
  pageSize: 9,
  total: 0,
})

const totalPages = computed(() => Math.max(1, Math.ceil(pagination.total / pagination.pageSize)))
const visiblePages = computed(() => {
  const windowSize = 5
  const start = Math.max(1, pagination.page - 2)
  const end = Math.min(totalPages.value, start + windowSize - 1)
  const normalizedStart = Math.max(1, end - windowSize + 1)

  return Array.from({ length: end - normalizedStart + 1 }, (_, index) => normalizedStart + index)
})

async function loadProjects(page = pagination.page) {
  isLoading.value = true
  hasLoadError.value = false

  try {
    const response = await listProjects({
      keyword: filters.keyword,
      status: filters.status ? Number(filters.status) : undefined,
      page,
      page_size: pagination.pageSize,
    })
    projects.value = response.list
    pagination.total = response.total
    pagination.page = response.page
  } catch (error) {
    hasLoadError.value = true
    notificationStore.notifyError(getErrorMessage(error, '项目列表加载失败'))
  } finally {
    isLoading.value = false
  }
}

async function applyFilters() {
  pagination.page = 1
  await loadProjects(1)
}

async function changePage(page: number) {
  if (page === pagination.page || page < 1 || page > totalPages.value) {
    return
  }

  await loadProjects(page)
}

function openCreateDialog() {
  dialogMode.value = 'create'
  editingProjectId.value = null
  formValue.value = defaultFormValue()
  dialogOpen.value = true
}

function openEditDialog(project: ProjectListItem) {
  dialogMode.value = 'edit'
  editingProjectId.value = project.id
  formValue.value = {
    name: project.name,
    description: project.description,
    planned_start_date: project.planned_start_date,
    planned_end_date: project.planned_end_date,
  }
  dialogOpen.value = true
}

async function submitProject(payload: ProjectFormPayload) {
  try {
    if (dialogMode.value === 'create') {
      await createProject(payload)
    } else if (editingProjectId.value !== null) {
      await updateProject(editingProjectId.value, payload)
    }

    dialogOpen.value = false
    await loadProjects(pagination.page)
  } catch (error) {
    notificationStore.notifyError(getErrorMessage(error, '项目保存失败'))
  }
}

async function runProjectAction(action: 'start' | 'complete' | 'reopen' | 'remove', project: ProjectListItem) {
  try {
    if (action === 'start') {
      await startProject(project.id)
    }

    if (action === 'complete') {
      await completeProject(project.id)
    }

    if (action === 'reopen') {
      await reopenProject(project.id)
    }

    if (action === 'remove') {
      await deleteProject(project.id)
    }

    await loadProjects(pagination.page)
  } catch (error) {
    notificationStore.notifyError(getErrorMessage(error, '项目操作失败'))
  }
}

function openProject(project: ProjectListItem) {
  router.push(`/projects/${project.id}`)
}

onMounted(loadProjects)
</script>

<template>
  <section class="project-list">
    <header class="project-list__hero">
      <div>
        <p class="project-list__eyebrow">Portfolio Console</p>
        <h2>项目总览</h2>
      </div>

      <p class="project-list__copy">
        在这里统一筛选项目、拉起项目详情工作台，并对项目生命周期执行基础动作。
      </p>
    </header>

    <ProjectFilters
      :keyword="filters.keyword"
      :status="filters.status"
      @create="openCreateDialog"
      @submit="applyFilters"
      @update:keyword="filters.keyword = $event"
      @update:status="filters.status = $event"
    />

    <p v-if="isLoading && projects.length === 0" class="project-list__state loading-panel">
      项目列表加载中...
    </p>
    <p v-else-if="!isLoading && !hasLoadError && projects.length === 0" class="project-list__state">
      当前没有匹配的项目。
    </p>

    <p
      v-if="isLoading && projects.length > 0"
      aria-live="polite"
      class="project-list__refreshing"
    >
      项目列表刷新中...
    </p>

    <ProjectGrid
      v-if="projects.length > 0"
      :projects="projects"
      @complete="runProjectAction('complete', $event)"
      @edit="openEditDialog"
      @open="openProject"
      @remove="runProjectAction('remove', $event)"
      @reopen="runProjectAction('reopen', $event)"
      @start="runProjectAction('start', $event)"
    />

    <section
      v-if="pagination.total > 0"
      class="project-list__pagination"
    >
      <p class="project-list__pagination-summary">共 {{ pagination.total }} 个项目</p>

      <div class="project-list__pagination-controls">
        <button
          type="button"
          :disabled="isLoading || pagination.page === 1"
          @click="changePage(1)"
        >
          首页
        </button>

        <button
          type="button"
          :disabled="isLoading || pagination.page === 1"
          @click="changePage(pagination.page - 1)"
        >
          上一页
        </button>

        <button
          v-for="page in visiblePages"
          :key="page"
          :class="['project-list__page', { 'is-active': page === pagination.page }]"
          type="button"
          :disabled="isLoading"
          @click="changePage(page)"
        >
          {{ page }}
        </button>

        <button
          type="button"
          :disabled="isLoading || pagination.page === totalPages"
          @click="changePage(pagination.page + 1)"
        >
          下一页
        </button>

        <button
          type="button"
          :disabled="isLoading || pagination.page === totalPages"
          @click="changePage(totalPages)"
        >
          尾页
        </button>
      </div>

      <p class="project-list__pagination-indicator">第 {{ pagination.page }} / {{ totalPages }} 页</p>
    </section>

    <ProjectFormDialog
      v-model="dialogOpen"
      :initial-value="formValue"
      :mode="dialogMode"
      @submit="submitProject"
    />
  </section>
</template>

<style scoped>
.project-list {
  display: grid;
  gap: 18px;
  padding: 0 8px 8px;
}

.project-list__hero {
  display: grid;
  grid-template-columns: minmax(0, 1fr) minmax(280px, 420px);
  gap: 18px;
  padding: 24px 26px;
  border: 1px solid var(--border-soft);
  border-radius: 20px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--glass-bg) 92%, transparent), var(--glass-bg)),
    radial-gradient(circle at top right, rgba(0, 194, 255, 0.16), transparent 24%);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
}

.project-list__hero h2,
.project-list__hero p {
  margin: 0;
}

.project-list__hero h2 {
  font-size: clamp(1.7rem, 2.8vw, 2.3rem);
}

.project-list__eyebrow {
  margin-bottom: 10px;
  font-size: 0.78rem;
  font-weight: 600;
  letter-spacing: 0.18em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.project-list__copy {
  max-width: 520px;
  color: var(--text-soft);
  align-self: end;
}

.project-list__state {
  margin: 0;
  padding: 18px 20px;
  border: 1px solid var(--border-soft);
  border-radius: 18px;
  background: var(--glass-bg);
  color: var(--text-soft);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
}

.project-list__refreshing {
  margin: 0;
  padding: 12px 16px;
  border: 1px solid var(--border-soft);
  border-radius: 16px;
  background: color-mix(in srgb, var(--glass-bg) 86%, transparent);
  color: var(--text-soft);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
}

.project-list__pagination {
  display: grid;
  grid-template-columns: auto minmax(0, 1fr) auto;
  align-items: center;
  gap: 16px;
  padding: 16px 18px;
  border: 1px solid var(--border-soft);
  border-radius: 18px;
  background: var(--glass-bg);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
}

.project-list__pagination-summary,
.project-list__pagination-indicator {
  margin: 0;
  color: var(--text-soft);
  font-size: 0.92rem;
}

.project-list__pagination-controls {
  display: flex;
  align-items: center;
  justify-content: center;
  flex-wrap: wrap;
  gap: 8px;
}

.project-list__pagination-controls button {
  min-width: 40px;
  min-height: 40px;
  border: 1px solid var(--border-soft);
  border-radius: 10px;
  padding: 0 14px;
  background: color-mix(in srgb, var(--panel-bg) 88%, transparent);
  color: var(--text-main);
  cursor: pointer;
}

.project-list__pagination-controls button:hover:not(:disabled) {
  transform: translateY(-2px);
  border-color: var(--accent-line);
  box-shadow: var(--shadow-panel-hover);
}

.project-list__pagination-controls button:disabled {
  cursor: not-allowed;
  opacity: 0.48;
}

.project-list__page.is-active {
  border-color: transparent;
  background: var(--gradient-primary);
  color: var(--text-inverse);
  box-shadow: 0 14px 24px rgba(10, 102, 255, 0.2);
}

@media (max-width: 900px) {
  .project-list__hero {
    grid-template-columns: 1fr;
  }

  .project-list__pagination {
    grid-template-columns: 1fr;
    justify-items: center;
    text-align: center;
  }
}
</style>
