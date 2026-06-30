import { createPinia, setActivePinia } from 'pinia'
import { render } from '@testing-library/vue'
import userEvent from '@testing-library/user-event'
import { defineComponent } from 'vue'
import { beforeEach, describe, expect, it, vi } from 'vitest'

const pushMock = vi.fn().mockResolvedValue(undefined)

vi.mock('vue-router', async () => {
  const actual = await vi.importActual<typeof import('vue-router')>('vue-router')
  return {
    ...actual,
    useRouter: () => ({
      push: pushMock,
    }),
  }
})

import BeginnerTutorialOverlay from '@/components/tutorial/BeginnerTutorialOverlay.vue'
import { useBeginnerTutorialStore } from '@/stores/beginnerTutorial'

const TutorialHost = defineComponent({
  components: {
    BeginnerTutorialOverlay,
  },
  template: `
    <div>
      <BeginnerTutorialOverlay />
    </div>
  `,
})

describe('BeginnerTutorialOverlay', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
    pushMock.mockClear()
    document.documentElement.style.overflow = ''
    document.body.style.overflow = ''
  })

  it('starts the tutorial and goes to manual form step', async () => {
    const pinia = createPinia()
    setActivePinia(pinia)

    const tutorialStore = useBeginnerTutorialStore(pinia)
    tutorialStore.active = true
    tutorialStore.step = 'intro'

    const screen = render(TutorialHost, {
      global: {
        plugins: [pinia],
        stubs: {
          Transition: false,
        },
      },
    })
    const user = userEvent.setup()

    expect(screen.getByRole('heading', { name: '创建第一个项目' })).toBeTruthy()

    await user.click(screen.getByRole('button', { name: '开始教程' }))

    expect(tutorialStore.step).toBe('manual-form')
    expect(tutorialStore.mode).toBe('manual')
    expect(tutorialStore.dialogRequest).toBe('manual')
  })
})
