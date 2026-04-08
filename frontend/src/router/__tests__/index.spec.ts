import { createPinia, setActivePinia } from 'pinia'
import { beforeEach, describe, expect, it, vi } from 'vitest'

import { createAppRouter } from '@/router'
import { useAuthStore } from '@/stores/auth'

describe('app router', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
  })

  it('redirects guests from protected routes to login', async () => {
    const pinia = createPinia()
    setActivePinia(pinia)
    const store = useAuthStore(pinia)
    vi.spyOn(store, 'initializeSession').mockImplementation(async () => {
      store.currentUser = null
      store.isReady = true
    })

    const router = createAppRouter(pinia)

    await router.push('/projects')

    expect(router.currentRoute.value.fullPath).toBe('/login')
  })

  it('allows authenticated users to access protected routes', async () => {
    const pinia = createPinia()
    setActivePinia(pinia)

    const store = useAuthStore(pinia)
    store.currentUser = {
      id: 1,
      username: 'zhangsan',
      real_name: '张三',
      system_role: 2,
      email: 'zhangsan@example.com',
      phone: '13800000000',
      status: 1,
      created_at: '2026-03-27T09:00:00+08:00',
      updated_at: '2026-03-27T09:00:00+08:00',
    }
    store.isReady = true

    const router = createAppRouter(pinia)
    await router.push('/projects')

    expect(router.currentRoute.value.fullPath).toBe('/projects')
  })

  it('hydrates the session before deciding whether a protected route is allowed', async () => {
    const pinia = createPinia()
    setActivePinia(pinia)

    const store = useAuthStore(pinia)
    const initializeSpy = vi.spyOn(store, 'initializeSession').mockImplementation(async () => {
      store.currentUser = {
        id: 1,
        username: 'zhangsan',
        real_name: '张三',
        system_role: 2,
        email: 'zhangsan@example.com',
        phone: '13800000000',
        status: 1,
        created_at: '2026-03-27T09:00:00+08:00',
        updated_at: '2026-03-27T09:00:00+08:00',
      }
      store.isReady = true
    })

    const router = createAppRouter(pinia)
    await router.push('/projects')

    expect(initializeSpy).toHaveBeenCalledTimes(1)
    expect(router.currentRoute.value.fullPath).toBe('/projects')
  })

  it('allows navigation to login even if session bootstrap fails', async () => {
    const pinia = createPinia()
    setActivePinia(pinia)

    const store = useAuthStore(pinia)
    vi.spyOn(store, 'initializeSession').mockRejectedValue(new Error('network down'))

    const router = createAppRouter(pinia)

    await expect(router.push('/login')).resolves.toBeUndefined()
    expect(router.currentRoute.value.fullPath).toBe('/login')
  })
})
