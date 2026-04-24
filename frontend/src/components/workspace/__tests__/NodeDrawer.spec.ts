import { mount } from '@vue/test-utils'
import { describe, expect, it } from 'vitest'

import NodeDrawer from '@/components/workspace/NodeDrawer.vue'
import type { ProjectNode } from '@/types/node'

const sampleNode: ProjectNode = {
  id: 2002,
  project_id: 1001,
  name: '前端开发',
  description: '完成项目界面与交互实现',
  sequence_no: 2,
  status: 2,
  planned_start_date: '2026-03-26',
  planned_end_date: '2026-04-20',
  completed_at: null,
  created_by: 1,
  created_at: '2026-03-25T18:20:00+08:00',
  updated_at: '2026-03-28T10:00:00+08:00',
  sub_task_count: 6,
  completed_sub_task_count: 2,
}

describe('NodeDrawer', () => {
  it('stacks the schedule start and end dates on separate lines in the header metrics', () => {
    const wrapper = mount(NodeDrawer, {
      props: {
        canManage: true,
        isLoading: false,
        node: sampleNode,
        subtasks: [],
      },
      global: {
        stubs: {
          SubtaskTable: {
            template: '<div data-testid="subtask-table-stub" />',
          },
        },
      },
    })

    const scheduleLines = wrapper.findAll('.node-drawer__schedule-range span')

    expect(scheduleLines).toHaveLength(2)
    expect(scheduleLines.map((line) => line.text())).toEqual(['2026-03-26', '2026-04-20'])
  })
})
