import { mount } from '@vue/test-utils'
import { describe, expect, it } from 'vitest'

import NodeGanttDialog from '@/components/workspace/NodeGanttDialog.vue'
import ProjectGanttView from '@/components/workspace/ProjectGanttView.vue'
import type { ProjectNodeSubtaskGantt, ProjectStageGantt } from '@/types/gantt'

const sampleProjectGantt: ProjectStageGantt = {
  project: {
    id: 1001,
    name: '内部进度平台',
    owner_user_id: 1,
    owner_real_name: '张三',
    status: 2,
    planned_start_date: '2026-03-20',
    planned_end_date: '2026-06-30',
    completed_at: null,
  },
  nodes: [
    {
      id: 2001,
      name: '需求分析',
      sequence_no: 1,
      status: 3,
      planned_start_date: '2026-03-20',
      planned_end_date: '2026-03-25',
      completed_at: '2026-03-25T18:00:00+08:00',
    },
    {
      id: 2002,
      name: '前端开发',
      sequence_no: 2,
      status: 2,
      planned_start_date: '2026-03-26',
      planned_end_date: '2026-04-20',
      completed_at: null,
    },
  ],
}

const sampleNodeGantt: ProjectNodeSubtaskGantt = {
  project: sampleProjectGantt.project,
  node: sampleProjectGantt.nodes[1],
  subtasks: [
    {
      id: 3001,
      node_id: 2002,
      node_name: '前端开发',
      name: '完成登录接口开发',
      responsible_user_id: 18,
      responsible_real_name: '王五',
      status: 2,
      progress_percent: 60,
      priority: 2,
      planned_start_date: '2026-03-27',
      planned_end_date: '2026-03-31',
      completed_at: null,
    },
  ],
}

describe('ProjectGanttView', () => {
  it('renders fixed stage labels and a matching time axis', () => {
    const wrapper = mount(ProjectGanttView, {
      props: {
        gantt: sampleProjectGantt,
        scale: 'week',
      },
    })

    expect(wrapper.get('[data-testid="project-gantt-view"]').exists()).toBe(true)
    expect(wrapper.get('[data-testid="project-gantt-axis"]').findAll('.project-gantt__axis-cell').length).toBe(15)
    expect(wrapper.get('[data-testid="project-gantt-row-label-2001"]').text()).toContain('需求分析')
    expect(wrapper.get('[data-testid="project-gantt-row-label-2002"]').text()).toContain('前端开发')
  })

  it('maps stage bars to semantic status classes', () => {
    const wrapper = mount(ProjectGanttView, {
      props: {
        gantt: sampleProjectGantt,
        scale: 'week',
      },
    })

    expect(wrapper.get('[data-testid="project-gantt-stage-bar-2001"]').classes()).toContain('project-gantt__bar--done')
    expect(wrapper.get('[data-testid="project-gantt-stage-bar-2002"]').classes()).toContain('project-gantt__bar--active')
  })

  it('emits scale changes from the gantt toolbar switcher', async () => {
    const wrapper = mount(ProjectGanttView, {
      props: {
        gantt: sampleProjectGantt,
        scale: 'week',
      },
    })

    await wrapper.get('[data-testid="gantt-scale-day"]').trigger('click')

    expect(wrapper.emitted('update:scale')).toEqual([['day']])
  })

  it('emits the selected node id when a stage bar is clicked', async () => {
    const wrapper = mount(ProjectGanttView, {
      props: {
        gantt: sampleProjectGantt,
        scale: 'week',
      },
    })

    await wrapper.get('[data-testid="project-gantt-stage-bar-2002"]').trigger('click')

    expect(wrapper.emitted('open-node')).toEqual([[2002]])
  })
})

describe('NodeGanttDialog', () => {
  it('renders a floating subtask gantt timeline for the selected node', () => {
    const wrapper = mount(NodeGanttDialog, {
      props: {
        modelValue: true,
        gantt: sampleNodeGantt,
        scale: 'week',
      },
    })

    expect(wrapper.get('[data-testid="node-gantt-dialog"]').exists()).toBe(true)
    expect(wrapper.get('[data-testid="node-gantt-axis"]').findAll('.node-gantt-dialog__axis-cell').length).toBeGreaterThan(0)
    expect(wrapper.get('[data-testid="node-gantt-row-label-3001"]').text()).toContain('完成登录接口开发')
    expect(wrapper.get('[data-testid="node-gantt-bar-3001"]').exists()).toBe(true)
  })
})
