import { http, unwrapResponse } from '@/api/http'
import type { ProjectNode, ProjectNodePayload } from '@/types/node'

export async function listProjectNodes(projectId: number): Promise<{ list: ProjectNode[] }> {
  return unwrapResponse<{ list: ProjectNode[] }>(http.get(`/api/projects/${projectId}/nodes`))
}

export async function getProjectNodeDetail(projectId: number, nodeId: number): Promise<ProjectNode> {
  return unwrapResponse<ProjectNode>(http.get(`/api/projects/${projectId}/nodes/${nodeId}`))
}

export async function createProjectNode(projectId: number, payload: ProjectNodePayload): Promise<ProjectNode> {
  return unwrapResponse<ProjectNode>(http.post(`/api/projects/${projectId}/nodes`, payload))
}

export async function updateProjectNode(
  projectId: number,
  nodeId: number,
  payload: Partial<ProjectNodePayload>,
): Promise<ProjectNode> {
  return unwrapResponse<ProjectNode>(http.patch(`/api/projects/${projectId}/nodes/${nodeId}`, payload))
}

export async function deleteProjectNode(projectId: number, nodeId: number): Promise<{ id: number }> {
  return unwrapResponse<{ id: number }>(http.delete(`/api/projects/${projectId}/nodes/${nodeId}`))
}

export async function startProjectNode(projectId: number, nodeId: number): Promise<{ id: number; status: number }> {
  return unwrapResponse<{ id: number; status: number }>(http.post(`/api/projects/${projectId}/nodes/${nodeId}/start`, {}))
}

export async function completeProjectNode(projectId: number, nodeId: number): Promise<{ id: number; status: number }> {
  return unwrapResponse<{ id: number; status: number }>(
    http.post(`/api/projects/${projectId}/nodes/${nodeId}/complete`, {}),
  )
}

export async function reopenProjectNode(projectId: number, nodeId: number): Promise<{ id: number; status: number }> {
  return unwrapResponse<{ id: number; status: number }>(
    http.post(`/api/projects/${projectId}/nodes/${nodeId}/reopen`, {}),
  )
}

export async function reorderProjectNodes(
  projectId: number,
  payload: { nodes: Array<{ node_id: number; sequence_no: number }> },
): Promise<{ project_id: number; updated_at: string }> {
  return unwrapResponse<{ project_id: number; updated_at: string }>(
    http.put(`/api/projects/${projectId}/nodes/reorder`, payload),
  )
}
