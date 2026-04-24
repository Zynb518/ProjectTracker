import type { ProjectNodePayload } from '@/types/node'
import type { ProjectFormPayload } from '@/types/project'
import type { SubtaskPayload } from '@/types/subtask'

export interface AiProjectDraftSubtask extends SubtaskPayload {
  responsible_user_id?: number
}

export interface AiProjectDraftNode extends ProjectNodePayload {
  subtasks: AiProjectDraftSubtask[]
}

export interface AiProjectDraft {
  project: ProjectFormPayload
  nodes: AiProjectDraftNode[]
}
