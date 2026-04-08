<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { storeToRefs } from 'pinia'
import { useRoute } from 'vue-router'

import AddMemberDialog from '@/components/members/AddMemberDialog.vue'
import MemberPanel from '@/components/members/MemberPanel.vue'
import SubtaskHistoryDrawer from '@/components/subtasks/SubtaskHistoryDrawer.vue'
import SubtaskProgressDialog from '@/components/subtasks/SubtaskProgressDialog.vue'
import SubtaskTable from '@/components/subtasks/SubtaskTable.vue'
import NodeRail from '@/components/workspace/NodeRail.vue'
import ProjectHero from '@/components/workspace/ProjectHero.vue'
import WorkspaceStats from '@/components/workspace/WorkspaceStats.vue'
import { getErrorMessage } from '@/api/http'
import { addProjectMember, removeProjectMember } from '@/api/members'
import { listSubtaskProgressRecords, submitSubtaskProgress } from '@/api/subtasks'
import { useNotificationStore } from '@/stores/notifications'
import { useProjectWorkspaceStore } from '@/stores/projectWorkspace'
import type { SubtaskProgressRecord } from '@/types/subtask'

const route = useRoute()
const workspaceStore = useProjectWorkspaceStore()
const notificationStore = useNotificationStore()
const { isLoading, isSubtasksLoading, members, nodes, project, selectedNode, selectedNodeId, subtasks } =
  storeToRefs(workspaceStore)

const projectId = Number(route.params.projectId)
const showAddMemberDialog = ref(false)
const showProgressDialog = ref(false)
const showHistoryDrawer = ref(false)
const selectedSubtaskId = ref<number | null>(null)
const historyTitle = ref('')
const progressRecords = ref<SubtaskProgressRecord[]>([])

const selectedSubtask = computed(
  () => subtasks.value.find((subtask) => subtask.id === selectedSubtaskId.value) ?? null,
)

const canManageMembers = computed(() => project.value?.permissions.can_manage_members ?? false)
const canManageNodes = computed(() => project.value?.permissions.can_manage_nodes ?? false)

onMounted(async () => {
  try {
    await workspaceStore.loadWorkspace(projectId)
  } catch (error) {
    notificationStore.notifyError(getErrorMessage(error, '项目工作台加载失败'))
  }
})

async function refreshWorkspace() {
  await workspaceStore.loadWorkspace(projectId)
}

async function handleAddMember(userId: number) {
  try {
    await addProjectMember(projectId, userId)
    showAddMemberDialog.value = false
    await refreshWorkspace()
  } catch (error) {
    notificationStore.notifyError(getErrorMessage(error, '添加成员失败'))
  }
}

async function handleRemoveMember(userId: number) {
  try {
    await removeProjectMember(projectId, userId)
    await refreshWorkspace()
  } catch (error) {
    notificationStore.notifyError(getErrorMessage(error, '移除成员失败'))
  }
}

async function openProgressDialog(subtaskId: number) {
  selectedSubtaskId.value = subtaskId
  showProgressDialog.value = true
}

async function handleSelectNode(nodeId: number) {
  try {
    await workspaceStore.selectNode(projectId, nodeId)
  } catch (error) {
    notificationStore.notifyError(getErrorMessage(error, '节点任务加载失败'))
  }
}

async function submitProgress(payload: {
  subtaskId: number
  status: number
  progress_percent: number
  progress_note: string
}) {
  try {
    await submitSubtaskProgress(payload.subtaskId, {
      status: payload.status,
      progress_percent: payload.progress_percent,
      progress_note: payload.progress_note,
    })
    showProgressDialog.value = false

    if (selectedNodeId.value !== null) {
      await workspaceStore.selectNode(projectId, selectedNodeId.value)
    }
  } catch (error) {
    notificationStore.notifyError(getErrorMessage(error, '进度提交失败'))
  }
}

async function openHistoryDrawer(subtaskId: number) {
  try {
    selectedSubtaskId.value = subtaskId
    historyTitle.value = selectedSubtask.value?.name ?? '任务历史'
    const response = await listSubtaskProgressRecords(subtaskId, {
      page: 1,
      page_size: 20,
    })
    progressRecords.value = response.list
    showHistoryDrawer.value = true
  } catch (error) {
    notificationStore.notifyError(getErrorMessage(error, '任务历史加载失败'))
  }
}
</script>

<template>
  <section class="project-detail">
    <p v-if="isLoading && !project" class="project-detail__state loading-panel">工作台加载中...</p>

    <template v-else-if="project">
      <p
        v-if="isLoading"
        aria-live="polite"
        class="project-detail__refreshing"
      >
        工作台刷新中...
      </p>

      <ProjectHero :project="project" />
      <WorkspaceStats
        :completed-node-count="project.completed_node_count"
        :completed-sub-task-count="project.completed_sub_task_count"
        :member-count="project.member_count"
        :node-count="project.node_count"
        :sub-task-count="project.sub_task_count"
      />

      <div class="project-detail__grid">
        <NodeRail
          :nodes="nodes"
          :selected-node-id="selectedNodeId"
          @select="handleSelectNode"
        />

        <div class="project-detail__main">
          <p v-if="isSubtasksLoading" class="project-detail__state loading-panel">节点任务加载中...</p>
          <SubtaskTable
            v-else
            :can-manage="canManageNodes"
            :selected-node="selectedNode"
            :subtasks="subtasks"
            @history="openHistoryDrawer"
            @progress="openProgressDialog"
          />
        </div>

        <MemberPanel
          :can-manage="canManageMembers"
          :members="members"
          @add="showAddMemberDialog = true"
          @remove="handleRemoveMember"
        />
      </div>
    </template>

    <AddMemberDialog
      v-if="project"
      v-model="showAddMemberDialog"
      :project-id="project.id"
      @select="handleAddMember"
    />

    <SubtaskProgressDialog
      v-model="showProgressDialog"
      :subtask="selectedSubtask"
      @submit="submitProgress"
    />

    <SubtaskHistoryDrawer
      v-model="showHistoryDrawer"
      :records="progressRecords"
      :title="historyTitle"
    />
  </section>
</template>

<style scoped>
.project-detail {
  display: grid;
  gap: 18px;
  padding: 0 8px 8px;
}

.project-detail__grid {
  display: grid;
  grid-template-columns: minmax(260px, 320px) minmax(0, 1fr) minmax(260px, 300px);
  gap: 18px;
  align-items: start;
}

.project-detail__main {
  min-width: 0;
}

.project-detail__state {
  margin: 0;
  padding: 18px 20px;
  border: 1px solid var(--border-soft);
  border-radius: 18px;
  background: var(--glass-bg);
  color: var(--text-soft);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
}

.project-detail__refreshing {
  margin: 0;
  padding: 12px 16px;
  border: 1px solid var(--border-soft);
  border-radius: 16px;
  background: color-mix(in srgb, var(--glass-bg) 86%, transparent);
  color: var(--text-soft);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
}

@media (max-width: 1180px) {
  .project-detail__grid {
    grid-template-columns: 1fr;
  }
}
</style>
