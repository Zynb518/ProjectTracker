import { createPinia, setActivePinia } from 'pinia'
import { render, waitFor } from '@testing-library/vue'
import userEvent from '@testing-library/user-event'
import { defineComponent, ref } from 'vue'
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

import ProjectAiDraftDialog from '@/components/projects/ProjectAiDraftDialog.vue'
import BeginnerTutorialOverlay from '@/components/tutorial/BeginnerTutorialOverlay.vue'
import { useBeginnerTutorialStore } from '@/stores/beginnerTutorial'

const TutorialAiHost = defineComponent({
  components: {
    BeginnerTutorialOverlay,
    ProjectAiDraftDialog,
  },
  setup() {
    const open = ref(true)

    return {
      open,
    }
  },
  template: `
    <div>
      <ProjectAiDraftDialog v-model="open" :redirect-on-created="false" />
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

  it('advances from AI review to AI submit when clicking the tutorial next action', async () => {
    const pinia = createPinia()
    setActivePinia(pinia)

    const tutorialStore = useBeginnerTutorialStore(pinia)
    tutorialStore.active = true
    tutorialStore.mode = 'ai'
    tutorialStore.step = 'ai-review'

    const screen = render(TutorialAiHost, {
      global: {
        plugins: [pinia],
        stubs: {
          Transition: false,
        },
      },
    })
    const user = userEvent.setup()

    expect(await screen.findByRole('heading', { name: '先检查结构树和右侧属性编辑' })).toBeTruthy()

    await user.click(screen.getByRole('button', { name: '下一步' }))

    await waitFor(() => {
      expect(tutorialStore.step).toBe('ai-submit')
    })

    expect(screen.getByRole('heading', { name: '确认无误后提交整份草稿' })).toBeTruthy()
    expect(screen.getByRole('button', { name: '最终提交' })).toBeTruthy()
  })
})
