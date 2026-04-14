import { createPinia, setActivePinia } from 'pinia'
import { fireEvent, render, waitFor } from '@testing-library/vue'
import userEvent from '@testing-library/user-event'
import { readFileSync } from 'node:fs'
import { resolve } from 'node:path'
import { beforeEach, describe, expect, it, vi } from 'vitest'

vi.mock('@/api/subtasks', async () => {
  const actual = await vi.importActual<typeof import('@/api/subtasks')>('@/api/subtasks')

  return {
    ...actual,
    listMySubtasks: vi.fn(),
    listSubtaskProgressRecords: vi.fn(),
    submitSubtaskProgress: vi.fn(),
  }
})

import { listMySubtasks, listSubtaskProgressRecords, submitSubtaskProgress } from '@/api/subtasks'
import MyTasksView from '@/views/MyTasksView.vue'

describe('MyTasksView', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
    vi.resetAllMocks()
    vi.mocked(listMySubtasks).mockResolvedValue({
      list: [
        {
          id: 3001,
          project_id: 1001,
          project_name: '内部进度平台',
          node_id: 2002,
          node_name: '开发实现',
          name: '完成登录接口开发',
          status: 2,
          progress_percent: 60,
          priority: 2,
          planned_start_date: '2026-03-27',
          planned_end_date: '2026-03-31',
          completed_at: null,
          updated_at: '2026-03-28T09:30:00+08:00',
        },
      ],
    })
    vi.mocked(listSubtaskProgressRecords).mockResolvedValue({
      list: [
        {
          id: 9001,
          sub_task_id: 3001,
          operator_user_id: 18,
          operator_real_name: '王五',
          progress_note: '字段联调已打通',
          progress_percent: 60,
          status: 2,
          created_at: '2026-03-28T09:30:00+08:00',
        },
      ],
      total: 1,
      page: 1,
      page_size: 20,
    })
    vi.mocked(submitSubtaskProgress).mockResolvedValue({
      subtask: {
        id: 3001,
        progress_percent: 80,
        updated_at: '2026-03-28T10:20:00+08:00',
      },
      progress_record: {
        id: 9002,
        sub_task_id: 3001,
        operator_user_id: 18,
        operator_real_name: '王五',
        progress_note: '推进到联调',
        progress_percent: 80,
        status: 2,
        created_at: '2026-03-28T10:20:00+08:00',
      },
    })
  })

  it('uses a shared galaxy hero shell without heavy backdrop blur in the task view top panel', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/views/MyTasksView.vue'), 'utf8')

    expect(source).toContain('.my-tasks-view__hero-shell {')
    expect(source).toContain('background: var(--gradient-surface), var(--project-card-glow);')
    expect(source).not.toContain('backdrop-filter: var(--backdrop-blur);')
  })

  it('merges the task hero and filters into one shared top panel', async () => {
    const screen = render(MyTasksView, {
      global: {
        plugins: [createPinia()],
      },
    })

    await screen.findByText('完成登录接口开发')

    const shell = screen.getByTestId('my-tasks-hero-shell')
    const filters = shell.querySelector('.my-task-filters')

    expect(shell.textContent).toContain('我的任务')
    expect(filters).not.toBeNull()
    expect(getComputedStyle(filters!).borderTopWidth).toBe('0px')
    expect(getComputedStyle(filters!).backgroundImage).toBe('none')
    expect(getComputedStyle(filters!).boxShadow).toBe('none')
  })

  it('loads my subtasks and immediately re-queries when the status filter changes', async () => {
    const pinia = createPinia()
    setActivePinia(pinia)

    const screen = render(MyTasksView, {
      global: {
        plugins: [pinia],
      },
    })
    const user = userEvent.setup()

    await waitFor(() => {
      expect(listMySubtasks).toHaveBeenCalledWith({
        project_keyword: undefined,
        status: undefined,
      })
    })

    expect(await screen.findByText('完成登录接口开发')).toBeTruthy()
    expect(screen.getByText('内部进度平台')).toBeTruthy()

    await user.click(screen.getByRole('button', { name: '进行中' }))

    await waitFor(() => {
      expect(listMySubtasks).toHaveBeenLastCalledWith({
        project_keyword: undefined,
        status: 2,
      })
    })
  })

  it('keeps the current task cards visible while refreshing tasks triggered from the project keyword combobox', async () => {
    vi.mocked(listMySubtasks)
      .mockResolvedValueOnce({
        list: [
          {
            id: 3001,
            project_id: 1001,
            project_name: '内部进度平台',
            node_id: 2002,
            node_name: '开发实现',
            name: '完成登录接口开发',
            status: 2,
            progress_percent: 60,
            priority: 2,
            planned_start_date: '2026-03-27',
            planned_end_date: '2026-03-31',
            completed_at: null,
            updated_at: '2026-03-28T09:30:00+08:00',
          },
        ],
      })
      .mockImplementationOnce(
        () =>
          new Promise(() => {
            return undefined
          }),
      )

    const screen = render(MyTasksView, {
      global: {
        plugins: [createPinia()],
      },
    })
    const user = userEvent.setup()

    expect(await screen.findByText('完成登录接口开发')).toBeTruthy()

    const projectKeywordInput = screen.getByPlaceholderText('输入项目名称后按回车，或直接选择候选')
    await user.clear(projectKeywordInput)
    await user.type(projectKeywordInput, '平台{enter}')

    await waitFor(() => {
      expect(listMySubtasks).toHaveBeenLastCalledWith({
        project_keyword: '平台',
        status: undefined,
      })
    })

    expect(screen.getByText('完成登录接口开发')).toBeTruthy()
    expect(screen.getByText('任务列表刷新中...')).toBeTruthy()
  })

  it('re-queries all tasks when the project keyword input is cleared and then blurred', async () => {
    const screen = render(MyTasksView, {
      global: {
        plugins: [createPinia()],
      },
    })
    const user = userEvent.setup()

    await screen.findByText('完成登录接口开发')

    const projectKeywordInput = screen.getByPlaceholderText('输入项目名称后按回车，或直接选择候选')
    await user.type(projectKeywordInput, '平台')
    await user.keyboard('{Enter}')

    await waitFor(() => {
      expect(listMySubtasks).toHaveBeenLastCalledWith({
        project_keyword: '平台',
        status: undefined,
      })
    })

    await user.clear(projectKeywordInput)
    await fireEvent.blur(projectKeywordInput)

    await waitFor(() => {
      expect(listMySubtasks).toHaveBeenLastCalledWith({
        project_keyword: undefined,
        status: undefined,
      })
    })
  })

  it('opens the task progress dialog from a task card and loads the latest 20 history records', async () => {
    const screen = render(MyTasksView, {
      global: {
        plugins: [createPinia()],
      },
    })
    const user = userEvent.setup()

    await screen.findByText('完成登录接口开发')

    await user.click(screen.getByTestId('my-task-card-3001'))

    await waitFor(() => {
      expect(listSubtaskProgressRecords).toHaveBeenCalledWith(3001, {
        page: 1,
        page_size: 20,
      })
    })

    expect(screen.getByText('最近 20 条提交')).toBeTruthy()
    expect(screen.getByText('字段联调已打通')).toBeTruthy()
  })

  it('submits progress from the my-task dialog and refreshes the task list', async () => {
    vi.mocked(listMySubtasks)
      .mockResolvedValueOnce({
        list: [
          {
            id: 3001,
            project_id: 1001,
            project_name: '内部进度平台',
            node_id: 2002,
            node_name: '开发实现',
            name: '完成登录接口开发',
            status: 2,
            progress_percent: 60,
            priority: 2,
            planned_start_date: '2026-03-27',
            planned_end_date: '2026-03-31',
            completed_at: null,
            updated_at: '2026-03-28T09:30:00+08:00',
          },
        ],
      })
      .mockResolvedValueOnce({
        list: [
          {
            id: 3001,
            project_id: 1001,
            project_name: '内部进度平台',
            node_id: 2002,
            node_name: '开发实现',
            name: '完成登录接口开发',
            status: 2,
            progress_percent: 80,
            priority: 2,
            planned_start_date: '2026-03-27',
            planned_end_date: '2026-03-31',
            completed_at: null,
            updated_at: '2026-03-28T10:20:00+08:00',
          },
        ],
      })

    const screen = render(MyTasksView, {
      global: {
        plugins: [createPinia()],
      },
    })
    const user = userEvent.setup()

    await screen.findByText('完成登录接口开发')
    await user.click(screen.getByTestId('my-task-card-3001'))

    const slider = await screen.findByTestId('my-task-progress-range')

    await fireEvent.update(slider, '80')
    await user.type(screen.getByLabelText('进展说明'), '推进到联调')
    await user.click(screen.getByRole('button', { name: '提交进度' }))

    await waitFor(() => {
      expect(submitSubtaskProgress).toHaveBeenCalledWith(3001, {
        progress_percent: 80,
        progress_note: '推进到联调',
      })
    })

    await waitFor(() => {
      expect(listMySubtasks).toHaveBeenLastCalledWith({
        project_keyword: undefined,
        status: undefined,
      })
    })

    await waitFor(() => {
      expect(screen.queryByText('最近 20 条提交')).toBeNull()
    })

    expect(screen.getAllByText('80%').length).toBeGreaterThan(0)
  })
})
