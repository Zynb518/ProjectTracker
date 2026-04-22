export type GanttScale = 'day' | 'week' | 'month'
export type GanttPerspective = 'member' | 'stage'

export interface GanttProjectSummary {
  id: number
  name: string
  owner_user_id: number
  owner_real_name: string
  status: number
  planned_start_date: string
  planned_end_date: string
  completed_at: string | null
}

export interface GanttNodeSummary {
  id: number
  name: string
  sequence_no: number
  status: number
  planned_start_date: string
  planned_end_date: string
  completed_at: string | null
}

export interface GanttSubtaskSummary {
  id: number
  node_id: number
  node_name: string
  name: string
  responsible_user_id: number
  responsible_real_name: string
  status: number
  progress_percent: number
  priority: number
  planned_start_date: string
  planned_end_date: string
  completed_at: string | null
}

export interface GanttMemberSubtaskSummary {
  subtask_id: number
  name: string
  node_id: number
  node_name: string
  status: number
  progress_percent: number
  planned_start_date: string
  planned_end_date: string
  completed_at: string | null
}

export interface GanttMemberRowSummary {
  user_id: number
  real_name: string
  subtasks: GanttMemberSubtaskSummary[]
}

export interface ProjectStageGantt {
  project: GanttProjectSummary
  nodes: GanttNodeSummary[]
}

export interface ProjectNodeSubtaskGantt {
  project: GanttProjectSummary
  node: GanttNodeSummary
  subtasks: GanttSubtaskSummary[]
}

export interface ProjectMemberGantt {
  project: GanttProjectSummary
  member_rows: GanttMemberRowSummary[]
}

export interface GanttNodeResizePayload {
  nodeId: number
  plannedStartDate: string
  plannedEndDate: string
}

export interface GanttSubtaskResizePayload {
  nodeId: number
  subtaskId: number
  plannedStartDate: string
  plannedEndDate: string
}
