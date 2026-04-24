import { mount } from '@vue/test-utils'
import { readFileSync } from 'node:fs'
import { resolve } from 'node:path'
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

  it('uses shared semantic work-status tokens for filter pills and selected states without the brand gradient', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/projects/ProjectFilters.vue'), 'utf8')

    expect(source).toContain('border-color: var(--work-status-active-border);')
    expect(source).toContain('background: var(--work-status-active-bg);')
    expect(source).toContain('background: var(--work-status-active-strong);')
    expect(source).toContain('color: var(--work-status-done-color);')
    expect(source).not.toContain(
      ".project-filters__status-pill--active {\n  border-color: rgba(0, 194, 255, 0.22);\n  background: var(--gradient-primary-soft);",
    )
    expect(source).not.toContain(
      ".project-filters__status-pill--active.project-filters__status-pill--selected {\n  border-color: rgba(0, 194, 255, 0.28);\n  background: var(--gradient-primary);",
    )
  })

  it('keeps the project status filter container visually invisible so only the pills carry color', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/projects/ProjectFilters.vue'), 'utf8')
    const statusBarBlock = source.match(/\.project-filters__status-bar\s*\{([\s\S]*?)\n\}/)?.[0]

    expect(statusBarBlock).toBeTruthy()
    expect(statusBarBlock).toContain('padding: 0;')
    expect(statusBarBlock).toContain('border: 1px solid transparent;')
    expect(statusBarBlock).toContain('background: transparent;')
    expect(statusBarBlock).toContain('box-shadow: none;')
  })

  it('keeps the project status pills on a single row instead of wrapping', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/projects/ProjectFilters.vue'), 'utf8')
    const statusBarBlock = source.match(/\.project-filters__status-bar\s*\{([\s\S]*?)\n\}/)?.[0]
    const statusPillBlock = source.match(/\.project-filters__status-pill\s*\{([\s\S]*?)\n\}/)?.[0]

    expect(statusBarBlock).toBeTruthy()
    expect(statusBarBlock).toContain('flex-wrap: nowrap;')
    expect(statusBarBlock).toContain('overflow-x: auto;')
    expect(statusPillBlock).toBeTruthy()
    expect(statusPillBlock).toContain('flex: 0 0 auto;')
  })

  it('uses the shared galaxy meta surface for the filter shell and removes the old frosted blur wrapper', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/projects/ProjectFilters.vue'), 'utf8')

    expect(source).toContain('background: var(--meta-surface-bg), var(--meta-surface-glow), var(--card-sheen);')
    expect(source).toContain('box-shadow: var(--meta-surface-shadow);')
    expect(source).not.toContain('backdrop-filter: var(--backdrop-blur);')
  })

  it('uses a narrower first grid column so the keyword search takes less horizontal space than the status area', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/projects/ProjectFilters.vue'), 'utf8')
    const rootBlock = source.match(/\.project-filters\s*\{([\s\S]*?)\n\}/)?.[0]

    expect(rootBlock).toBeTruthy()
    expect(rootBlock).toContain('grid-template-columns: minmax(220px, 280px) minmax(0, 1fr) auto;')
    expect(rootBlock).not.toContain('grid-template-columns: repeat(2, minmax(0, 1fr)) auto;')
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

  it('emits the gantt trigger origin from the gantt overview button', async () => {
    const wrapper = mount(ProjectFilters, {
      props: {
        keyword: '',
        status: '',
      },
    })

    const button = wrapper.get('[data-testid="open-project-gantt"]')
    vi.spyOn(button.element, 'getBoundingClientRect').mockReturnValue({
      x: 100,
      y: 80,
      width: 120,
      height: 46,
      top: 80,
      right: 220,
      bottom: 126,
      left: 100,
      toJSON: () => ({}),
    } as DOMRect)

    expect(button.attributes('aria-label')).toBe('打开项目甘特图')
    expect(button.text()).toContain('项目甘特图')

    await button.trigger('click')

    expect(wrapper.emitted('open-gantt')).toEqual([
      [
        {
          x: 160,
          y: 103,
        },
      ],
    ])
  })
})
