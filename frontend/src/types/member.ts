export interface ProjectMember {
  project_id: number
  user_id: number
  username: string
  real_name: string
  system_role: number
  status: number
  joined_at: string
  is_owner: boolean
}

export interface MemberCandidate {
  id: number
  username: string
  real_name: string
  system_role: number
  status: number
}
