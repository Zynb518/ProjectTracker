<script setup lang="ts">
import { computed, onMounted, reactive, ref } from 'vue'
import { useRouter } from 'vue-router'

import ProjectFilters from '@/components/projects/ProjectFilters.vue'
import ProjectFormDialog from '@/components/projects/ProjectFormDialog.vue'
import ProjectListGanttDialog from '@/components/projects/ProjectListGanttDialog.vue'
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
import type { GanttScale } from '@/types/gantt'
import type { ProjectFormPayload, ProjectListItem } from '@/types/project'
import { getTotalPages, getVisiblePages } from '@/utils/pagination'

const router = useRouter()

type DialogMotionOrigin = {
  x: number
  y: number
  translateX: number
  translateY: number
}

type DialogTriggerOrigin = {
  x: number
  y: number
}

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
const ganttLoadError = ref<string | null>(null)
const ganttProjects = ref<ProjectListItem[]>([])
const ganttDialogOpen = ref(false)
const ganttScale = ref<GanttScale>('week')
const isGanttLoading = ref(false)
const dialogOpen = ref(false)
const dialogMode = ref<'create' | 'edit'>('create')
const formValue = ref<ProjectFormPayload>(defaultFormValue())
const editingProjectId = ref<number | null>(null)
const dialogMotionOrigin = ref<DialogMotionOrigin | null>(null)
const ganttDialogMotionOrigin = ref<DialogMotionOrigin | null>(null)
const notificationStore = useNotificationStore()
const pagination = reactive({
  page: 1,
  pageSize: 9,
  total: 0,
})

const totalPages = computed(() => getTotalPages(pagination.total, pagination.pageSize))
const visiblePages = computed(() => getVisiblePages(pagination.page, totalPages.value))
const ganttFiltersSummary = computed(() => {
  const segments = [
    filters.keyword.trim() ? `关键字：${filters.keyword.trim()}` : '关键字：全部项目',
    filters.status ? `状态：${getStatusLabel(filters.status)}` : '状态：全部状态',
  ]

  return segments.join(' · ')
})

const ganttProjectCount = computed(() => ganttProjects.value.length)

function getStatusLabel(status: string) {
  return (
    {
      '1': '未开始',
      '2': '进行中',
      '3': '已完成',
      '4': '已延期',
    }[status] ?? '全部状态'
  )
}

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

async function loadProjectsForGantt() {
  const status = filters.status ? Number(filters.status) : undefined
  const pageSize = 100

  isGanttLoading.value = true
  ganttLoadError.value = null

  try {
    const firstPage = await listProjects({
      keyword: filters.keyword,
      status,
      page: 1,
      page_size: pageSize,
    })

    const mergedProjects = [...firstPage.list]
    const allPages = getTotalPages(firstPage.total, firstPage.page_size)

    for (let page = 2; page <= allPages; page += 1) {
      const nextPage = await listProjects({
        keyword: filters.keyword,
        status,
        page,
        page_size: pageSize,
      })

      mergedProjects.push(...nextPage.list)
    }

    ganttProjects.value = mergedProjects
  } catch (error) {
    ganttLoadError.value = getErrorMessage(error, '项目甘特图加载失败')
  } finally {
    isGanttLoading.value = false
  }
}

async function changePage(page: number) {
  if (page === pagination.page || page < 1 || page > totalPages.value) {
    return
  }

  await loadProjects(page)
}

function createDialogMotionOrigin(origin: DialogTriggerOrigin | null): DialogMotionOrigin | null {
  if (!origin || typeof window === 'undefined') {
    return null
  }

  return {
    x: origin.x,
    y: origin.y,
    translateX: origin.x - window.innerWidth / 2,
    translateY: origin.y - window.innerHeight / 2,
  }
}

function openCreateDialog(origin: DialogTriggerOrigin) {
  dialogMode.value = 'create'
  editingProjectId.value = null
  formValue.value = defaultFormValue()
  dialogMotionOrigin.value = createDialogMotionOrigin(origin)
  dialogOpen.value = true
}

async function openGanttDialog(origin: DialogTriggerOrigin) {
  ganttDialogMotionOrigin.value = createDialogMotionOrigin(origin)
  ganttDialogOpen.value = true
  await loadProjectsForGantt()
}

async function retryGanttDialog() {
  await loadProjectsForGantt()
}

function openEditDialog(project: ProjectListItem) {
  dialogMode.value = 'edit'
  editingProjectId.value = project.id
  dialogMotionOrigin.value = null
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

    if (ganttDialogOpen.value) {
      await loadProjectsForGantt()
    }
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
    <section class="project-list__hero-shell" data-testid="project-list-hero-shell">
      <header class="project-list__hero">
        <div>
          <p class="project-list__eyebrow">Portfolio Console</p>
          <h2>项目总览</h2>
        </div>

        <p class="project-list__copy">
          在这里统一筛选项目、拉起项目详情工作台，并对项目生命周期执行基础动作。
        </p>
      </header>

      <div class="project-list__hero-filters">
        <ProjectFilters
          :embedded="true"
          :keyword="filters.keyword"
          :status="filters.status"
          @create="openCreateDialog"
          @open-gantt="openGanttDialog"
          @submit="applyFilters"
          @update:keyword="filters.keyword = $event"
          @update:status="filters.status = $event"
        />
      </div>
    </section>

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
      :motion-origin="dialogMotionOrigin"
      :mode="dialogMode"
      @submit="submitProject"
    />

    <ProjectListGanttDialog
      v-model="ganttDialogOpen"
      :error="ganttLoadError"
      :filters-summary="ganttFiltersSummary"
      :is-loading="isGanttLoading"
      :motion-origin="ganttDialogMotionOrigin"
      :project-count="ganttProjectCount"
      :projects="ganttProjects"
      :scale="ganttScale"
      @open-project="openProject"
      @retry="retryGanttDialog"
      @update:scale="ganttScale = $event"
    />
  </section>
</template>

<style scoped>
.project-list {
  display: grid;
  gap: 18px;
  padding: 0 8px 8px;
}

.project-list__hero-shell {
  display: grid;
  gap: 20px;
  padding: 24px 26px;
  border: 1px solid var(--border-soft);
  border-radius: 20px;
  background: var(--gradient-surface), var(--project-card-glow);
  box-shadow: var(--shadow-panel-hover);
}

.project-list__hero {
  display: grid;
  grid-template-columns: minmax(0, 1fr) minmax(280px, 420px);
  gap: 18px;
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

.project-list__hero-filters {
  padding-top: 20px;
  border-top: 1px solid color-mix(in srgb, var(--border-soft) 92%, transparent);
}

.project-list__state {
  margin: 0;
  padding: 18px 20px;
  border: 1px solid var(--border-soft);
  border-radius: 18px;
  background: var(--meta-surface-bg), var(--meta-surface-glow);
  color: var(--text-soft);
  box-shadow: var(--meta-surface-shadow);
}

.project-list__refreshing {
  margin: 0;
  padding: 12px 16px;
  border: 1px solid var(--border-soft);
  border-radius: 16px;
  background: var(--meta-surface-bg), var(--meta-surface-glow);
  color: var(--text-soft);
  box-shadow: var(--meta-surface-shadow);
}

.project-list__pagination {
  display: grid;
  grid-template-columns: auto minmax(0, 1fr) auto;
  align-items: center;
  gap: 16px;
  padding: 16px 18px;
  border: 1px solid var(--border-soft);
  border-radius: 18px;
  background: var(--meta-surface-bg), var(--meta-surface-glow);
  box-shadow: var(--meta-surface-shadow);
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

<style>
html.light .project-list,
html:not(.dark) .project-list {
  --project-list-sky-panel: linear-gradient(180deg, rgba(247, 251, 255, 0.84), rgba(235, 243, 252, 0.72));
  --project-list-sky-fill: linear-gradient(145deg, rgba(255, 255, 255, 0.48), rgba(209, 228, 248, 0.12) 54%);
  --project-list-sky-card: linear-gradient(180deg, rgba(249, 252, 255, 0.82), rgba(238, 245, 253, 0.68));
  --project-list-sky-border: rgba(118, 149, 185, 0.24);
  --project-list-sky-border-soft: rgba(128, 160, 196, 0.18);
  --project-list-sky-text: #21324b;
  --project-list-sky-text-soft: rgba(69, 89, 118, 0.82);
  --project-list-sky-shadow:
    0 18px 34px rgba(63, 92, 126, 0.14),
    0 4px 16px rgba(108, 138, 173, 0.08),
    inset 0 1px 0 rgba(255, 255, 255, 0.74);
}

html.light .project-list__hero-shell,
html:not(.dark) .project-list__hero-shell {
  border-color: var(--project-list-sky-border);
  background: var(--project-list-sky-panel), var(--project-list-sky-fill);
  box-shadow: var(--project-list-sky-shadow);
}

html.light .project-list__hero h2,
html:not(.dark) .project-list__hero h2 {
  color: var(--project-list-sky-text);
  letter-spacing: -0.03em;
  text-shadow: 0 1px 0 rgba(255, 255, 255, 0.44);
}

html.light .project-list__eyebrow,
html.light .project-list__copy,
html.light .project-list__pagination-summary,
html.light .project-list__pagination-indicator,
html.light .project-list__state,
html.light .project-list__refreshing,
html:not(.dark) .project-list__eyebrow,
html:not(.dark) .project-list__copy,
html:not(.dark) .project-list__pagination-summary,
html:not(.dark) .project-list__pagination-indicator,
html:not(.dark) .project-list__state,
html:not(.dark) .project-list__refreshing {
  color: var(--project-list-sky-text-soft);
}

html.light .project-list__hero-filters,
html:not(.dark) .project-list__hero-filters {
  border-top-color: rgba(141, 171, 205, 0.24);
}

html.light .project-list__state,
html.light .project-list__refreshing,
html.light .project-list__pagination,
html:not(.dark) .project-list__state,
html:not(.dark) .project-list__refreshing,
html:not(.dark) .project-list__pagination {
  border-color: var(--project-list-sky-border);
  background: var(--project-list-sky-panel), var(--project-list-sky-fill);
  box-shadow:
    0 16px 32px rgba(67, 94, 127, 0.12),
    0 2px 10px rgba(120, 151, 186, 0.08),
    inset 0 1px 0 rgba(255, 255, 255, 0.72);
}

html.light .project-list__pagination-controls button,
html:not(.dark) .project-list__pagination-controls button {
  border-color: var(--project-list-sky-border-soft);
  background: var(--project-list-sky-card);
  color: rgba(48, 71, 103, 0.92);
  box-shadow:
    0 6px 16px rgba(120, 150, 186, 0.08),
    inset 0 1px 0 rgba(255, 255, 255, 0.72);
}

html.light .project-list__pagination-controls button:hover:not(:disabled),
html:not(.dark) .project-list__pagination-controls button:hover:not(:disabled) {
  border-color: rgba(98, 177, 227, 0.34);
  box-shadow:
    0 14px 24px rgba(92, 122, 158, 0.14),
    inset 0 1px 0 rgba(255, 255, 255, 0.82),
    0 0 18px rgba(114, 201, 245, 0.1);
}

html.light .project-list__page.is-active,
html:not(.dark) .project-list__page.is-active {
  background: linear-gradient(135deg, rgba(61, 119, 197, 0.94), rgba(110, 210, 247, 0.9));
  color: rgba(248, 252, 255, 0.98);
  box-shadow:
    0 14px 26px rgba(75, 111, 154, 0.18),
    0 0 18px rgba(107, 204, 244, 0.16);
}

html.light .project-list .project-filters__field span,
html:not(.dark) .project-list .project-filters__field span {
  color: rgba(78, 101, 132, 0.78);
}

html.light .project-list .project-filters__input,
html:not(.dark) .project-list .project-filters__input {
  border-color: var(--project-list-sky-border-soft);
  background: linear-gradient(180deg, rgba(255, 255, 255, 0.78), rgba(243, 248, 255, 0.62));
  box-shadow:
    0 6px 14px rgba(120, 150, 186, 0.06),
    inset 0 1px 0 rgba(255, 255, 255, 0.82);
  color: rgba(36, 53, 79, 0.94);
}

html.light .project-list .project-filters__input::placeholder,
html:not(.dark) .project-list .project-filters__input::placeholder {
  color: rgba(111, 132, 161, 0.72);
}

html.light .project-list .project-filters__input:focus,
html:not(.dark) .project-list .project-filters__input:focus {
  border-color: rgba(98, 177, 227, 0.4);
  box-shadow:
    0 8px 18px rgba(114, 160, 206, 0.1),
    inset 0 1px 0 rgba(255, 255, 255, 0.92),
    0 0 0 4px rgba(114, 201, 245, 0.12);
}

html.light .project-list .project-filters__status-pill,
html:not(.dark) .project-list .project-filters__status-pill {
  border-color: var(--project-list-sky-border-soft);
  background: linear-gradient(180deg, rgba(255, 255, 255, 0.64), rgba(239, 246, 255, 0.52));
  color: rgba(67, 88, 118, 0.88);
}

html.light .project-list .project-filters__status-pill--selected,
html:not(.dark) .project-list .project-filters__status-pill--selected {
  box-shadow:
    0 10px 18px rgba(96, 124, 158, 0.12),
    inset 0 1px 0 rgba(255, 255, 255, 0.8);
}

html.light .project-list .project-filters__status-pill--all,
html:not(.dark) .project-list .project-filters__status-pill--all {
  border-color: var(--project-list-sky-border-soft);
}

html.light .project-list .project-filters__status-pill--pending,
html:not(.dark) .project-list .project-filters__status-pill--pending {
  border-color: var(--work-status-pending-border);
  background: var(--work-status-pending-bg);
  color: var(--work-status-pending-color);
}

html.light .project-list .project-filters__status-pill--active,
html:not(.dark) .project-list .project-filters__status-pill--active {
  border-color: var(--work-status-active-border);
  background: var(--work-status-active-bg);
  color: var(--work-status-active-color);
}

html.light .project-list .project-filters__status-pill--done,
html:not(.dark) .project-list .project-filters__status-pill--done {
  border-color: var(--work-status-done-border);
  background: var(--work-status-done-bg);
  color: var(--work-status-done-color);
}

html.light .project-list .project-filters__status-pill--delayed,
html:not(.dark) .project-list .project-filters__status-pill--delayed {
  border-color: var(--work-status-delayed-border);
  background: var(--work-status-delayed-bg);
  color: var(--work-status-delayed-color);
}

html.light .project-list .project-filters__status-pill--pending.project-filters__status-pill--selected,
html:not(.dark) .project-list .project-filters__status-pill--pending.project-filters__status-pill--selected {
  border-color: var(--work-status-pending-border);
  background: var(--work-status-pending-strong);
  color: var(--work-status-pending-contrast);
}

html.light .project-list .project-filters__status-pill--active.project-filters__status-pill--selected,
html:not(.dark) .project-list .project-filters__status-pill--active.project-filters__status-pill--selected {
  border-color: var(--work-status-active-border);
  background: var(--work-status-active-strong);
  color: var(--work-status-active-contrast);
}

html.light .project-list .project-filters__status-pill--done.project-filters__status-pill--selected,
html:not(.dark) .project-list .project-filters__status-pill--done.project-filters__status-pill--selected {
  border-color: var(--work-status-done-border);
  background: var(--work-status-done-strong);
  color: var(--work-status-done-contrast);
}

html.light .project-list .project-filters__status-pill--delayed.project-filters__status-pill--selected,
html:not(.dark) .project-list .project-filters__status-pill--delayed.project-filters__status-pill--selected {
  border-color: var(--work-status-delayed-border);
  background: var(--work-status-delayed-strong);
  color: var(--work-status-delayed-contrast);
}

html.light .project-list .project-filters__create,
html:not(.dark) .project-list .project-filters__create {
  background: linear-gradient(135deg, rgba(77, 130, 205, 0.92), rgba(123, 220, 248, 0.88));
  box-shadow:
    0 14px 28px rgba(84, 118, 155, 0.18),
    0 0 20px rgba(123, 220, 248, 0.14);
}

html.light .project-list .project-filters__overview,
html:not(.dark) .project-list .project-filters__overview {
  border-color: var(--project-list-sky-border-soft);
  background: linear-gradient(180deg, rgba(255, 255, 255, 0.82), rgba(239, 246, 255, 0.68));
  color: rgba(38, 60, 91, 0.94);
  box-shadow:
    0 8px 18px rgba(120, 150, 186, 0.08),
    inset 0 1px 0 rgba(255, 255, 255, 0.82);
}

html.light .project-list .project-filters__overview:hover,
html.light .project-list .project-filters__overview:focus-visible,
html:not(.dark) .project-list .project-filters__overview:hover,
html:not(.dark) .project-list .project-filters__overview:focus-visible {
  border-color: rgba(98, 177, 227, 0.34);
  box-shadow:
    0 14px 24px rgba(92, 122, 158, 0.14),
    inset 0 1px 0 rgba(255, 255, 255, 0.86),
    0 0 18px rgba(114, 201, 245, 0.1);
}

html.light .project-list .project-filters__overview-icon,
html:not(.dark) .project-list .project-filters__overview-icon {
  color: rgba(59, 119, 184, 0.9);
}

html.light .project-list .project-card,
html:not(.dark) .project-list .project-card {
  border-color: var(--project-list-sky-border);
  background: var(--project-list-sky-card), var(--project-list-sky-fill);
  box-shadow:
    0 18px 36px rgba(67, 94, 127, 0.14),
    0 4px 14px rgba(118, 148, 183, 0.08),
    inset 0 1px 0 rgba(255, 255, 255, 0.76);
}

html.light .project-list .project-card::before,
html:not(.dark) .project-list .project-card::before {
  content: '';
  position: absolute;
  inset: -18% -36% 34% 28%;
  border-radius: inherit;
  background:
    linear-gradient(
      122deg,
      rgba(104, 212, 255, 0),
      rgba(104, 212, 255, 0.12) 56%,
      rgba(255, 214, 170, 0.18) 72%,
      rgba(255, 214, 170, 0) 88%
    );
  opacity: 0;
  pointer-events: none;
  transform: translate3d(18%, -12%, 0) rotate(-10deg);
  transition:
    opacity 220ms ease-out,
    transform 560ms cubic-bezier(0.22, 1, 0.36, 1);
}

html.light .project-list .project-card::after,
html:not(.dark) .project-list .project-card::after {
  background: radial-gradient(
    circle at 76% 84%,
    rgba(255, 220, 176, 0.24),
    rgba(96, 202, 255, 0.08) 44%,
    transparent 72%
  );
}

html.light .project-list .project-card:hover,
html.light .project-list .project-card:focus-visible,
html:not(.dark) .project-list .project-card:hover,
html:not(.dark) .project-list .project-card:focus-visible {
  border-color: rgba(98, 177, 227, 0.34);
  box-shadow:
    0 22px 42px rgba(70, 99, 134, 0.16),
    inset 0 1px 0 rgba(255, 255, 255, 0.78),
    0 0 24px rgba(114, 201, 245, 0.12);
}

html.light .project-list .project-card:hover::before,
html.light .project-list .project-card:focus-visible::before,
html:not(.dark) .project-list .project-card:hover::before,
html:not(.dark) .project-list .project-card:focus-visible::before {
  opacity: 1;
  transform: translate3d(-10%, 10%, 0) rotate(-10deg);
}

html.light .project-list .project-card__name,
html:not(.dark) .project-list .project-card__name {
  color: var(--project-list-sky-text);
}

html.light .project-list .project-card:hover .project-card__name,
html.light .project-list .project-card:focus-visible .project-card__name,
html:not(.dark) .project-list .project-card:hover .project-card__name,
html:not(.dark) .project-list .project-card:focus-visible .project-card__name {
  color: var(--project-list-sky-text);
  text-shadow: none;
}

html.light .project-list .project-card__description,
html.light .project-list .project-card__meta dt,
html:not(.dark) .project-list .project-card__description,
html:not(.dark) .project-list .project-card__meta dt {
  color: var(--project-list-sky-text-soft);
}

html.light .project-list .project-card__meta dd,
html:not(.dark) .project-list .project-card__meta dd {
  color: rgba(31, 49, 73, 0.9);
}

html.light .project-list .project-card__meta-item,
html:not(.dark) .project-list .project-card__meta-item {
  border-color: rgba(139, 169, 204, 0.22);
  background: linear-gradient(180deg, rgba(255, 255, 255, 0.76), rgba(235, 243, 252, 0.64));
  box-shadow:
    0 6px 14px rgba(113, 143, 179, 0.08),
    inset 0 1px 0 rgba(255, 255, 255, 0.82);
}

html.light .project-list .project-card__meta-item::after,
html:not(.dark) .project-list .project-card__meta-item::after {
  border-color: rgba(139, 169, 204, 0.24);
  background: linear-gradient(180deg, rgba(246, 250, 255, 0.96), rgba(235, 244, 253, 0.88));
  color: rgba(66, 96, 127, 0.88);
  box-shadow: 0 10px 20px rgba(96, 124, 158, 0.12);
}

html.light .project-list .project-card__meta-icon,
html:not(.dark) .project-list .project-card__meta-icon {
  color: rgba(59, 119, 184, 0.82);
}

html.light .project-list .project-actions__button,
html:not(.dark) .project-list .project-actions__button {
  border-color: rgba(139, 169, 204, 0.22);
  box-shadow:
    0 8px 18px rgba(108, 138, 173, 0.08),
    inset 0 1px 0 rgba(255, 255, 255, 0.78);
}

html.light .project-list .project-actions__button:hover:not(:disabled),
html:not(.dark) .project-list .project-actions__button:hover:not(:disabled) {
  box-shadow:
    0 12px 22px rgba(95, 126, 163, 0.12),
    inset 0 1px 0 rgba(255, 255, 255, 0.86),
    0 0 18px rgba(114, 201, 245, 0.08);
}

html.light .project-list .project-actions__button--start,
html:not(.dark) .project-list .project-actions__button--start,
html.light .project-list .project-actions__button--success,
html:not(.dark) .project-list .project-actions__button--success {
  background: linear-gradient(180deg, rgba(236, 246, 255, 0.9), rgba(213, 232, 249, 0.74));
}

html.light .project-list .project-actions__button--start,
html:not(.dark) .project-list .project-actions__button--start {
  border-color: rgba(117, 195, 236, 0.34);
  color: rgba(40, 101, 145, 0.96);
}

html.light .project-list .project-actions__button--success,
html:not(.dark) .project-list .project-actions__button--success {
  border-color: rgba(121, 203, 157, 0.34);
  color: rgba(39, 119, 83, 0.96);
  background: linear-gradient(180deg, rgba(236, 251, 243, 0.92), rgba(211, 238, 223, 0.76));
}

html.light .project-list .project-actions__button--warning,
html:not(.dark) .project-list .project-actions__button--warning {
  border-color: rgba(226, 173, 104, 0.36);
  background: linear-gradient(180deg, rgba(255, 240, 219, 0.92), rgba(244, 215, 173, 0.78));
  color: rgba(148, 91, 36, 0.96);
}

html.light .project-list .project-actions__button--neutral,
html:not(.dark) .project-list .project-actions__button--neutral {
  border-color: rgba(139, 169, 204, 0.22);
  background: linear-gradient(180deg, rgba(252, 255, 255, 0.8), rgba(236, 244, 252, 0.68));
  color: rgba(41, 65, 95, 0.92);
  box-shadow:
    0 8px 18px rgba(108, 138, 173, 0.08),
    inset 0 1px 0 rgba(255, 255, 255, 0.78);
}

html.light .project-list .project-actions__button--danger,
html:not(.dark) .project-list .project-actions__button--danger {
  border-color: rgba(231, 147, 151, 0.34);
  background: linear-gradient(180deg, rgba(255, 239, 241, 0.92), rgba(245, 214, 219, 0.76));
  color: rgba(180, 62, 74, 0.94);
}

html.light .project-list .project-actions__button--disabled,
html:not(.dark) .project-list .project-actions__button--disabled {
  border-color: rgba(165, 181, 203, 0.28);
  background: linear-gradient(180deg, rgba(244, 248, 252, 0.84), rgba(228, 236, 246, 0.7));
  color: rgba(112, 127, 148, 0.86);
  box-shadow:
    0 6px 14px rgba(118, 148, 183, 0.06),
    inset 0 1px 0 rgba(255, 255, 255, 0.74);
}
</style>
