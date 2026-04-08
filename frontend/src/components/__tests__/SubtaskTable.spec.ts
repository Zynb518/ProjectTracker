import { mount } from '@vue/test-utils'

import SubtaskTable from '@/components/subtasks/SubtaskTable.vue'

describe('SubtaskTable', () => {
  it('emits operational actions for a rendered subtask row', async () => {
    const wrapper = mount(SubtaskTable, {
      props: {
        canManage: true,
        selectedNode: {
          id: 2001,
          project_id: 1001,
          name: '需求分析',
          description: '完成需求梳理',
          sequence_no: 1,
          status: 2,
          planned_start_date: '2026-03-20',
          planned_end_date: '2026-03-25',
          completed_at: null,
          created_by: 1,
          created_at: '2026-03-19T11:20:00+08:00',
          updated_at: '2026-03-25T18:00:00+08:00',
          sub_task_count: 1,
          completed_sub_task_count: 0,
        },
        subtasks: [
          {
            id: 3001,
            node_id: 2001,
            name: '完成登录接口开发',
            description: '完成账号密码登录功能',
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
        ],
      },
    })

    await wrapper.get('[data-testid="subtask-progress-3001"]').trigger('click')
    await wrapper.get('[data-testid="subtask-history-3001"]').trigger('click')

    expect(wrapper.emitted('progress')).toEqual([[3001]])
    expect(wrapper.emitted('history')).toEqual([[3001]])
  })
})
