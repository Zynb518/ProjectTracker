import { http, unwrapResponse } from '@/api/http'
import type { PaginatedList } from '@/types/pagination'
import type {
  Subtask,
  SubtaskPayload,
  SubtaskProgressPayload,
  SubtaskProgressRecord,
} from '@/types/subtask'

export async function listNodeSubtasks(
  projectId: number,
  nodeId: number,
  query: { status?: number; responsible_user_id?: number },
): Promise<{ list: Subtask[] }> {
  return unwrapResponse<{ list: Subtask[] }>(
    http.get(`/api/projects/${projectId}/nodes/${nodeId}/subtasks`, { params: query }),
  )
}

export async function listMySubtasks(query: { status?: number; project_id?: number }): Promise<{ list: Subtask[] }> {
  return unwrapResponse<{ list: Subtask[] }>(http.get('/api/my/subtasks', { params: query }))
}

export async function createSubtask(projectId: number, nodeId: number, payload: SubtaskPayload): Promise<Subtask> {
  return unwrapResponse<Subtask>(http.post(`/api/projects/${projectId}/nodes/${nodeId}/subtasks`, payload))
}

export async function updateSubtask(subtaskId: number, payload: Partial<SubtaskPayload>): Promise<Subtask> {
  return unwrapResponse<Subtask>(http.patch(`/api/subtasks/${subtaskId}`, payload))
}

export async function deleteSubtask(subtaskId: number): Promise<{ id: number }> {
  return unwrapResponse<{ id: number }>(http.delete(`/api/subtasks/${subtaskId}`))
}

export async function startSubtask(
  subtaskId: number,
  progress_note = '',
): Promise<{ subtask: Partial<Subtask>; progress_record: SubtaskProgressRecord }> {
  return unwrapResponse<{ subtask: Partial<Subtask>; progress_record: SubtaskProgressRecord }>(
    http.post(`/api/subtasks/${subtaskId}/start`, { progress_note }),
  )
}

export async function reopenSubtask(subtaskId: number): Promise<Partial<Subtask>> {
  return unwrapResponse<Partial<Subtask>>(http.post(`/api/subtasks/${subtaskId}/reopen`, {}))
}

export async function submitSubtaskProgress(
  subtaskId: number,
  payload: SubtaskProgressPayload,
): Promise<{ subtask: Partial<Subtask>; progress_record: SubtaskProgressRecord }> {
  return unwrapResponse<{ subtask: Partial<Subtask>; progress_record: SubtaskProgressRecord }>(
    http.post(`/api/subtasks/${subtaskId}/progress`, payload),
  )
}

export async function listSubtaskProgressRecords(
  subtaskId: number,
  query: { page?: number; page_size?: number },
): Promise<PaginatedList<SubtaskProgressRecord>> {
  return unwrapResponse<PaginatedList<SubtaskProgressRecord>>(
    http.get(`/api/subtasks/${subtaskId}/progress`, { params: query }),
  )
}
