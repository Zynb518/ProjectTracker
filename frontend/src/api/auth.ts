import { http, unwrapResponse } from '@/api/http'
import type { LoginPayload, LoginResponse, UserProfile } from '@/types/auth'

export async function login(payload: LoginPayload): Promise<UserProfile> {
  const data = await unwrapResponse<LoginResponse>(http.post('/api/auth/login', payload))
  return data.user
}

export async function logout(): Promise<void> {
  await unwrapResponse<null>(http.post('/api/auth/logout', {}))
}

export async function getCurrentUser(): Promise<UserProfile> {
  return unwrapResponse<UserProfile>(http.get('/api/auth/me'))
}
