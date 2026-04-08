import { nextTick } from 'vue'
import { createPinia, setActivePinia } from 'pinia'
import { mount } from '@vue/test-utils'
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest'

import GlobalNoticeLayer from '@/components/GlobalNoticeLayer.vue'
import { useNotificationStore } from '@/stores/notifications'

describe('GlobalNoticeLayer', () => {
  beforeEach(() => {
    vi.useFakeTimers()
    setActivePinia(createPinia())
  })

  afterEach(() => {
    vi.useRealTimers()
    document.body.innerHTML = ''
  })

  it('renders an error notice and removes it after the timeout', async () => {
    const pinia = createPinia()
    setActivePinia(pinia)
    const store = useNotificationStore(pinia)

    const wrapper = mount(GlobalNoticeLayer, {
      global: {
        plugins: [pinia],
      },
    })

    store.notifyError('后端返回了错误', 2400)
    await nextTick()

    expect(document.body.querySelector('[role="alert"]')?.textContent).toContain('后端返回了错误')

    vi.advanceTimersByTime(2400)
    await nextTick()
    await nextTick()

    expect(document.body.querySelector('[role="alert"]')).toBeNull()

    wrapper.unmount()
  })
})
