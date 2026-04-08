import { computed, ref } from 'vue'
import { defineStore } from 'pinia'

import { listProjectMembers } from '@/api/members'
import { listProjectNodes } from '@/api/nodes'
import { getProjectDetail } from '@/api/projects'
import { listNodeSubtasks } from '@/api/subtasks'
import type { ProjectMember } from '@/types/member'
import type { ProjectNode } from '@/types/node'
import type { ProjectDetail } from '@/types/project'
import type { Subtask } from '@/types/subtask'

export const useProjectWorkspaceStore = defineStore('project-workspace', () => {
  const project = ref<ProjectDetail | null>(null)
  const members = ref<ProjectMember[]>([])
  const nodes = ref<ProjectNode[]>([])
  const subtasks = ref<Subtask[]>([])
  const selectedNodeId = ref<number | null>(null)
  const isLoading = ref(false)
  const isSubtasksLoading = ref(false)

  const selectedNode = computed(
    () => nodes.value.find((node) => node.id === selectedNodeId.value) ?? null,
  )

  async function loadWorkspace(projectId: number) {
    isLoading.value = true

    try {
      const [projectResponse, membersResponse, nodesResponse] = await Promise.all([
        getProjectDetail(projectId),
        listProjectMembers(projectId),
        listProjectNodes(projectId),
      ])

      project.value = projectResponse
      members.value = membersResponse.list
      nodes.value = nodesResponse.list

      if (nodes.value.length > 0) {
        await selectNode(projectId, nodes.value[0].id)
      } else {
        selectedNodeId.value = null
        subtasks.value = []
      }
    } finally {
      isLoading.value = false
    }
  }

  async function selectNode(
    projectId: number,
    nodeId: number,
    filters: { status?: number; responsible_user_id?: number } = {},
  ) {
    selectedNodeId.value = nodeId
    isSubtasksLoading.value = true

    try {
      const response = await listNodeSubtasks(projectId, nodeId, filters)
      subtasks.value = response.list
    } finally {
      isSubtasksLoading.value = false
    }
  }

  return {
    isLoading,
    isSubtasksLoading,
    loadWorkspace,
    members,
    nodes,
    project,
    selectedNode,
    selectedNodeId,
    selectNode,
    subtasks,
  }
})
