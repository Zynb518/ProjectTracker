import { createPinia, setActivePinia } from 'pinia'
import { mount } from '@vue/test-utils'
import { readFileSync } from 'node:fs'
import { resolve } from 'node:path'
import { beforeEach } from 'vitest'

import AppShell from '@/layouts/AppShell.vue'
import { useAuthStore } from '@/stores/auth'

describe('AppShell', () => {
  it('uses project card surfaces for the sidebar shell without reviving the old frosted treatment', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/layouts/AppShell.vue'), 'utf8')
    const sidebarBlock = source.match(/\.app-shell__sidebar \{[\s\S]*?\n\}/)?.[0] ?? ''

    expect(sidebarBlock).toContain('border: 1px solid var(--border-soft);')
    expect(sidebarBlock).toContain(
      'background: var(--project-card-bg), var(--project-card-glow), var(--card-sheen);',
    )
    expect(sidebarBlock).toContain('box-shadow: var(--shadow-panel);')
    expect(sidebarBlock).not.toContain('var(--meta-surface-bg)')
    expect(source).not.toContain('backdrop-filter: var(--backdrop-blur);')
  })

  it('keeps the light sky backdrop but removes the legacy light-only dark-glass overrides', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/layouts/AppShell.vue'), 'utf8')

    expect(source).toContain("url('../assets/login/sky.png')")
    expect(source).toContain('html.light .app-shell::before,')
    expect(source).toContain('position: fixed;')
    expect(source).toContain('background-size: cover;')
    expect(source).toContain('background-attachment: scroll;')
    expect(source).not.toContain('html.light .app-shell__sidebar,')
    expect(source).not.toContain('html:not(.dark) .app-shell__sidebar {')
    expect(source).not.toContain('html.light .app-shell__brand h1,')
    expect(source).not.toContain('html.light .app-shell__theme-toggle,')
    expect(source).not.toContain('color: #f2f7ff;')
  })

  beforeEach(() => {
    document.documentElement.className = 'light'
    localStorage.clear()
  })

  it('renders primary navigation links and the current user', () => {
    const pinia = createPinia()
    setActivePinia(pinia)

    const authStore = useAuthStore()
    authStore.currentUser = {
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

    const wrapper = mount(AppShell, {
      global: {
        plugins: [pinia],
        stubs: {
          RouterLink: {
            props: ['to'],
            template: '<a :href="to"><slot /></a>',
          },
          RouterView: {
            template: '<main data-testid="workspace-slot" />',
          },
        },
      },
    })

    expect(wrapper.text()).toContain('项目列表')
    expect(wrapper.text()).toContain('我的任务')
    expect(wrapper.text()).not.toContain('用户管理')
    expect(wrapper.get('[data-testid="sidebar-account"]').text()).toContain('张三')
    expect(wrapper.get('[data-testid="sidebar-account"]').text()).toContain('项目经理')
    expect(wrapper.get('[data-testid="sidebar-account"]').text()).toContain('退出登录')
    expect(wrapper.find('.app-shell__header').exists()).toBe(false)
    expect(wrapper.text()).not.toContain('Workspace Signal')
    expect(wrapper.find('[data-testid="workspace-slot"]').exists()).toBe(true)
  })

  it('renders the user management link for administrators only', () => {
    const pinia = createPinia()
    setActivePinia(pinia)

    const authStore = useAuthStore()
    authStore.currentUser = {
      id: 1,
      username: 'admin',
      real_name: '系统管理员',
      system_role: 1,
      email: 'admin@example.com',
      phone: '13800000000',
      status: 1,
      created_at: '2026-03-27T09:00:00+08:00',
      updated_at: '2026-03-27T09:00:00+08:00',
    }

    const wrapper = mount(AppShell, {
      global: {
        plugins: [pinia],
        stubs: {
          RouterLink: {
            props: ['to'],
            template: '<a :href="to"><slot /></a>',
          },
          RouterView: {
            template: '<main data-testid="workspace-slot" />',
          },
        },
      },
    })

    expect(wrapper.text()).toContain('用户管理')
  })

  it('toggles between light and dark themes and persists the selection', async () => {
    const pinia = createPinia()
    setActivePinia(pinia)

    const authStore = useAuthStore()
    authStore.currentUser = {
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

    const wrapper = mount(AppShell, {
      global: {
        plugins: [pinia],
        stubs: {
          RouterLink: {
            props: ['to'],
            template: '<a :href="to"><slot /></a>',
          },
          RouterView: {
            template: '<main data-testid="workspace-slot" />',
          },
        },
      },
    })

    const toggle = wrapper.get('[data-testid="theme-toggle"]')

    expect(document.documentElement.classList.contains('light')).toBe(true)

    await toggle.trigger('click')

    expect(document.documentElement.classList.contains('dark')).toBe(true)
    expect(localStorage.getItem('project-tracker-theme')).toBe('dark')
  })

  it('keeps the sidebar pinned on desktop screens while the workspace scrolls', () => {
    const pinia = createPinia()
    setActivePinia(pinia)

    const authStore = useAuthStore()
    authStore.currentUser = {
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

    const wrapper = mount(AppShell, {
      global: {
        plugins: [pinia],
        stubs: {
          RouterLink: {
            props: ['to'],
            template: '<a :href="to"><slot /></a>',
          },
          RouterView: {
            template: '<main data-testid="workspace-slot" />',
          },
        },
      },
    })

    const sidebar = wrapper.get('.app-shell__sidebar').element
    const styles = getComputedStyle(sidebar)

    expect(styles.position).toBe('sticky')
    expect(styles.top).toBe('20px')
    expect(styles.height).toBe('calc(100vh - 40px)')
  })
})
