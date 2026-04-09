import { mount } from '@vue/test-utils'
import { describe, expect, it } from 'vitest'

import SubtaskHistoryDrawer from '@/components/subtasks/SubtaskHistoryDrawer.vue'
import SubtaskProgressDialog from '@/components/subtasks/SubtaskProgressDialog.vue'

describe('Subtask dialogs', () => {
  it('uses a slider-only progress editor with a non-decreasing floor and smooth entrance', async () => {
    const wrapper = mount(SubtaskProgressDialog, {
      props: {
        modelValue: true,
        subtask: {
          id: 3001,
          node_id: 2002,
          name: '实现项目卡片详情',
          description: '补齐卡片数据展示与布局',
          responsible_user_id: 18,
          responsible_real_name: '王五',
          status: 2,
          progress_percent: 40,
          priority: 2,
          planned_start_date: '2026-04-09',
          planned_end_date: '2026-04-12',
          completed_at: null,
          updated_at: '2026-04-08T22:00:00+08:00',
        },
      },
    })

    const backdrop = wrapper.get('.subtask-dialog__backdrop')
    const slider = wrapper.get('[data-testid="subtask-progress-range"]')

    expect(backdrop.classes()).toContain('subtask-dialog-fade-enter-from')
    expect(backdrop.classes()).toContain('subtask-dialog-fade-enter-active')
    expect(wrapper.find('select').exists()).toBe(false)
    expect(slider.attributes('min')).toBe('40')
    expect(slider.attributes('step')).toBe('10')

    await slider.setValue(20)
    await wrapper.get('textarea').setValue('继续推进')
    await wrapper.get('button.primary').trigger('click')

    expect(wrapper.emitted('submit')).toEqual([
      [
        {
          subtaskId: 3001,
          progress_percent: 40,
          progress_note: '继续推进',
        },
      ],
    ])
  })

  it('wraps the history drawer in an appear transition for smoother slide-in entrance', () => {
    const wrapper = mount(SubtaskHistoryDrawer, {
      props: {
        modelValue: true,
        title: '实现项目卡片详情',
        records: [
          {
            id: 9001,
            sub_task_id: 3001,
            operator_user_id: 1,
            operator_real_name: '张三',
            progress_note: '已完成基础排版',
            progress_percent: 40,
            status: 2,
            created_at: '2026-04-08T22:06:00+08:00',
          },
        ],
      },
    })

    const backdrop = wrapper.get('.history-drawer__backdrop')

    expect(backdrop.classes()).toContain('history-drawer-slide-enter-from')
    expect(backdrop.classes()).toContain('history-drawer-slide-enter-active')
  })
})
