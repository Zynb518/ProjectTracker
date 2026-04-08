export interface UserProfile {
  id: number
  username: string
  real_name: string
  system_role: number
  email: string
  phone: string
  status: number
  created_at: string
  updated_at: string
}

export interface LoginPayload {
  username: string
  password: string
}

export interface LoginResponse {
  user: UserProfile
}
