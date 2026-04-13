import { http, unwrapResponse } from '@/api/http'
import type { ProjectMemberGantt, ProjectStageGantt } from '@/types/gantt'
import type { PaginatedList } from '@/types/pagination'
import type {
  ProjectDetail,
  ProjectFormPayload,
  ProjectListItem,
  ProjectListQuery,
  ProjectOwnerCandidate,
  ProjectOwnerCandidateQuery,
  ProjectOwnerTransferResult,
  ProjectMutationResult,
} from '@/types/project'

export async function listProjects(query: ProjectListQuery): Promise<PaginatedList<ProjectListItem>> {
  return unwrapResponse<PaginatedList<ProjectListItem>>(http.get('/api/projects', { params: query }))
}

export async function getProjectDetail(projectId: number): Promise<ProjectDetail> {
  return unwrapResponse<ProjectDetail>(http.get(`/api/projects/${projectId}`))
}

export async function getProjectGanttNodes(projectId: number): Promise<ProjectStageGantt> {
  return unwrapResponse<ProjectStageGantt>(http.get(`/api/projects/${projectId}/gantt/nodes`))
}

export async function getProjectGanttMembers(projectId: number): Promise<ProjectMemberGantt> {
  return unwrapResponse<ProjectMemberGantt>(http.get(`/api/projects/${projectId}/gantt/members`))
}

export async function createProject(payload: ProjectFormPayload): Promise<ProjectMutationResult> {
  return unwrapResponse<ProjectMutationResult>(http.post('/api/projects', payload))
}

export async function updateProject(
  projectId: number,
  payload: Partial<ProjectFormPayload>,
): Promise<ProjectMutationResult> {
  return unwrapResponse<ProjectMutationResult>(http.patch(`/api/projects/${projectId}`, payload))
}

export async function deleteProject(projectId: number): Promise<{ id: number }> {
  return unwrapResponse<{ id: number }>(http.delete(`/api/projects/${projectId}`))
}

export async function startProject(projectId: number): Promise<ProjectMutationResult> {
  return unwrapResponse<ProjectMutationResult>(http.post(`/api/projects/${projectId}/start`, {}))
}

export async function completeProject(projectId: number): Promise<ProjectMutationResult> {
  return unwrapResponse<ProjectMutationResult>(http.post(`/api/projects/${projectId}/complete`, {}))
}

export async function reopenProject(projectId: number): Promise<ProjectMutationResult> {
  return unwrapResponse<ProjectMutationResult>(http.post(`/api/projects/${projectId}/reopen`, {}))
}

export async function listProjectOwnerCandidates(
  projectId: number,
  query: ProjectOwnerCandidateQuery,
): Promise<PaginatedList<ProjectOwnerCandidate>> {
  return unwrapResponse<PaginatedList<ProjectOwnerCandidate>>(
    http.get(`/api/projects/${projectId}/owner/candidates`, { params: query }),
  )
}

export async function transferProjectOwner(
  projectId: number,
  targetUserId: number,
): Promise<ProjectOwnerTransferResult> {
  return unwrapResponse<ProjectOwnerTransferResult>(
    http.post(`/api/projects/${projectId}/owner/transfer`, {
      target_user_id: targetUserId,
    }),
  )
}
