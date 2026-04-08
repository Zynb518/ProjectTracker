import { beforeEach, describe, expect, it, vi } from 'vitest'
import { createPinia, setActivePinia } from 'pinia'

vi.mock('@/api/auth', () => ({
  getCurrentUser: vi.fn(),
  login: vi.fn(),
  logout: vi.fn(),
}))

import { getCurrentUser, logout } from '@/api/auth'
import { useAuthStore } from '@/stores/auth'

describe('auth store', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
    vi.resetAllMocks()
  })

  it('hydrates the current user from the session endpoint', async () => {
    vi.mocked(getCurrentUser).mockResolvedValue({
      id: 1,
      username: 'zhangsan',
      real_name: '张三',
      system_role: 2,
      email: 'zhangsan@example.com',
      phone: '13800000000',
      status: 1,
      created_at: '2026-03-27T09:00:00+08:00',
      updated_at: '2026-03-27T09:00:00+08:00',
    })

    const store = useAuthStore()
    await store.initializeSession()

    expect(store.currentUser?.username).toBe('zhangsan')
    expect(store.isReady).toBe(true)
  })

  it('clears the current user after logout', async () => {
    vi.mocked(getCurrentUser).mockResolvedValue({
      id: 1,
      username: 'zhangsan',
      real_name: '张三',
      system_role: 2,
      email: 'zhangsan@example.com',
      phone: '13800000000',
      status: 1,
      created_at: '2026-03-27T09:00:00+08:00',
      updated_at: '2026-03-27T09:00:00+08:00',
    })
    vi.mocked(logout).mockResolvedValue()

    const store = useAuthStore()
    await store.initializeSession()
    await store.logout()

    expect(store.currentUser).toBeNull()
  })
})
