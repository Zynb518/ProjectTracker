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
  it('renders semantic status filter pills and emits updates when a status pill is selected', async () => {
    const wrapper = mount(MyTaskFilters, {
      props: {
        projectId: '',
        status: '',
      },
    })

    await wrapper.get('[data-testid="my-task-filter-status-2"]').trigger('click')

    expect(wrapper.emitted('update:status')).toEqual([['2']])
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

  it('styles the refresh button as a polished call-to-action instead of a plain flat control', () => {
    const source = readFileSync(
      resolve(process.cwd(), 'src/components/my-tasks/MyTaskFilters.vue'),
      'utf8',
    )

    expect(source).toContain('class="my-task-filters__submit"')
    expect(source).toContain('<svg viewBox="0 0 24 24"')
    expect(source).toContain('display: inline-flex;')
    expect(source).toContain('justify-content: center;')
    expect(source).toContain('background: linear-gradient(135deg, color-mix(in srgb, var(--accent-start) 88%, #0d2754 12%), color-mix(in srgb, var(--accent-end) 78%, #0f2c5b 22%));')
    expect(source).toContain('inset 0 1px 0 color-mix(in srgb, #ffffff 18%, transparent),')
    expect(source).toContain('.my-task-filters__submit:hover {')
    expect(source).toContain('transform: translateY(-2px);')
  })
})
