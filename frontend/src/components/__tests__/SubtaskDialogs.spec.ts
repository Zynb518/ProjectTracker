import { mount } from '@vue/test-utils'
import { readFileSync } from 'node:fs'
import { resolve } from 'node:path'
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
    expect(slider.attributes('min')).toBe('0')
    expect((slider.element as HTMLInputElement).value).toBe('40')
    expect(slider.attributes('step')).toBe('10')
    expect(wrapper.text()).toContain('当前 40%')
    expect(wrapper.text()).toContain('0%')
    expect(wrapper.text()).toContain('100%')

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

  it('uses darker shared surface tokens for the progress dialog and history drawer', () => {
    const progressSource = readFileSync(
      resolve(process.cwd(), 'src/components/subtasks/SubtaskProgressDialog.vue'),
      'utf8',
    )
    const historySource = readFileSync(
      resolve(process.cwd(), 'src/components/subtasks/SubtaskHistoryDrawer.vue'),
      'utf8',
    )

    expect(progressSource).toContain('background: var(--dialog-surface-glow), var(--dialog-surface-bg);')
    expect(progressSource).toContain('background: var(--dialog-control-bg);')
    expect(historySource).toContain('background: var(--dialog-surface-glow), var(--drawer-surface-bg);')
    expect(historySource).toContain('background: var(--drawer-item-bg);')
    expect(historySource).toContain('background: var(--dialog-control-bg-strong);')
  })

  it('uses distinct locked and remaining track colors around the progress thumb', () => {
    const progressSource = readFileSync(
      resolve(process.cwd(), 'src/components/subtasks/SubtaskProgressDialog.vue'),
      'utf8',
    )

    expect(progressSource).toContain('--subtask-progress-locked-color')
    expect(progressSource).toContain('--subtask-progress-remaining-color')
    expect(progressSource).toContain('var(--subtask-progress-locked-color) 0%')
    expect(progressSource).toContain('var(--subtask-progress-remaining-color) 100%')
    expect(progressSource).toContain("input[type='range']::-webkit-slider-runnable-track")
    expect(progressSource).toContain("input[type='range']::-moz-range-track")
  })

  it('vertically centers the progress thumb on the track', () => {
    const progressSource = readFileSync(
      resolve(process.cwd(), 'src/components/subtasks/SubtaskProgressDialog.vue'),
      'utf8',
    )

    expect(progressSource).toContain("input[type='range'] {\n  --subtask-progress-current: 0%;")
    expect(progressSource).toContain('height: 18px;')
    expect(progressSource).toContain("input[type='range']::-webkit-slider-thumb")
    expect(progressSource).toContain('margin-top: -5px;')
  })
})
