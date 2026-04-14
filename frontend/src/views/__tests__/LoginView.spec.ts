import { defineComponent, nextTick } from 'vue'
import { createPinia, setActivePinia } from 'pinia'
import { render } from '@testing-library/vue'
import userEvent from '@testing-library/user-event'
import { createMemoryHistory, createRouter } from 'vue-router'
import { readFileSync } from 'node:fs'
import { resolve } from 'node:path'
import { beforeEach, describe, expect, it, vi } from 'vitest'

import { useAuthStore } from '@/stores/auth'
import LoginView from '@/views/LoginView.vue'

describe('LoginView', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
  })

  it('renders a compact single-card login layout without the old hero marketing panel', async () => {
    const pinia = createPinia()
    setActivePinia(pinia)
    const router = createRouter({
      history: createMemoryHistory(),
      routes: [{ path: '/login', component: LoginView }],
    })
    router.push('/login')
    await router.isReady()

    const screen = render(LoginView, {
      global: {
        plugins: [router, pinia],
      },
    })

    expect(screen.container.querySelectorAll('.login-card')).toHaveLength(1)
    expect(screen.queryByText('Control Surface')).toBeNull()
    expect(screen.queryByText('双主题')).toBeNull()
    expect(screen.queryByText('高密度')).toBeNull()
    expect(screen.queryByText('实时调度')).toBeNull()
  })

  it('defines a plain full-screen login background surface without bitmap imports or inline background-image binding', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/views/LoginView.vue'), 'utf8')

    expect(source).not.toContain("import skyImage from '@/assets/login/sky.png'")
    expect(source).not.toContain("import cat3Image from '@/assets/login/cat3.png'")
    expect(source).not.toContain("import { useThemeStore } from '@/stores/theme'")
    expect(source).not.toContain("const isDark = computed(() => themeStore.mode === 'dark')")
    expect(source).not.toContain('const activeBackgroundImage = computed(() => `url(${isDark.value ? cat3Image : skyImage})`)')
    expect(source).not.toContain('new Image()')
    expect(source).toContain('data-testid="login-background"')
    expect(source).not.toContain(":style=\"{ backgroundImage: activeBackgroundImage }\"")
  })

  it('keeps the login background lightweight by avoiding duplicated full-screen layers, blur and zoom transitions', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/views/LoginView.vue'), 'utf8')

    expect(source).toContain('.login-view__background {')
    expect(source).not.toContain('.login-view__scrim')
    expect(source).not.toContain('login-view__background--light')
    expect(source).not.toContain('login-view__background--dark')
    expect(source).not.toContain('.login-view::before')
    expect(source).not.toContain('--login-background-image')
    expect(source).not.toContain('backdrop-filter: blur(3px);')
    expect(source).not.toContain('transform: scale(1.03);')
  })

  it('submits credentials and redirects to the projects page after login', async () => {
    const pinia = createPinia()
    setActivePinia(pinia)

    const router = createRouter({
      history: createMemoryHistory(),
      routes: [
        { path: '/login', component: LoginView },
        { path: '/projects', component: defineComponent({ template: '<div>projects</div>' }) },
      ],
    })

    router.push('/login')
    await router.isReady()

    const store = useAuthStore()
    const loginSpy = vi.spyOn(store, 'login').mockResolvedValue()

    const screen = render(LoginView, {
      global: {
        plugins: [router, pinia],
      },
    })
    const user = userEvent.setup()

    await user.type(screen.getByTestId('login-username'), 'zhangsan')
    await user.type(screen.getByTestId('login-password'), '123456')
    await user.click(screen.getByRole('button', { name: '进入项目台' }))

    expect(loginSpy).toHaveBeenCalledWith({
      username: 'zhangsan',
      password: '123456',
    })

    await nextTick()

    expect(router.currentRoute.value.fullPath).toBe('/projects')
  })
})
