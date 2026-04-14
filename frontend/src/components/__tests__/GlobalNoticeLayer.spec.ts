import { nextTick } from 'vue'
import { createPinia, setActivePinia } from 'pinia'
import { mount } from '@vue/test-utils'
import { readFileSync } from 'node:fs'
import { resolve } from 'node:path'
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

  it('uses the shared galaxy meta surface for notices and removes the old hard-coded blur shell', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/GlobalNoticeLayer.vue'), 'utf8')

    expect(source).toContain('background: var(--meta-surface-bg), var(--meta-surface-glow), var(--card-sheen);')
    expect(source).toContain('box-shadow: var(--meta-surface-shadow);')
    expect(source).not.toContain('backdrop-filter: blur(16px);')
  })
})
