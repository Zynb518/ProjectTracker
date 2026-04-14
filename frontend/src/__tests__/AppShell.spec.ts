import { createPinia, setActivePinia } from 'pinia'
import { mount } from '@vue/test-utils'
import { existsSync, readFileSync } from 'node:fs'
import { resolve } from 'node:path'

import App from '@/App.vue'
import { useNotificationStore } from '@/stores/notifications'

describe('App shell', () => {
  afterEach(() => {
    document.body.innerHTML = ''
  })

  it('renders a router outlet for the single-page application shell', () => {
    const pinia = createPinia()
    setActivePinia(pinia)

    const wrapper = mount(App, {
      global: {
        plugins: [pinia],
        stubs: {
          RouterView: {
            name: 'RouterView',
            template: '<main data-testid="router-outlet" />',
          },
        },
      },
    })

    expect(wrapper.find('[data-testid="router-outlet"]').exists()).toBe(true)
  })

  it('renders the global notice layer above the routed page', async () => {
    const pinia = createPinia()
    setActivePinia(pinia)
    const notificationStore = useNotificationStore(pinia)

    const wrapper = mount(App, {
      global: {
        plugins: [pinia],
        stubs: {
          RouterView: {
            name: 'RouterView',
            template: '<main data-testid="router-outlet" />',
          },
        },
      },
    })

    notificationStore.notifyError('项目操作失败')
    await wrapper.vm.$nextTick()

    expect(document.body.querySelector('[role="alert"]')?.textContent).toContain('项目操作失败')

    wrapper.unmount()
  })

  it('mounts a dedicated meteor overlay ahead of the routed content shell', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/App.vue'), 'utf8')

    expect(existsSync(resolve(process.cwd(), 'src/components/backgrounds/MeteorSkyOverlay.vue'))).toBe(true)
    expect(existsSync(resolve(process.cwd(), 'src/components/backgrounds/SkyBackdrop.vue'))).toBe(true)
    expect(source).toContain("import SkyBackdrop from '@/components/backgrounds/SkyBackdrop.vue'")
    expect(source).toContain('<SkyBackdrop />')
    expect(source).toContain("import MeteorSkyOverlay from '@/components/backgrounds/MeteorSkyOverlay.vue'")
    expect(source).toContain('<MeteorSkyOverlay />')
    expect(source).toContain('class="app-root__content"')
  })
})
