import { createPinia, setActivePinia } from 'pinia'
import { render, waitFor } from '@testing-library/vue'
import userEvent from '@testing-library/user-event'
import { beforeEach, describe, expect, it, vi } from 'vitest'

vi.mock('@/api/subtasks', async () => {
  const actual = await vi.importActual<typeof import('@/api/subtasks')>('@/api/subtasks')

  return {
    ...actual,
    listMySubtasks: vi.fn(),
  }
})

import { listMySubtasks } from '@/api/subtasks'
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
  })

  it('loads my subtasks and re-queries when the status filter changes', async () => {
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
        project_id: undefined,
        status: undefined,
      })
    })

    expect(await screen.findByText('完成登录接口开发')).toBeTruthy()
    expect(screen.getByText('内部进度平台')).toBeTruthy()

    await user.selectOptions(screen.getByTestId('my-tasks-status'), '2')
    await user.click(screen.getByRole('button', { name: '刷新任务' }))

    await waitFor(() => {
      expect(listMySubtasks).toHaveBeenLastCalledWith({
        project_id: undefined,
        status: 2,
      })
    })
  })

  it('keeps the current task cards visible while refreshing tasks', async () => {
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

    await user.click(screen.getByRole('button', { name: '刷新任务' }))

    await waitFor(() => {
      expect(listMySubtasks).toHaveBeenLastCalledWith({
        project_id: undefined,
        status: undefined,
      })
    })

    expect(screen.getByText('完成登录接口开发')).toBeTruthy()
    expect(screen.getByText('任务列表刷新中...')).toBeTruthy()
  })
})
