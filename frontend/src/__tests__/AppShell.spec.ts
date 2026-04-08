import { createPinia, setActivePinia } from 'pinia'
import { mount } from '@vue/test-utils'
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
})
