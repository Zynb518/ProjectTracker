import { http, unwrapResponse } from '@/api/http'
import type { PaginatedList } from '@/types/pagination'
import type {
  ProjectDetail,
  ProjectFormPayload,
  ProjectListItem,
  ProjectListQuery,
  ProjectMutationResult,
} from '@/types/project'

export async function listProjects(query: ProjectListQuery): Promise<PaginatedList<ProjectListItem>> {
  return unwrapResponse<PaginatedList<ProjectListItem>>(http.get('/api/projects', { params: query }))
}

export async function getProjectDetail(projectId: number): Promise<ProjectDetail> {
  return unwrapResponse<ProjectDetail>(http.get(`/api/projects/${projectId}`))
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
