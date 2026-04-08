<script setup lang="ts">
import { onMounted, reactive, ref } from 'vue'

import { getErrorMessage } from '@/api/http'
import MyTaskBoard from '@/components/my-tasks/MyTaskBoard.vue'
import MyTaskFilters from '@/components/my-tasks/MyTaskFilters.vue'
import { listMySubtasks } from '@/api/subtasks'
import { useNotificationStore } from '@/stores/notifications'
import type { Subtask } from '@/types/subtask'

const filters = reactive({
  projectId: '',
  status: '',
})

const tasks = ref<Subtask[]>([])
const isLoading = ref(false)
const hasLoadError = ref(false)
const notificationStore = useNotificationStore()

async function loadTasks() {
  isLoading.value = true
  hasLoadError.value = false

  try {
    const response = await listMySubtasks({
      project_id: filters.projectId ? Number(filters.projectId) : undefined,
      status: filters.status ? Number(filters.status) : undefined,
    })
    tasks.value = response.list
  } catch (error) {
    hasLoadError.value = true
    notificationStore.notifyError(getErrorMessage(error, '任务加载失败'))
  } finally {
    isLoading.value = false
  }
}

onMounted(loadTasks)
</script>

<template>
  <section class="my-tasks-view">
    <header class="my-tasks-view__hero">
      <div>
        <p class="my-tasks-view__eyebrow">Execution Desk</p>
        <h2>我的任务</h2>
      </div>
      <p>聚焦当前负责人任务，快速查看所属项目、节点与截止日期。</p>
    </header>

    <MyTaskFilters
      :project-id="filters.projectId"
      :status="filters.status"
      @submit="loadTasks"
      @update:project-id="filters.projectId = $event"
      @update:status="filters.status = $event"
    />

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
    />
  </section>
</template>

<style scoped>
.my-tasks-view {
  display: grid;
  gap: 18px;
  padding: 0 8px 8px;
}

.my-tasks-view__hero {
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

.my-tasks-view__state {
  margin: 0;
  padding: 18px 20px;
  border: 1px solid var(--border-soft);
  border-radius: 18px;
  background: var(--glass-bg);
  color: var(--text-soft);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
}

.my-tasks-view__refreshing {
  margin: 0;
  padding: 12px 16px;
  border: 1px solid var(--border-soft);
  border-radius: 16px;
  background: color-mix(in srgb, var(--glass-bg) 86%, transparent);
  color: var(--text-soft);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
}

@media (max-width: 900px) {
  .my-tasks-view__hero {
    grid-template-columns: 1fr;
  }
}
</style>
