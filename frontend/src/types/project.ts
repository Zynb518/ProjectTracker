export interface ProjectListItem {
  id: number
  name: string
  description: string
  owner_user_id: number
  owner_real_name: string
  status: number
  planned_start_date: string
  planned_end_date: string
  completed_at: string | null
  member_count: number
  node_count: number
  sub_task_count: number
  created_at: string
  updated_at: string
  is_owner: boolean
}

export interface ProjectListQuery {
  keyword?: string
  status?: number
  owner_user_id?: number
  page?: number
  page_size?: number
}

export interface ProjectPermissions {
  can_edit_basic: boolean
  can_manage_members: boolean
  can_manage_nodes: boolean
  can_transfer_owner: boolean
  can_delete: boolean
}

export interface ProjectDetail {
  id: number
  name: string
  description: string
  owner_user_id: number
  owner_real_name: string
  status: number
  planned_start_date: string
  planned_end_date: string
  completed_at: string | null
  created_by: number
  created_by_real_name: string
  created_at: string
  updated_at: string
  member_count: number
  node_count: number
  completed_node_count: number
  sub_task_count: number
  completed_sub_task_count: number
  permissions: ProjectPermissions
}

export interface ProjectFormPayload {
  name: string
  description: string
  planned_start_date: string
  planned_end_date: string
}

export interface ProjectMutationResult {
  id: number
  name?: string
  description?: string
  status?: number
  planned_start_date?: string
  planned_end_date?: string
  completed_at?: string | null
  updated_at?: string
}
