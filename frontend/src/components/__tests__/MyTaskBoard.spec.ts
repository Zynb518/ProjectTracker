import { mount } from '@vue/test-utils'
import { readFileSync } from 'node:fs'
import { resolve } from 'node:path'
import { describe, expect, it } from 'vitest'

import MyTaskBoard from '@/components/my-tasks/MyTaskBoard.vue'
import MyTaskFilters from '@/components/my-tasks/MyTaskFilters.vue'
import type { Subtask } from '@/types/subtask'

const sampleTasks: Subtask[] = [
  {
    id: 3001,
    project_id: 1001,
    project_name: '内部进度平台',
    node_id: 2002,
    node_name: '开发实现',
    name: '完成登录接口开发',
    description: '联调并完成页面接入',
    responsible_user_id: 18,
    responsible_real_name: '王五',
    status: 2,
    progress_percent: 60,
    priority: 2,
    planned_start_date: '2026-03-27',
    planned_end_date: '2026-03-31',
    completed_at: null,
    updated_at: '2026-03-28T09:30:00+08:00',
  },
]

describe('MyTaskBoard', () => {
  it('renders project, node and subtask names in a clear hierarchy with semantic priority text', () => {
    const wrapper = mount(MyTaskBoard, {
      props: {
        tasks: sampleTasks,
      },
    })

    expect(wrapper.get('.my-task-board__project').text()).toBe('内部进度平台')
    expect(wrapper.get('.my-task-board__node').text()).toBe('开发实现')
    expect(wrapper.get('.my-task-board__header h2').text()).toBe('完成登录接口开发')
    expect(wrapper.get('.my-task-board__priority').text()).toBe('中优先级')
    expect(wrapper.html()).not.toContain('<dd>2</dd>')
  })

  it('emits a progress-open event when the user clicks a task card', async () => {
    const wrapper = mount(MyTaskBoard, {
      props: {
        tasks: sampleTasks,
      },
    })

    await wrapper.get('[data-testid="my-task-card-3001"]').trigger('click')

    expect(wrapper.emitted('open-progress')).toEqual([[3001]])
  })

  it('uses layered task card surfaces, priority tones and hover elevation instead of flat neutral cards', () => {
    const source = readFileSync(
      resolve(process.cwd(), 'src/components/my-tasks/MyTaskBoard.vue'),
      'utf8',
    )

    expect(source).toContain(".my-task-board__card--active {")
    expect(source).toContain('radial-gradient(circle at top right,')
    expect(source).toContain('.my-task-board__card:hover {')
    expect(source).toContain('transform: translateY(-4px);')
    expect(source).toContain('.my-task-board__priority--medium {')
    expect(source).toContain('.my-task-board__progress-track {')
    expect(source).toContain('.my-task-board__progress-fill {')
    expect(source).not.toContain('backdrop-filter: var(--backdrop-blur);')
  })

  it('adds a luminous light-theme glass treatment with nested frosted surfaces instead of plain white cards', () => {
    const source = readFileSync(
      resolve(process.cwd(), 'src/components/my-tasks/MyTaskBoard.vue'),
      'utf8',
    )

    expect(source).toContain(':global(html.light) .my-task-board__card {')
    expect(source).toContain('linear-gradient(160deg,')
    expect(source).toContain('.my-task-board__card::after {')
    expect(source).toContain('--task-card-subsurface-bg: linear-gradient(')
    expect(source).toContain('background: var(--task-card-subsurface-bg);')
    expect(source).toContain('background: var(--task-card-pill-bg);')
  })

  it('keeps project and node names on a single truncated line so long labels do not stretch the card height', () => {
    const source = readFileSync(
      resolve(process.cwd(), 'src/components/my-tasks/MyTaskBoard.vue'),
      'utf8',
    )

    expect(source).toContain('.my-task-board__project {')
    expect(source).toContain('overflow: hidden;')
    expect(source).toContain('text-overflow: ellipsis;')
    expect(source).toContain('white-space: nowrap;')
    expect(source).toContain('.my-task-board__node {')
    expect(source).toContain('max-width: 100%;')
  })
})

describe('MyTaskFilters', () => {
  it('applies the typed project keyword on Enter without needing a separate refresh button', async () => {
    const wrapper = mount(MyTaskFilters, {
      props: {
        projectKeyword: '平台',
        projectCandidates: ['内部进度平台', '平台升级'],
        status: '',
      },
    })

    await wrapper.get('input').trigger('keydown.enter')

    expect(wrapper.emitted('update:projectKeyword')).toEqual([['平台']])
    expect(wrapper.emitted('apply-project-keyword')).toEqual([['平台']])
  })

  it('clears and applies an empty project keyword when the combobox loses focus after being emptied', async () => {
    const wrapper = mount(MyTaskFilters, {
      props: {
        projectKeyword: '内部进度平台',
        projectCandidates: ['内部进度平台'],
        status: '',
      },
    })

    const input = wrapper.get('input')
    await input.setValue('')
    await wrapper.setProps({ projectKeyword: '' })
    await input.trigger('blur')

    expect(wrapper.emitted('update:projectKeyword')).toEqual([[''], ['']])
    expect(wrapper.emitted('apply-project-keyword')).toEqual([['']])
  })

  it('applies a selected project candidate directly from the combobox dropdown', async () => {
    const wrapper = mount(MyTaskFilters, {
      props: {
        projectKeyword: '平台',
        projectCandidates: ['内部进度平台', '平台升级'],
        status: '',
      },
    })

    await wrapper.get('input').trigger('focus')
    await wrapper.get('[data-testid="my-task-project-candidate-内部进度平台"]').trigger('mousedown')

    expect(wrapper.emitted('update:projectKeyword')).toEqual([['内部进度平台']])
    expect(wrapper.emitted('apply-project-keyword')).toEqual([['内部进度平台']])
  })

  it('elevates the whole filter surface while the project candidate panel is open so the overlay is not covered by task cards', async () => {
    const wrapper = mount(MyTaskFilters, {
      props: {
        projectKeyword: '平台',
        projectCandidates: ['内部进度平台', '平台升级'],
        status: '',
      },
    })

    expect(wrapper.classes()).not.toContain('my-task-filters--candidate-open')

    await wrapper.get('input').trigger('focus')

    expect(wrapper.classes()).toContain('my-task-filters--candidate-open')

    await wrapper.get('input').trigger('blur')

    expect(wrapper.classes()).not.toContain('my-task-filters--candidate-open')
  })

  it('renders the dropdown as a compact matched project list without header copy or secondary metadata', async () => {
    const wrapper = mount(MyTaskFilters, {
      props: {
        projectKeyword: '平台',
        projectCandidates: ['内部进度平台', '平台升级'],
        status: '',
      },
    })

    await wrapper.get('input').trigger('focus')

    const panel = wrapper.get('.my-task-filters__candidate-panel')
    expect(panel.text()).toContain('内部进度平台')
    expect(panel.text()).toContain('平台升级')
    expect(panel.text()).not.toContain('项目候选')
    expect(panel.text()).not.toContain('关键词命中')
    expect(wrapper.find('.my-task-filters__candidate-panel-head').exists()).toBe(false)
    expect(wrapper.find('.my-task-filters__candidate-meta').exists()).toBe(false)
    expect(wrapper.find('.my-task-filters__candidate-arrow').exists()).toBe(false)
  })

  it('uses a wider project search column and lets matched rows span flush with the combobox width', () => {
    const source = readFileSync(
      resolve(process.cwd(), 'src/components/my-tasks/MyTaskFilters.vue'),
      'utf8',
    )

    expect(source).toContain('grid-template-columns: minmax(0, 420px) minmax(0, 1fr);')
    expect(source).toContain('.my-task-filters__candidate-panel {')
    expect(source).toContain('padding: 6px 0;')
    expect(source).toContain('overflow: hidden;')
  })

  it('uses galaxy meta surfaces for the filter shell and candidate panel instead of the old glass blur treatment', () => {
    const source = readFileSync(
      resolve(process.cwd(), 'src/components/my-tasks/MyTaskFilters.vue'),
      'utf8',
    )

    expect(source).toContain('background: var(--meta-surface-bg), var(--meta-surface-glow), var(--card-sheen);')
    expect(source).toContain('box-shadow: var(--meta-surface-shadow);')
    expect(source).not.toContain('backdrop-filter: var(--backdrop-blur);')
  })

  it('supports an embedded mode so the task filters can be rendered inside a shared top panel shell', () => {
    const wrapper = mount(MyTaskFilters, {
      props: {
        embedded: true,
        projectKeyword: '',
        projectCandidates: [],
        status: '',
      },
    })
    const source = readFileSync(
      resolve(process.cwd(), 'src/components/my-tasks/MyTaskFilters.vue'),
      'utf8',
    )

    expect(wrapper.classes()).toContain('my-task-filters--embedded')
    expect(source).toContain('embedded?: boolean')
    expect(source).toContain('.my-task-filters--embedded {')
    expect(source).toContain('padding: 0;')
    expect(source).toContain('border: 0 solid transparent;')
    expect(source).toContain('background: none;')
    expect(source).toContain('box-shadow: none;')
  })

  it('renders semantic status filter pills and emits updates when a status pill is selected', async () => {
    const wrapper = mount(MyTaskFilters, {
      props: {
        projectKeyword: '',
        projectCandidates: [],
        status: '',
      },
    })

    await wrapper.get('[data-testid="my-task-filter-status-2"]').trigger('click')

    expect(wrapper.emitted('update:status')).toEqual([['2']])
  })

  it('immediately emits submit after a status pill selection so task status filters apply without an extra refresh click', async () => {
    const wrapper = mount(MyTaskFilters, {
      props: {
        projectKeyword: '',
        projectCandidates: [],
        status: '',
      },
    })

    await wrapper.get('[data-testid="my-task-filter-status-3"]').trigger('click')

    expect(wrapper.emitted('update:status')).toEqual([['3']])
    expect(wrapper.emitted('submit')).toEqual([[]])
  })

  it('uses semantic color classes and active-state emphasis for task status pills while keeping the outer frame visually invisible', () => {
    const source = readFileSync(
      resolve(process.cwd(), 'src/components/my-tasks/MyTaskFilters.vue'),
      'utf8',
    )

    expect(source).toContain('.my-task-filters__status-group')
    expect(source).toContain('padding: 0;')
    expect(source).toContain('border: 1px solid transparent;')
    expect(source).toContain('background: transparent;')
    expect(source).toContain('box-shadow: none;')
    expect(source).toContain('.my-task-filters__status-pill--pending')
    expect(source).toContain('.my-task-filters__status-pill--active')
    expect(source).toContain('.my-task-filters__status-pill--done')
    expect(source).toContain('.my-task-filters__status-pill--delayed')
    expect(source).toContain('.my-task-filters__status-pill.is-active')
    expect(source).toContain('aria-pressed')
  })

  it('removes the old refresh button and wires the project keyword combobox to keyboard-first filtering', () => {
    const source = readFileSync(
      resolve(process.cwd(), 'src/components/my-tasks/MyTaskFilters.vue'),
      'utf8',
    )

    expect(source).toContain('@keydown.enter.prevent="handleProjectKeywordSubmit"')
    expect(source).toContain('@blur="handleProjectKeywordBlur"')
    expect(source).toContain('my-task-filters__candidate-panel')
    expect(source).toContain('role="combobox"')
    expect(source).not.toContain('class="my-task-filters__submit"')
    expect(source).not.toContain('刷新任务')
  })
})
