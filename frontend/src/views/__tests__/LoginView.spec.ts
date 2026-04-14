import { defineComponent, nextTick } from 'vue'
import { createPinia, setActivePinia } from 'pinia'
import { render } from '@testing-library/vue'
import userEvent from '@testing-library/user-event'
import { createMemoryHistory, createRouter } from 'vue-router'
import { existsSync, readFileSync } from 'node:fs'
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

  it('applies a login-specific class to the theme toggle so the toolbar control can visually match the cinematic card', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/views/LoginView.vue'), 'utf8')

    expect(source).toContain('<ThemeToggle class="login-view__theme-toggle" />')
    expect(source).toContain('.login-view .login-view__theme-toggle {')
    expect(source).toContain('.login-view .login-view__theme-toggle .theme-toggle__label {')
  })

  it('keeps the login background declarative in CSS instead of switching images through script bindings', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/views/LoginView.vue'), 'utf8')

    expect(source).not.toContain("import { useThemeStore } from '@/stores/theme'")
    expect(source).not.toContain("const isDark = computed(() => themeStore.mode === 'dark')")
    expect(source).not.toContain('const activeBackgroundImage = computed(() => `url(${isDark.value ? cat3Image : skyImage})`)')
    expect(source).not.toContain('new Image()')
    expect(source).toContain('data-testid="login-background"')
    expect(source).not.toContain(":style=\"{ backgroundImage: activeBackgroundImage }\"")
    expect(source).toContain("url('../assets/login/748069.png')")
  })

  it('removes the unused legacy cat illustration asset from the login surface bundle', () => {
    expect(existsSync(resolve(process.cwd(), 'src/assets/login/cat3.png'))).toBe(false)
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

  it('uses the new light-theme bitmap as the login background with lightweight overlay treatment', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/views/LoginView.vue'), 'utf8')

    expect(source).toContain("url('../assets/login/748069.png')")
    expect(source).toContain('background-position: center center;')
    expect(source).toContain('background-size: cover;')
    expect(source).toContain(
      'linear-gradient(180deg, rgba(244, 249, 255, 0.26), rgba(223, 234, 248, 0.14))',
    )
    expect(source).toContain(
      'linear-gradient(180deg, rgba(8, 17, 34, 0.22), rgba(8, 17, 34, 0.42))',
    )
    expect(source).toContain('0 30px 60px rgba(7, 17, 34, 0.24)')
    expect(source).toContain('inset 0 1px 0 rgba(255, 255, 255, 0.58)')
  })

  it('keeps the light login visuals cheap to paint by using one bitmap plus a small overlay stack', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/views/LoginView.vue'), 'utf8')

    const lightBackgroundBlock = source.match(
      /html\.light \.login-view__background,[\s\S]*?background-image:([\s\S]*?)background-repeat:/,
    )?.[1]

    expect(lightBackgroundBlock).toBeTruthy()
    expect((lightBackgroundBlock?.match(/url\(/g) ?? []).length).toBe(1)
    expect((lightBackgroundBlock?.match(/linear-gradient\(/g) ?? []).length).toBeLessThanOrEqual(2)
    expect((lightBackgroundBlock?.match(/radial-gradient\(/g) ?? []).length).toBeLessThanOrEqual(2)
    expect(source).not.toContain('backdrop-filter: blur(18px) saturate(1.15);')
    expect(source).not.toContain('filter: blur(20px);')
  })

  it('styles the light login card like the project list fog-white cards with a translucent white surface', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/views/LoginView.vue'), 'utf8')

    expect(source).toContain(
      'linear-gradient(180deg, rgba(249, 252, 255, 0.82), rgba(238, 245, 253, 0.68))',
    )
    expect(source).toContain(
      'linear-gradient(145deg, rgba(255, 255, 255, 0.48), rgba(209, 228, 248, 0.12) 54%)',
    )
    expect(source).toContain(
      '0 18px 36px rgba(67, 94, 127, 0.14)',
    )
    expect(source).toContain(
      'color: #21324b;',
    )
    expect(source).toContain(
      'color: rgba(69, 89, 118, 0.82);',
    )
    expect(source).not.toContain(
      'linear-gradient(180deg, rgba(14, 28, 48, 0.58), rgba(20, 39, 67, 0.46))',
    )
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
