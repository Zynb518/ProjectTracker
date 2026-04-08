import { http, unwrapResponse } from '@/api/http'
import type { MemberCandidate, ProjectMember } from '@/types/member'
import type { PaginatedList } from '@/types/pagination'

export async function listProjectMembers(projectId: number): Promise<{ list: ProjectMember[] }> {
  return unwrapResponse<{ list: ProjectMember[] }>(http.get(`/api/projects/${projectId}/members`))
}

export async function listMemberCandidates(
  projectId: number,
  query: { keyword?: string; page?: number; page_size?: number },
): Promise<PaginatedList<MemberCandidate>> {
  return unwrapResponse<PaginatedList<MemberCandidate>>(
    http.get(`/api/projects/${projectId}/members/candidates`, { params: query }),
  )
}

export async function addProjectMember(projectId: number, userId: number): Promise<{ project_id: number; user_id: number }> {
  return unwrapResponse<{ project_id: number; user_id: number }>(
    http.post(`/api/projects/${projectId}/members`, { user_id: userId }),
  )
}

export async function removeProjectMember(projectId: number, userId: number): Promise<{ project_id: number; user_id: number }> {
  return unwrapResponse<{ project_id: number; user_id: number }>(
    http.delete(`/api/projects/${projectId}/members/${userId}`),
  )
}
