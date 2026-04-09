import { http, unwrapResponse } from '@/api/http'
import type { PaginatedList } from '@/types/pagination'
import type {
  CreateUserPayload,
  UpdateUserPayload,
  UpdateUserStatusPayload,
  UserDetail,
  UserListItem,
  UserListQuery,
  UserMutationResult,
  UserStatusMutationResult,
} from '@/types/user'

export async function listUsers(query: UserListQuery): Promise<PaginatedList<UserListItem>> {
  return unwrapResponse<PaginatedList<UserListItem>>(http.get('/api/users', { params: query }))
}

export async function getUserDetail(userId: number): Promise<UserDetail> {
  return unwrapResponse<UserDetail>(http.get(`/api/users/${userId}`))
}

export async function createUser(payload: CreateUserPayload): Promise<UserMutationResult> {
  return unwrapResponse<UserMutationResult>(http.post('/api/users', payload))
}

export async function updateUser(
  userId: number,
  payload: UpdateUserPayload,
): Promise<UserMutationResult> {
  return unwrapResponse<UserMutationResult>(http.patch(`/api/users/${userId}`, payload))
}

export async function updateUserStatus(
  userId: number,
  payload: UpdateUserStatusPayload,
): Promise<UserStatusMutationResult> {
  return unwrapResponse<UserStatusMutationResult>(http.patch(`/api/users/${userId}/status`, payload))
}
