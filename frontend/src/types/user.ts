import type { UserProfile } from '@/types/auth'

export type UserListItem = UserProfile

export type UserDetail = UserProfile

export interface UserListQuery {
  keyword?: string
  system_role?: number
  status?: number
  page?: number
  page_size?: number
}

export interface CreateUserPayload {
  username: string
  password: string
  real_name: string
  system_role: number
  email: string
  phone: string
}

export interface UpdateUserPayload {
  real_name?: string
  system_role?: number
  email?: string
  phone?: string
}

export interface UpdateUserStatusPayload {
  status: 1 | 2
}

export interface UserMutationResult {
  id: number
  username?: string
  real_name?: string
  system_role?: number
  email?: string
  phone?: string
  status?: number
  created_at?: string
  updated_at: string
}

export interface UserStatusMutationResult {
  id: number
  status: number
  updated_at: string
}
