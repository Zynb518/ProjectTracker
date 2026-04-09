import { mount } from '@vue/test-utils'
import { describe, expect, it } from 'vitest'

import SubtaskTable from '@/components/subtasks/SubtaskTable.vue'

describe('SubtaskTable', () => {
  it('renders an icon-only create button and emits create for manageable tables', async () => {
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
        subtasks: [],
      },
    })

    const createButton = wrapper.get('[data-testid="create-subtask"]')

    expect(createButton.attributes('aria-label')).toBe('新建子任务')
    expect(createButton.text()).toBe('')
    expect(wrapper.text()).not.toContain('需求分析')

    await createButton.trigger('click')

    expect(wrapper.emitted('create')).toEqual([[]])
  })

  it('shows edit as enabled below 100%, hides start, and keeps reopen disabled before completion', async () => {
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

    expect(wrapper.get('[data-testid="subtask-edit-3001"]').attributes('aria-label')).toBe('编辑')
    expect(wrapper.get('[data-testid="subtask-edit-3001"]').attributes('data-tooltip')).toBe('编辑')
    expect(wrapper.get('[data-testid="subtask-edit-3001"]').text()).toBe('')
    expect(wrapper.find('[data-testid="subtask-start-3001"]').exists()).toBe(false)
    expect(wrapper.get('[data-testid="subtask-edit-3001"]').attributes('disabled')).toBeUndefined()
    expect(wrapper.get('[data-testid="subtask-reopen-3001"]').attributes('aria-label')).toBe('撤销完成')
    expect(wrapper.get('[data-testid="subtask-reopen-3001"]').attributes('disabled')).toBe('')
    expect(wrapper.get('[data-testid="subtask-reopen-3001"]').text()).toBe('')
    expect(wrapper.get('[data-testid="subtask-remove-3001"]').attributes('aria-label')).toBe('删除')
    expect(wrapper.get('[data-testid="subtask-remove-3001"]').text()).toBe('')

    await wrapper.get('[data-testid="subtask-progress-3001"]').trigger('click')
    await wrapper.get('[data-testid="subtask-history-3001"]').trigger('click')
    await wrapper.get('[data-testid="subtask-edit-3001"]').trigger('click')
    await wrapper.get('[data-testid="subtask-remove-3001"]').trigger('click')

    expect(wrapper.emitted('progress')).toEqual([[3001]])
    expect(wrapper.emitted('history')).toEqual([[3001]])
    expect(wrapper.emitted('edit')).toEqual([[3001]])
    expect(wrapper.emitted('reopen')).toBeUndefined()
    expect(wrapper.emitted('remove')).toEqual([[3001]])
  })

  it('only enables reopen once subtask progress reaches 100%', async () => {
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
          completed_sub_task_count: 1,
        },
        subtasks: [
          {
            id: 3002,
            node_id: 2001,
            name: '联调登录态校验',
            description: '补齐状态持久化和回跳逻辑',
            responsible_user_id: 18,
            responsible_real_name: '王五',
            status: 3,
            progress_percent: 100,
            priority: 2,
            planned_start_date: '2026-03-27',
            planned_end_date: '2026-03-31',
            completed_at: '2026-03-31T18:00:00+08:00',
            updated_at: '2026-03-31T18:00:00+08:00',
          },
        ],
      },
    })

    expect(wrapper.get('[data-testid="subtask-edit-3002"]').attributes('disabled')).toBe('')
    expect(wrapper.get('[data-testid="subtask-reopen-3002"]').attributes('disabled')).toBeUndefined()
    expect(wrapper.find('[data-testid="subtask-start-3002"]').exists()).toBe(false)

    await wrapper.get('[data-testid="subtask-reopen-3002"]').trigger('click')

    expect(wrapper.emitted('edit')).toBeUndefined()
    expect(wrapper.emitted('reopen')).toEqual([[3002]])
  })
})
