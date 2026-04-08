export interface Subtask {
  id: number
  node_id: number
  name: string
  description: string
  responsible_user_id: number
  responsible_real_name?: string
  status: number
  progress_percent: number
  priority: number
  planned_start_date: string
  planned_end_date: string
  completed_at: string | null
  created_by?: number
  created_at?: string
  updated_at: string
  project_id?: number
  project_name?: string
  node_name?: string
}

export interface SubtaskPayload {
  name: string
  description: string
  responsible_user_id?: number
  priority?: number
  planned_start_date: string
  planned_end_date: string
}

export interface SubtaskProgressPayload {
  status: number
  progress_percent: number
  progress_note?: string
}

export interface SubtaskProgressRecord {
  id: number
  sub_task_id: number
  operator_user_id: number
  operator_real_name?: string
  progress_note: string
  progress_percent: number
  status: number
  created_at: string
}
