<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { storeToRefs } from 'pinia'
import { useRoute } from 'vue-router'

import AddMemberDialog from '@/components/members/AddMemberDialog.vue'
import MemberPanel from '@/components/members/MemberPanel.vue'
import SubtaskFormDialog from '@/components/subtasks/SubtaskFormDialog.vue'
import SubtaskHistoryDrawer from '@/components/subtasks/SubtaskHistoryDrawer.vue'
import SubtaskProgressDialog from '@/components/subtasks/SubtaskProgressDialog.vue'
import NodeDrawer from '@/components/workspace/NodeDrawer.vue'
import NodeGanttDialog from '@/components/workspace/NodeGanttDialog.vue'
import NodeFormDialog from '@/components/workspace/NodeFormDialog.vue'
import NodeRail from '@/components/workspace/NodeRail.vue'
import ProjectGanttView from '@/components/workspace/ProjectGanttView.vue'
import ProjectMemberGanttView from '@/components/workspace/ProjectMemberGanttView.vue'
import ProjectHero from '@/components/workspace/ProjectHero.vue'
import { getErrorMessage } from '@/api/http'
import { addProjectMember, removeProjectMember } from '@/api/members'
import {
  completeProjectNode,
  createProjectNode,
  deleteProjectNode,
  getProjectNodeDetail,
  getProjectNodeGantt,
  reorderProjectNodes,
  reopenProjectNode,
  startProjectNode,
  updateProjectNode,
} from '@/api/nodes'
import { getProjectGanttMembers, getProjectGanttNodes } from '@/api/projects'
import {
  createSubtask,
  deleteSubtask,
  listSubtaskProgressRecords,
  reopenSubtask,
  submitSubtaskProgress,
  updateSubtask,
} from '@/api/subtasks'
import { useNotificationStore } from '@/stores/notifications'
import { useProjectWorkspaceStore } from '@/stores/projectWorkspace'
import type {
  GanttPerspective,
  GanttScale,
  ProjectMemberGantt,
  ProjectNodeSubtaskGantt,
  ProjectStageGantt,
} from '@/types/gantt'
import type { ProjectMember } from '@/types/member'
import type { ProjectNodePayload } from '@/types/node'
import type { Subtask, SubtaskPayload, SubtaskProgressRecord } from '@/types/subtask'

type NodeReorderPayload = { nodes: Array<{ node_id: number; sequence_no: number }> }
type ProjectDetailViewMode = 'workspace' | 'gantt'

const route = useRoute()
const workspaceStore = useProjectWorkspaceStore()
const notificationStore = useNotificationStore()
const { isLoading, isSubtasksLoading, members, nodes, project, selectedNodeId, subtasks } =
  storeToRefs(workspaceStore)

const projectId = Number(route.params.projectId)
const showAddMemberDialog = ref(false)
const showNodeDialog = ref(false)
const showSubtaskDialog = ref(false)
const showProgressDialog = ref(false)
const showHistoryDrawer = ref(false)
const showNodeGanttDialog = ref(false)
const selectedSubtaskId = ref<number | null>(null)
const selectedGanttNodeId = ref<number | null>(null)
const historyTitle = ref('')
const progressRecords = ref<SubtaskProgressRecord[]>([])
const drawerNodeId = ref<number | null>(null)
const editingNodeId = ref<number | null>(null)
const editingSubtaskId = ref<number | null>(null)
const nodeDialogMode = ref<'create' | 'edit'>('create')
const subtaskDialogMode = ref<'create' | 'edit'>('create')
const nodeFormInitial = ref<ProjectNodePayload>({
  name: '',
  description: '',
  planned_start_date: '',
  planned_end_date: '',
})
const currentViewMode = ref<ProjectDetailViewMode>('workspace')
const ganttPerspective = ref<GanttPerspective>('stage')
const ganttScale = ref<GanttScale>('week')
const projectGantt = ref<ProjectStageGantt | null>(null)
const ganttLoadError = ref<string | null>(null)
const isGanttLoading = ref(false)
const projectMemberGantt = ref<ProjectMemberGantt | null>(null)
const memberGanttLoadError = ref<string | null>(null)
const isMemberGanttLoading = ref(false)
const nodeGantt = ref<ProjectNodeSubtaskGantt | null>(null)
const nodeGanttLoadError = ref<string | null>(null)
const isNodeGanttLoading = ref(false)
const nodeGanttCache = ref<Record<number, ProjectNodeSubtaskGantt>>({})
const subtaskFormInitial = ref<SubtaskPayload>({
  name: '',
  description: '',
  planned_start_date: '',
  planned_end_date: '',
  priority: 2,
})

const selectedSubtask = computed(
  () => subtasks.value.find((subtask) => subtask.id === selectedSubtaskId.value) ?? null,
)
const drawerNode = computed(
  () => nodes.value.find((node) => node.id === drawerNodeId.value) ?? null,
)
const isWorkspaceView = computed(() => currentViewMode.value === 'workspace')

const canManageMembers = computed(() => project.value?.permissions.can_manage_members ?? false)
const canManageNodes = computed(() => project.value?.permissions.can_manage_nodes ?? false)
const projectStartDate = computed(() => project.value?.planned_start_date)
const projectEndDate = computed(() => project.value?.planned_end_date)
const selectedNodeStartDate = computed(() => drawerNode.value?.planned_start_date ?? projectStartDate.value)
const selectedNodeEndDate = computed(() => drawerNode.value?.planned_end_date ?? projectEndDate.value)
const subtaskAssignees = computed<ProjectMember[]>(() => {
  if (project.value === null) {
    return members.value
  }

  const ownerId = project.value.owner_user_id
  const hasOwner = members.value.some((member) => member.user_id === ownerId)

  if (hasOwner) {
    return members.value
  }

  return [
    {
      project_id: project.value.id,
      user_id: ownerId,
      username: '',
      real_name: project.value.owner_real_name,
      system_role: 2,
      status: 1,
      joined_at: project.value.created_at,
      is_owner: true,
    },
    ...members.value,
  ]
})

onMounted(async () => {
  try {
    await workspaceStore.loadWorkspace(projectId)
  } catch (error) {
    notificationStore.notifyError(getErrorMessage(error, '项目工作台加载失败'))
  }
})

function invalidateGanttCache() {
  projectGantt.value = null
  ganttLoadError.value = null
  projectMemberGantt.value = null
  memberGanttLoadError.value = null
  nodeGantt.value = null
  nodeGanttLoadError.value = null
  selectedGanttNodeId.value = null
  showNodeGanttDialog.value = false
  nodeGanttCache.value = {}
}

async function loadProjectGantt(force = false) {
  if (!force && projectGantt.value !== null) {
    return
  }

  isGanttLoading.value = true
  ganttLoadError.value = null

  try {
    projectGantt.value = await getProjectGanttNodes(projectId)
  } catch (error) {
    ganttLoadError.value = getErrorMessage(error, '阶段甘特图加载失败')
  } finally {
    isGanttLoading.value = false
  }
}

async function loadProjectMemberGantt(force = false) {
  if (!force && projectMemberGantt.value !== null) {
    return
  }

  isMemberGanttLoading.value = true
  memberGanttLoadError.value = null

  try {
    projectMemberGantt.value = await getProjectGanttMembers(projectId)
  } catch (error) {
    memberGanttLoadError.value = getErrorMessage(error, '人员甘特图加载失败')
  } finally {
    isMemberGanttLoading.value = false
  }
}

async function loadNodeGantt(nodeId: number, force = false) {
  if (!force) {
    const cached = nodeGanttCache.value[nodeId]

    if (cached !== undefined) {
      nodeGantt.value = cached
      nodeGanttLoadError.value = null
      return
    }
  }

  isNodeGanttLoading.value = true
  nodeGanttLoadError.value = null
  nodeGantt.value = null

  try {
    const response = await getProjectNodeGantt(projectId, nodeId)
    nodeGantt.value = response
    nodeGanttCache.value = {
      ...nodeGanttCache.value,
      [nodeId]: response,
    }
  } catch (error) {
    nodeGanttLoadError.value = getErrorMessage(error, '阶段子任务甘特图加载失败')
  } finally {
    isNodeGanttLoading.value = false
  }
}

async function switchView(mode: ProjectDetailViewMode) {
  currentViewMode.value = mode

  if (mode === 'workspace') {
    showNodeGanttDialog.value = false
    return
  }

  if (ganttPerspective.value === 'member') {
    await loadProjectMemberGantt()
    return
  }

  await loadProjectGantt()
}

async function switchGanttPerspective(perspective: GanttPerspective) {
  ganttPerspective.value = perspective
  showNodeGanttDialog.value = false

  if (currentViewMode.value !== 'gantt') {
    return
  }

  if (perspective === 'member') {
    await loadProjectMemberGantt()
    return
  }

  await loadProjectGantt()
}

async function openNodeGantt(nodeId: number) {
  selectedGanttNodeId.value = nodeId
  showNodeGanttDialog.value = true
  await loadNodeGantt(nodeId)
}

async function retryNodeGantt() {
  if (selectedGanttNodeId.value === null) {
    return
  }

  await loadNodeGantt(selectedGanttNodeId.value, true)
}

async function refreshWorkspace() {
  const openNodeId = drawerNodeId.value
  await workspaceStore.loadWorkspace(projectId)
  invalidateGanttCache()

  if (openNodeId === null) {
    return
  }

  const nodeStillExists = nodes.value.some((node) => node.id === openNodeId)

  if (!nodeStillExists) {
    drawerNodeId.value = null
    return
  }

  drawerNodeId.value = openNodeId
  await workspaceStore.selectNode(projectId, openNodeId)
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
  const previousDrawerNodeId = drawerNodeId.value

  drawerNodeId.value = nodeId

  try {
    await workspaceStore.selectNode(projectId, nodeId)
  } catch (error) {
    drawerNodeId.value = previousDrawerNodeId
    notificationStore.notifyError(getErrorMessage(error, '节点任务加载失败'))
  }
}

function openCreateNodeDialog() {
  nodeDialogMode.value = 'create'
  editingNodeId.value = null
  nodeFormInitial.value = {
    name: '',
    description: '',
    planned_start_date: '',
    planned_end_date: '',
  }
  showNodeDialog.value = true
}

function openCreateSubtaskDialog() {
  if (drawerNode.value === null) {
    return
  }

  subtaskDialogMode.value = 'create'
  editingSubtaskId.value = null
  subtaskFormInitial.value = {
    name: '',
    description: '',
    planned_start_date: drawerNode.value.planned_start_date,
    planned_end_date: drawerNode.value.planned_end_date,
    priority: 2,
    responsible_user_id: project.value?.owner_user_id,
  }
  showSubtaskDialog.value = true
}

function applySubtaskToForm(subtask: Subtask) {
  subtaskFormInitial.value = {
    name: subtask.name,
    description: subtask.description,
    planned_start_date: subtask.planned_start_date,
    planned_end_date: subtask.planned_end_date,
    priority: subtask.priority,
    responsible_user_id: subtask.responsible_user_id,
  }
}

function openEditSubtaskDialog(subtaskId: number) {
  const subtask = subtasks.value.find((item) => item.id === subtaskId)

  if (!subtask) {
    notificationStore.notifyError('子任务详情加载失败')
    return
  }

  subtaskDialogMode.value = 'edit'
  editingSubtaskId.value = subtaskId
  applySubtaskToForm(subtask)
  showSubtaskDialog.value = true
}

async function openEditNodeDialog(nodeId: number) {
  try {
    const node = await getProjectNodeDetail(projectId, nodeId)
    nodeDialogMode.value = 'edit'
    editingNodeId.value = nodeId
    nodeFormInitial.value = {
      name: node.name,
      description: node.description,
      planned_start_date: node.planned_start_date,
      planned_end_date: node.planned_end_date,
    }
    showNodeDialog.value = true
  } catch (error) {
    notificationStore.notifyError(getErrorMessage(error, '节点详情加载失败'))
  }
}

async function submitNode(payload: ProjectNodePayload) {
  try {
    if (nodeDialogMode.value === 'create') {
      const createdNode = await createProjectNode(projectId, payload)
      showNodeDialog.value = false
      await refreshWorkspace()
      await handleSelectNode(createdNode.id)
      return
    }

    if (editingNodeId.value === null) {
      return
    }

    const nodeId = editingNodeId.value
    await updateProjectNode(projectId, nodeId, payload)
    showNodeDialog.value = false
    await refreshWorkspace()
    await handleSelectNode(nodeId)
  } catch (error) {
    notificationStore.notifyError(getErrorMessage(error, '节点保存失败'))
  }
}

async function submitSubtask(payload: SubtaskPayload) {
  if (drawerNode.value === null) {
    return
  }

  try {
    if (subtaskDialogMode.value === 'create') {
      await createSubtask(projectId, drawerNode.value.id, payload)
    } else {
      if (editingSubtaskId.value === null) {
        return
      }

      await updateSubtask(editingSubtaskId.value, payload)
    }

    showSubtaskDialog.value = false
    editingSubtaskId.value = null

    if (drawerNodeId.value !== null) {
      await refreshWorkspace()
      await handleSelectNode(drawerNodeId.value)
    }
  } catch (error) {
    notificationStore.notifyError(
      getErrorMessage(error, subtaskDialogMode.value === 'edit' ? '子任务保存失败' : '子任务创建失败'),
    )
  }
}

async function handleReopenSubtask(subtaskId: number) {
  try {
    await reopenSubtask(subtaskId)
    await refreshWorkspace()
  } catch (error) {
    notificationStore.notifyError(getErrorMessage(error, '撤销子任务完成失败'))
  }
}

async function handleRemoveSubtask(subtaskId: number) {
  if (!window.confirm('确认删除该子任务吗？')) {
    return
  }

  try {
    await deleteSubtask(subtaskId)
    await refreshWorkspace()
  } catch (error) {
    notificationStore.notifyError(getErrorMessage(error, '子任务删除失败'))
  }
}

async function handleStartNode(nodeId: number) {
  try {
    await startProjectNode(projectId, nodeId)
    await refreshWorkspace()
  } catch (error) {
    notificationStore.notifyError(getErrorMessage(error, '节点开始失败'))
  }
}

async function handleCompleteNode(nodeId: number) {
  try {
    await completeProjectNode(projectId, nodeId)
    await refreshWorkspace()
  } catch (error) {
    notificationStore.notifyError(getErrorMessage(error, '节点完成失败'))
  }
}

async function handleReopenNode(nodeId: number) {
  try {
    await reopenProjectNode(projectId, nodeId)
    await refreshWorkspace()
  } catch (error) {
    notificationStore.notifyError(getErrorMessage(error, '撤销节点完成失败'))
  }
}

async function handleRemoveNode(nodeId: number) {
  if (!window.confirm('确认删除该阶段节点吗？其下子任务也会一并删除。')) {
    return
  }

  try {
    await deleteProjectNode(projectId, nodeId)
    await refreshWorkspace()
  } catch (error) {
    notificationStore.notifyError(getErrorMessage(error, '节点删除失败'))
  }
}

function applyNodeReorder(payload: NodeReorderPayload) {
  const sequenceMap = new Map(payload.nodes.map((item) => [item.node_id, item.sequence_no]))

  nodes.value = [...nodes.value]
    .map((node) => {
      const nextSequence = sequenceMap.get(node.id)

      if (nextSequence === undefined) {
        return node
      }

      return {
        ...node,
        sequence_no: nextSequence,
      }
    })
    .sort((left, right) => left.sequence_no - right.sequence_no)
}

async function handleReorderNodes(payload: NodeReorderPayload) {
  const previousNodes = nodes.value

  applyNodeReorder(payload)

  try {
    const response = await reorderProjectNodes(projectId, payload)

    if (project.value !== null) {
      project.value = {
        ...project.value,
        updated_at: response.updated_at,
      }
    }
  } catch (error) {
    nodes.value = previousNodes
    notificationStore.notifyError(getErrorMessage(error, '节点顺序调整失败'))
  }
}

async function submitProgress(payload: {
  subtaskId: number
  progress_percent: number
  progress_note: string
}) {
  try {
    await submitSubtaskProgress(payload.subtaskId, {
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

      <section class="project-detail__view-switch" aria-label="项目详情视图">
        <button
          :class="['project-detail__view-button', { 'is-active': isWorkspaceView }]"
          type="button"
          @click="switchView('workspace')"
        >
          工作区
        </button>

        <button
          :class="['project-detail__view-button', { 'is-active': !isWorkspaceView }]"
          type="button"
          @click="switchView('gantt')"
        >
          甘特图
        </button>
      </section>

      <div v-if="isWorkspaceView" class="project-detail__grid">
        <section class="project-detail__workspace-card" data-testid="project-workspace-card">
          <div class="project-detail__workspace-rail">
            <NodeRail
              :can-manage="canManageNodes"
              :embedded="true"
              :nodes="nodes"
              :selected-node-id="drawerNodeId"
              @complete="handleCompleteNode"
              @create="openCreateNodeDialog"
              @edit="openEditNodeDialog"
              @remove="handleRemoveNode"
              @reorder="handleReorderNodes"
              @reopen="handleReopenNode"
              @select="handleSelectNode"
              @start="handleStartNode"
            />
          </div>

          <div class="project-detail__workspace-main">
            <Transition name="project-drawer">
              <NodeDrawer
                v-if="drawerNode"
                :key="drawerNode.id"
                :can-manage="canManageNodes"
                :embedded="true"
                :is-loading="isSubtasksLoading"
                :node="drawerNode"
                :subtasks="subtasks"
                @create="openCreateSubtaskDialog"
                @edit="openEditSubtaskDialog"
                @history="openHistoryDrawer"
                @progress="openProgressDialog"
                @remove="handleRemoveSubtask"
                @reopen="handleReopenSubtask"
              />

              <section
                v-else
                key="placeholder"
                data-testid="node-drawer-placeholder"
                class="project-detail__placeholder"
              >
                <div class="project-detail__placeholder-glow" aria-hidden="true" />
                <div class="project-detail__placeholder-copy">
                  <span>Node Drawer</span>
                  <strong>点击左侧未完成阶段，查看节点详情与子任务面板</strong>
                  <p>已完成节点保留在时间线中用于回看，进行中、未开始、已延期节点可直接展开。</p>
                </div>
              </section>
            </Transition>
          </div>
        </section>

        <div class="project-detail__members">
          <MemberPanel
            :can-manage="canManageMembers"
            :fixed-height="true"
            :members="members"
            @add="showAddMemberDialog = true"
            @remove="handleRemoveMember"
          />
        </div>
      </div>

      <ProjectGanttView
        v-else-if="ganttPerspective === 'stage'"
        :error="ganttLoadError"
        :gantt="projectGantt"
        :is-loading="isGanttLoading"
        :perspective="ganttPerspective"
        :scale="ganttScale"
        @open-node="openNodeGantt"
        @retry="loadProjectGantt(true)"
        @update:perspective="switchGanttPerspective"
        @update:scale="ganttScale = $event"
      />

      <ProjectMemberGanttView
        v-else
        :error="memberGanttLoadError"
        :gantt="projectMemberGantt"
        :is-loading="isMemberGanttLoading"
        :perspective="ganttPerspective"
        :scale="ganttScale"
        @retry="loadProjectMemberGantt(true)"
        @update:perspective="switchGanttPerspective"
        @update:scale="ganttScale = $event"
      />
    </template>

    <AddMemberDialog
      v-if="project"
      v-model="showAddMemberDialog"
      :project-id="project.id"
      @select="handleAddMember"
    />

    <NodeFormDialog
      v-model="showNodeDialog"
      :initial-value="nodeFormInitial"
      :max-date="projectEndDate"
      :min-date="projectStartDate"
      :mode="nodeDialogMode"
      @submit="submitNode"
    />

    <SubtaskFormDialog
      v-model="showSubtaskDialog"
      :assignees="subtaskAssignees"
      :default-responsible-user-id="project?.owner_user_id"
      :initial-value="subtaskFormInitial"
      :max-date="selectedNodeEndDate"
      :min-date="selectedNodeStartDate"
      :mode="subtaskDialogMode"
      @submit="submitSubtask"
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

    <NodeGanttDialog
      v-model="showNodeGanttDialog"
      :error="nodeGanttLoadError"
      :gantt="nodeGantt"
      :is-loading="isNodeGanttLoading"
      :scale="ganttScale"
      @retry="retryNodeGantt"
    />
  </section>
</template>

<style scoped>
.project-detail {
  display: grid;
  gap: 18px;
  padding: 0 8px 8px;
}

.project-detail__view-switch {
  display: inline-flex;
  align-items: center;
  gap: 8px;
  width: fit-content;
  padding: 6px;
  border: 1px solid color-mix(in srgb, var(--accent-line) 26%, var(--border-soft));
  border-radius: 999px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--glass-bg) 96%, transparent), color-mix(in srgb, var(--glass-bg) 82%, transparent)),
    radial-gradient(circle at top, color-mix(in srgb, var(--accent-end) 12%, transparent), transparent 64%);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
}

.project-detail__view-button {
  min-width: 92px;
  padding: 11px 18px;
  border: 0;
  border-radius: 999px;
  background: transparent;
  color: var(--text-soft);
  font-size: 0.84rem;
  font-weight: 600;
  cursor: pointer;
  transition:
    transform 180ms ease-out,
    color 180ms ease-out,
    background 180ms ease-out,
    box-shadow 180ms ease-out;
}

.project-detail__view-button:hover {
  transform: translateY(-1px);
  color: var(--text-main);
}

.project-detail__view-button.is-active {
  color: #f7fbff;
  background: linear-gradient(135deg, var(--accent-start), var(--accent-end));
  box-shadow:
    0 14px 28px color-mix(in srgb, var(--accent-end) 20%, transparent),
    inset 0 1px 0 color-mix(in srgb, #ffffff 32%, transparent);
}

.project-detail__grid {
  display: grid;
  grid-template-columns: minmax(0, 1fr) minmax(260px, 300px);
  gap: 18px;
  align-items: stretch;
}

.project-detail__workspace-card,
.project-detail__members {
  height: 720px;
  min-height: 720px;
}

.project-detail__workspace-card {
  overflow: hidden;
  display: grid;
  grid-template-columns: minmax(280px, 340px) minmax(0, 1fr);
  border: 1px solid var(--border-soft);
  border-radius: 28px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--glass-bg) 96%, transparent), var(--glass-bg)),
    radial-gradient(circle at top right, color-mix(in srgb, var(--accent-end) 12%, transparent), transparent 34%);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
}

.project-detail__workspace-rail,
.project-detail__workspace-main {
  min-width: 0;
  min-height: 0;
}

.project-detail__workspace-rail {
  padding: 22px 18px 22px 22px;
  border-right: 1px solid color-mix(in srgb, var(--border-soft) 92%, transparent);
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--glass-bg) 68%, transparent), transparent),
    radial-gradient(circle at top left, color-mix(in srgb, var(--accent-start) 10%, transparent), transparent 36%);
}

.project-detail__workspace-main {
  padding: 22px 22px 22px 18px;
}

.project-detail__members {
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

.project-detail__placeholder {
  position: relative;
  overflow: hidden;
  display: grid;
  place-items: center;
  min-height: 0;
  height: 100%;
  padding: 28px;
  border: 1px solid var(--border-soft);
  border-radius: 24px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--glass-bg) 96%, transparent), var(--glass-bg)),
    radial-gradient(circle at 78% 22%, color-mix(in srgb, var(--accent-end) 14%, transparent), transparent 28%),
    radial-gradient(circle at 18% 82%, color-mix(in srgb, var(--accent-start) 12%, transparent), transparent 34%);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
}

.project-detail__placeholder::before {
  content: '';
  position: absolute;
  inset: 18px;
  border: 1px solid color-mix(in srgb, var(--border-soft) 92%, transparent);
  border-radius: 20px;
  pointer-events: none;
}

.project-detail__placeholder-glow {
  position: absolute;
  width: 280px;
  height: 280px;
  border-radius: 50%;
  background: radial-gradient(circle, color-mix(in srgb, var(--accent-end) 20%, transparent), transparent 68%);
  filter: blur(8px);
  animation: placeholder-float 4.2s ease-in-out infinite;
}

.project-detail__placeholder-copy {
  position: relative;
  z-index: 1;
  display: grid;
  gap: 14px;
  max-width: 420px;
  text-align: center;
}

.project-detail__placeholder-copy span,
.project-detail__placeholder-copy strong,
.project-detail__placeholder-copy p {
  margin: 0;
}

.project-detail__placeholder-copy span {
  color: var(--text-soft);
  font-size: 0.8rem;
  font-weight: 600;
  letter-spacing: 0.16em;
  text-transform: uppercase;
}

.project-detail__placeholder-copy strong {
  font-size: clamp(1.2rem, 2vw, 1.6rem);
  line-height: 1.45;
}

.project-detail__placeholder-copy p {
  color: var(--text-soft);
  line-height: 1.75;
}

.project-drawer-enter-active,
.project-drawer-leave-active {
  transition:
    opacity 240ms ease-out,
    transform 240ms ease-out;
}

.project-drawer-enter-from,
.project-drawer-leave-to {
  opacity: 0;
  transform: translateX(24px);
}

@keyframes placeholder-float {
  0%,
  100% {
    transform: translate3d(0, 0, 0) scale(1);
  }

  50% {
    transform: translate3d(10px, -12px, 0) scale(1.04);
  }
}

@media (max-width: 1180px) {
  .project-detail__grid {
    grid-template-columns: 1fr;
  }

  .project-detail__workspace-card,
  .project-detail__members {
    height: auto;
    min-height: 0;
  }

  .project-detail__workspace-card {
    grid-template-columns: 1fr;
  }

  .project-detail__workspace-rail {
    padding-right: 22px;
    border-right: none;
    border-bottom: 1px solid color-mix(in srgb, var(--border-soft) 92%, transparent);
  }

  .project-detail__workspace-main {
    padding-left: 22px;
  }

  .project-detail__placeholder {
    min-height: 420px;
  }
}
</style>
