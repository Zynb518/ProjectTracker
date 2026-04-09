import { mount } from '@vue/test-utils'
import { describe, expect, it, vi } from 'vitest'

import ProjectFilters from '@/components/projects/ProjectFilters.vue'

describe('ProjectFilters', () => {
  it('renders the current status as a colored filter pill and submits on click', async () => {
    const wrapper = mount(ProjectFilters, {
      props: {
        keyword: '',
        status: '2',
      },
    })

    const activeButton = wrapper.get('[data-testid="projects-status-2"]')
    const doneButton = wrapper.get('[data-testid="projects-status-3"]')

    expect(activeButton.classes()).toContain('project-filters__status-pill--active')
    expect(activeButton.classes()).toContain('project-filters__status-pill--selected')
    expect(doneButton.classes()).toContain('project-filters__status-pill--done')
    expect(doneButton.classes()).not.toContain('project-filters__status-pill--selected')

    await doneButton.trigger('click')

    expect(wrapper.emitted('update:status')).toEqual([['3']])
    expect(wrapper.emitted('submit')).toEqual([[]])
  })

  it('emits the create trigger origin from the new project button', async () => {
    const wrapper = mount(ProjectFilters, {
      props: {
        keyword: '',
        status: '',
      },
    })

    const button = wrapper.get('[data-testid="create-project"]')
    vi.spyOn(button.element, 'getBoundingClientRect').mockReturnValue({
      x: 24,
      y: 36,
      width: 44,
      height: 44,
      top: 36,
      right: 68,
      bottom: 80,
      left: 24,
      toJSON: () => ({}),
    } as DOMRect)

    expect(button.attributes('aria-label')).toBe('新建项目')
    expect(button.text()).toBe('')

    await button.trigger('click')

    expect(wrapper.emitted('create')).toEqual([
      [
        {
          x: 46,
          y: 58,
        },
      ],
    ])
  })
})
