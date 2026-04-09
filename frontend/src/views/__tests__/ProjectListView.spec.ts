import { createPinia, setActivePinia } from 'pinia'
import { render, waitFor } from '@testing-library/vue'
import userEvent from '@testing-library/user-event'
import { beforeEach, describe, expect, it, vi } from 'vitest'

vi.mock('vue-router', async () => {
  const actual = await vi.importActual<typeof import('vue-router')>('vue-router')

  return {
    ...actual,
    useRouter: () => ({
      push: vi.fn(),
    }),
  }
})

vi.mock('@/api/projects', () => ({
  listProjects: vi.fn(),
  createProject: vi.fn(),
  updateProject: vi.fn(),
  deleteProject: vi.fn(),
  startProject: vi.fn(),
  completeProject: vi.fn(),
  reopenProject: vi.fn(),
}))

import { completeProject, listProjects } from '@/api/projects'
import { useNotificationStore } from '@/stores/notifications'
import ProjectListView from '@/views/ProjectListView.vue'

describe('ProjectListView', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
    vi.resetAllMocks()
    vi.mocked(listProjects).mockResolvedValue({
      list: [
        {
          id: 1001,
          name: '内部进度平台',
          description: '用于内部项目进度跟踪',
          owner_user_id: 1,
          owner_real_name: '张三',
          status: 2,
          planned_start_date: '2026-03-20',
          planned_end_date: '2026-06-30',
          completed_at: null,
          member_count: 5,
          node_count: 3,
          sub_task_count: 12,
          created_at: '2026-03-19T11:00:00+08:00',
          updated_at: '2026-03-27T09:30:00+08:00',
          is_owner: true,
        },
      ],
      total: 1,
      page: 1,
      page_size: 9,
    })
  })

  it('does not render a dedicated filter button', async () => {
    const screen = render(ProjectListView)

    await screen.findByText('内部进度平台')

    expect(screen.queryByRole('button', { name: '筛选项目' })).toBeNull()
  })

  it('re-queries when pressing Enter in the keyword field', async () => {
    const screen = render(ProjectListView)
    const user = userEvent.setup()

    await waitFor(() => {
      expect(listProjects).toHaveBeenCalledWith({
        keyword: '',
        status: undefined,
        page: 1,
        page_size: 9,
      })
    })

    expect(await screen.findByText('内部进度平台')).toBeTruthy()

    await user.type(screen.getByPlaceholderText('按项目名称搜索'), '平台{Enter}')

    await waitFor(() => {
      expect(listProjects).toHaveBeenLastCalledWith({
        keyword: '平台',
        status: undefined,
        page: 1,
        page_size: 9,
      })
    })
  })

  it('re-queries immediately when the status filter changes', async () => {
    const screen = render(ProjectListView)
    const user = userEvent.setup()

    await waitFor(() => {
      expect(listProjects).toHaveBeenCalledWith({
        keyword: '',
        status: undefined,
        page: 1,
        page_size: 9,
      })
    })

    expect(await screen.findByText('内部进度平台')).toBeTruthy()

    await user.click(screen.getByTestId('projects-status-2'))

    await waitFor(() => {
      expect(listProjects).toHaveBeenLastCalledWith({
        keyword: '',
        status: 2,
        page: 1,
        page_size: 9,
      })
    })
  })

  it('uses a page size of nine projects and allows paging forward', async () => {
    vi.mocked(listProjects)
      .mockResolvedValueOnce({
        list: [
          {
            id: 1001,
            name: '内部进度平台',
            description: '用于内部项目进度跟踪',
            owner_user_id: 1,
            owner_real_name: '张三',
            status: 2,
            planned_start_date: '2026-03-20',
            planned_end_date: '2026-06-30',
            completed_at: null,
            member_count: 5,
            node_count: 3,
            sub_task_count: 12,
            created_at: '2026-03-19T11:00:00+08:00',
            updated_at: '2026-03-27T09:30:00+08:00',
            is_owner: true,
          },
        ],
        total: 18,
        page: 1,
        page_size: 9,
      })
      .mockResolvedValueOnce({
        list: [
          {
            id: 1002,
            name: '第二页项目',
            description: '用于验证分页切换',
            owner_user_id: 2,
            owner_real_name: '李四',
            status: 1,
            planned_start_date: '2026-04-01',
            planned_end_date: '2026-07-01',
            completed_at: null,
            member_count: 4,
            node_count: 2,
            sub_task_count: 7,
            created_at: '2026-03-21T11:00:00+08:00',
            updated_at: '2026-03-28T09:30:00+08:00',
            is_owner: true,
          },
        ],
        total: 18,
        page: 2,
        page_size: 9,
      })

    const screen = render(ProjectListView)
    const user = userEvent.setup()

    await waitFor(() => {
      expect(listProjects).toHaveBeenCalledWith({
        keyword: '',
        status: undefined,
        page: 1,
        page_size: 9,
      })
    })

    expect(await screen.findByText('内部进度平台')).toBeTruthy()
    expect(screen.getByText('第 1 / 2 页')).toBeTruthy()

    await user.click(screen.getByRole('button', { name: '下一页' }))

    await waitFor(() => {
      expect(listProjects).toHaveBeenLastCalledWith({
        keyword: '',
        status: undefined,
        page: 2,
        page_size: 9,
      })
    })

    expect(await screen.findByText('第二页项目')).toBeTruthy()
    expect(screen.getByText('第 2 / 2 页')).toBeTruthy()
  })

  it('supports jumping directly to the first and last page', async () => {
    vi.mocked(listProjects)
      .mockResolvedValueOnce({
        list: [
          {
            id: 1001,
            name: '第一页项目',
            description: '用于验证首页和尾页跳转',
            owner_user_id: 1,
            owner_real_name: '张三',
            status: 2,
            planned_start_date: '2026-03-20',
            planned_end_date: '2026-06-30',
            completed_at: null,
            member_count: 5,
            node_count: 3,
            sub_task_count: 12,
            created_at: '2026-03-19T11:00:00+08:00',
            updated_at: '2026-03-27T09:30:00+08:00',
            is_owner: true,
          },
        ],
        total: 27,
        page: 1,
        page_size: 9,
      })
      .mockResolvedValueOnce({
        list: [
          {
            id: 1003,
            name: '尾页项目',
            description: '用于验证尾页跳转',
            owner_user_id: 3,
            owner_real_name: '王五',
            status: 3,
            planned_start_date: '2026-05-01',
            planned_end_date: '2026-08-01',
            completed_at: '2026-07-30T10:00:00+08:00',
            member_count: 6,
            node_count: 4,
            sub_task_count: 14,
            created_at: '2026-04-01T11:00:00+08:00',
            updated_at: '2026-04-28T09:30:00+08:00',
            is_owner: true,
          },
        ],
        total: 27,
        page: 3,
        page_size: 9,
      })
      .mockResolvedValueOnce({
        list: [
          {
            id: 1001,
            name: '第一页项目',
            description: '用于验证首页和尾页跳转',
            owner_user_id: 1,
            owner_real_name: '张三',
            status: 2,
            planned_start_date: '2026-03-20',
            planned_end_date: '2026-06-30',
            completed_at: null,
            member_count: 5,
            node_count: 3,
            sub_task_count: 12,
            created_at: '2026-03-19T11:00:00+08:00',
            updated_at: '2026-03-27T09:30:00+08:00',
            is_owner: true,
          },
        ],
        total: 27,
        page: 1,
        page_size: 9,
      })

    const screen = render(ProjectListView)
    const user = userEvent.setup()

    expect(await screen.findByText('第一页项目')).toBeTruthy()
    expect(screen.getByText('第 1 / 3 页')).toBeTruthy()

    await user.click(screen.getByRole('button', { name: '尾页' }))

    await waitFor(() => {
      expect(listProjects).toHaveBeenLastCalledWith({
        keyword: '',
        status: undefined,
        page: 3,
        page_size: 9,
      })
    })

    expect(await screen.findByText('尾页项目')).toBeTruthy()
    expect(screen.getByText('第 3 / 3 页')).toBeTruthy()

    await user.click(screen.getByRole('button', { name: '首页' }))

    await waitFor(() => {
      expect(listProjects).toHaveBeenLastCalledWith({
        keyword: '',
        status: undefined,
        page: 1,
        page_size: 9,
      })
    })

    expect(await screen.findByText('第一页项目')).toBeTruthy()
    expect(screen.getByText('第 1 / 3 页')).toBeTruthy()
  })

  it('keeps the current project cards visible while the next page is loading', async () => {
    vi.mocked(listProjects)
      .mockResolvedValueOnce({
        list: [
          {
            id: 1001,
            name: '内部进度平台',
            description: '用于内部项目进度跟踪',
            owner_user_id: 1,
            owner_real_name: '张三',
            status: 2,
            planned_start_date: '2026-03-20',
            planned_end_date: '2026-06-30',
            completed_at: null,
            member_count: 5,
            node_count: 3,
            sub_task_count: 12,
            created_at: '2026-03-19T11:00:00+08:00',
            updated_at: '2026-03-27T09:30:00+08:00',
            is_owner: true,
          },
        ],
        total: 18,
        page: 1,
        page_size: 9,
      })
      .mockImplementationOnce(
        () =>
          new Promise(() => {
            return undefined
          }),
      )

    const screen = render(ProjectListView)
    const user = userEvent.setup()

    expect(await screen.findByText('内部进度平台')).toBeTruthy()

    await user.click(screen.getByRole('button', { name: '下一页' }))

    await waitFor(() => {
      expect(listProjects).toHaveBeenLastCalledWith({
        keyword: '',
        status: undefined,
        page: 2,
        page_size: 9,
      })
    })

    expect(screen.getByText('内部进度平台')).toBeTruthy()
    expect(screen.getByText('项目列表刷新中...')).toBeTruthy()
  })

  it('opens the create project dialog', async () => {
    const screen = render(ProjectListView)
    const user = userEvent.setup()

    await user.click(screen.getByRole('button', { name: '新建项目' }))

    expect(screen.getByRole('heading', { name: '新建项目' })).toBeTruthy()
  })

  it('shows an error message when a project action fails', async () => {
    vi.mocked(completeProject).mockRejectedValue(new Error('项目完成失败：仍有未完成节点'))

    const pinia = createPinia()
    setActivePinia(pinia)
    const notificationStore = useNotificationStore(pinia)

    const screen = render(ProjectListView, {
      global: {
        plugins: [pinia],
      },
    })
    const user = userEvent.setup()

    await screen.findByText('内部进度平台')
    await user.click(screen.getByRole('button', { name: '完成' }))

    await waitFor(() => {
      expect(notificationStore.notices[0]?.message).toBe('项目完成失败：仍有未完成节点')
    })
    expect(screen.queryByText('项目完成失败：仍有未完成节点')).toBeNull()
  })

  it('renders long project content inside bounded card regions', async () => {
    const longName = '跨部门联合项目名称非常长用于验证项目卡片在窄空间内依然不会把状态标签和内容挤出边界'
    const longDescription =
      '这是一个特意写得很长的项目描述，用来验证项目卡片中的正文区域会在卡片内部正常换行，而不是把其他模块顶出卡片。'

    vi.mocked(listProjects).mockResolvedValue({
      list: [
        {
          id: 2001,
          name: longName,
          description: longDescription,
          owner_user_id: 1,
          owner_real_name: '产品交付协调负责人名称同样较长',
          status: 2,
          planned_start_date: '2026-03-20',
          planned_end_date: '2027-12-31',
          completed_at: null,
          member_count: 12,
          node_count: 8,
          sub_task_count: 37,
          created_at: '2026-03-19T11:00:00+08:00',
          updated_at: '2026-03-27T09:30:00+08:00',
          is_owner: true,
        },
      ],
      total: 1,
      page: 1,
      page_size: 9,
    })

    const screen = render(ProjectListView)

    const title = await screen.findByRole('heading', { name: longName })
    const description = screen.getByText(longDescription)
    const scheduleStart = screen.getByText('2026-03-20')
    const scheduleEnd = screen.getByText('2027-12-31')

    expect(title.closest('.project-card__title')).toBeTruthy()
    expect(title.getAttribute('title')).toBe(longName)
    expect(description.getAttribute('title')).toBe(longDescription)
    expect(scheduleStart.className).toContain('project-card__schedule-line')
    expect(scheduleEnd.className).toContain('project-card__schedule-line')
  })
})
