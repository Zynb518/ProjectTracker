export interface ProjectNode {
  id: number
  project_id: number
  name: string
  description: string
  sequence_no: number
  status: number
  planned_start_date: string
  planned_end_date: string
  completed_at: string | null
  created_by: number
  created_at: string
  updated_at: string
  sub_task_count: number
  completed_sub_task_count: number
}

export interface ProjectNodePayload {
  name: string
  description: string
  planned_start_date: string
  planned_end_date: string
}
