import { createPinia, setActivePinia } from 'pinia'
import { mount } from '@vue/test-utils'
import { beforeEach } from 'vitest'

import AppShell from '@/layouts/AppShell.vue'
import { useAuthStore } from '@/stores/auth'

describe('AppShell', () => {
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
