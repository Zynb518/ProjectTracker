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

  it('applies a login-specific class to the theme toggle so the toolbar control can visually match the cinematic card', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/views/LoginView.vue'), 'utf8')

    expect(source).toContain('<ThemeToggle class="login-view__theme-toggle" />')
    expect(source).toContain('.login-view .login-view__theme-toggle {')
    expect(source).toContain('.login-view .login-view__theme-toggle .theme-toggle__label {')
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

  it('styles the login view as a cinematic dawn sky with a horizon glow and no svg comet asset', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/views/LoginView.vue'), 'utf8')

    expect(source).toContain('background-color: #091321;')
    expect(source).toContain(
      'radial-gradient(circle at 9% 12%, rgba(255, 255, 255, 0.96) 0 1.4px, transparent 2.2px)',
    )
    expect(source).toContain(
      'radial-gradient(circle at 78% 18%, rgba(191, 230, 255, 0.82) 0 1.2px, transparent 2px)',
    )
    expect(source).toContain(
      'radial-gradient(circle at 34% 24%, rgba(228, 239, 255, 0.66) 0 0.95px, transparent 1.8px)',
    )
    expect(source).toContain(
      'radial-gradient(circle at 83% 24%, rgba(214, 233, 255, 0.64) 0 0.95px, transparent 1.75px)',
    )
    expect(source).toContain(
      'radial-gradient(ellipse 18px 1px at 9% 12%, rgba(255, 255, 255, 0.44), transparent 72%)',
    )
    expect(source).toContain(
      'radial-gradient(ellipse 1px 18px at 88% 11%, rgba(220, 238, 255, 0.38), transparent 72%)',
    )
    expect(source).not.toContain("url('../assets/login/comet-light.svg')")
    expect(source).not.toContain('radial-gradient(ellipse 520px 980px at 87% -12%')
    expect(source).not.toContain('radial-gradient(ellipse 210px 620px at 58% -2%')
    expect(source).not.toContain('linear-gradient(122deg, rgba(255, 255, 255, 0) 43%')
    expect(source).not.toContain('linear-gradient(118deg, rgba(255, 255, 255, 0) 56%')
    expect(source).not.toContain('linear-gradient(127deg, rgba(132, 228, 255, 0) 24%')
    expect(source).not.toContain('radial-gradient(circle at 61% 42%, rgba(160, 228, 255, 0.16), transparent 7.5%)')
    expect(source).toContain(
      'linear-gradient(180deg, rgba(10, 19, 39, 0.96) 0%, rgba(28, 50, 86, 0.9) 32%, rgba(87, 120, 160, 0.66) 68%, rgba(232, 242, 255, 0.3) 100%)',
    )
    expect(source).toContain(
      'radial-gradient(circle at 50% 100%, rgba(255, 212, 163, 0.62), transparent 40%)',
    )
    expect(source).toContain('0 30px 60px rgba(7, 17, 34, 0.24)')
    expect(source).toContain('inset 0 1px 0 rgba(255, 255, 255, 0.58)')
  })

  it('keeps the light login visuals cheaper to paint by capping background layers and avoiding oversized blur filters', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/views/LoginView.vue'), 'utf8')

    const lightBackgroundBlock = source.match(
      /html\.light \.login-view__background,[\s\S]*?background-image:([\s\S]*?)background-repeat:/,
    )?.[1]

    expect(lightBackgroundBlock).toBeTruthy()
    expect((lightBackgroundBlock?.match(/radial-gradient\(/g) ?? []).length).toBeLessThanOrEqual(14)
    expect(source).not.toContain('backdrop-filter: blur(18px) saturate(1.15);')
    expect(source).not.toContain('filter: blur(20px);')
    expect(source).not.toContain('comet-light.svg')
  })

  it('keeps the light login card translucent instead of covering the sky with an opaque white slab', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/views/LoginView.vue'), 'utf8')

    expect(source).toContain(
      'linear-gradient(180deg, rgba(12, 24, 43, 0.34), rgba(16, 34, 60, 0.22))',
    )
    expect(source).not.toContain(
      'linear-gradient(180deg, rgba(235, 245, 255, 0.76), rgba(214, 229, 247, 0.64))',
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
