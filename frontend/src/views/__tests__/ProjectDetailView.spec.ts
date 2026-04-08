import { createPinia, setActivePinia } from 'pinia'
import { render, waitFor } from '@testing-library/vue'
import userEvent from '@testing-library/user-event'
import { describe, expect, it, vi } from 'vitest'

vi.mock('vue-router', async () => {
  const actual = await vi.importActual<typeof import('vue-router')>('vue-router')

  return {
    ...actual,
    useRoute: () => ({
      params: {
        projectId: '1001',
      },
    }),
    useRouter: () => ({
      push: vi.fn(),
    }),
  }
})

vi.mock('@/api/projects', () => ({
  getProjectDetail: vi.fn(),
}))

vi.mock('@/api/members', () => ({
  listProjectMembers: vi.fn(),
  listMemberCandidates: vi.fn(),
  addProjectMember: vi.fn(),
  removeProjectMember: vi.fn(),
}))

vi.mock('@/api/nodes', () => ({
  listProjectNodes: vi.fn(),
  createProjectNode: vi.fn(),
  updateProjectNode: vi.fn(),
  deleteProjectNode: vi.fn(),
  startProjectNode: vi.fn(),
  completeProjectNode: vi.fn(),
  reopenProjectNode: vi.fn(),
  reorderProjectNodes: vi.fn(),
}))

vi.mock('@/api/subtasks', () => ({
  listNodeSubtasks: vi.fn(),
  listMySubtasks: vi.fn(),
  createSubtask: vi.fn(),
  updateSubtask: vi.fn(),
  deleteSubtask: vi.fn(),
  startSubtask: vi.fn(),
  reopenSubtask: vi.fn(),
  submitSubtaskProgress: vi.fn(),
  listSubtaskProgressRecords: vi.fn(),
}))

import { getProjectDetail } from '@/api/projects'
import { listProjectMembers, removeProjectMember } from '@/api/members'
import { listProjectNodes } from '@/api/nodes'
import { listNodeSubtasks } from '@/api/subtasks'
import { useNotificationStore } from '@/stores/notifications'
import ProjectDetailView from '@/views/ProjectDetailView.vue'

describe('ProjectDetailView', () => {
  it('shows an error message when the workspace fails to load', async () => {
    vi.mocked(getProjectDetail).mockRejectedValue(new Error('项目详情加载失败'))
    vi.mocked(listProjectMembers).mockResolvedValue({ list: [] })
    vi.mocked(listProjectNodes).mockResolvedValue({ list: [] })

    const pinia = createPinia()
    setActivePinia(pinia)
    const notificationStore = useNotificationStore(pinia)

    const screen = render(ProjectDetailView, {
      global: {
        plugins: [pinia],
      },
    })

    await waitFor(() => {
      expect(notificationStore.notices[0]?.message).toBe('项目详情加载失败')
    })

    expect(screen.queryByText('项目详情加载失败')).toBeNull()
  })

  it('loads the project workspace summary, members, nodes, and selected node subtasks', async () => {
    vi.mocked(getProjectDetail).mockResolvedValue({
      id: 1001,
      name: '内部进度平台',
      description: '用于内部项目进度跟踪',
      owner_user_id: 1,
      owner_real_name: '张三',
      status: 2,
      planned_start_date: '2026-03-20',
      planned_end_date: '2026-06-30',
      completed_at: null,
      created_by: 1,
      created_by_real_name: '张三',
      created_at: '2026-03-19T11:00:00+08:00',
      updated_at: '2026-03-27T09:30:00+08:00',
      member_count: 5,
      node_count: 3,
      completed_node_count: 1,
      sub_task_count: 12,
      completed_sub_task_count: 4,
      permissions: {
        can_edit_basic: true,
        can_manage_members: true,
        can_manage_nodes: true,
        can_transfer_owner: true,
        can_delete: true,
      },
    })

    vi.mocked(listProjectMembers).mockResolvedValue({
      list: [
        {
          project_id: 1001,
          user_id: 1,
          username: 'zhangsan',
          real_name: '张三',
          system_role: 2,
          status: 1,
          joined_at: '2026-03-19T11:00:00+08:00',
          is_owner: true,
        },
      ],
    })

    vi.mocked(listProjectNodes).mockResolvedValue({
      list: [
        {
          id: 2001,
          project_id: 1001,
          name: '需求分析',
          description: '完成需求梳理',
          sequence_no: 1,
          status: 3,
          planned_start_date: '2026-03-20',
          planned_end_date: '2026-03-25',
          completed_at: '2026-03-25T18:00:00+08:00',
          created_by: 1,
          created_at: '2026-03-19T11:20:00+08:00',
          updated_at: '2026-03-25T18:00:00+08:00',
          sub_task_count: 4,
          completed_sub_task_count: 4,
        },
      ],
    })

    vi.mocked(listNodeSubtasks).mockResolvedValue({
      list: [
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
          created_by: 1,
          created_at: '2026-03-27T15:00:00+08:00',
          updated_at: '2026-03-28T09:30:00+08:00',
        },
      ],
    })

    const screen = render(ProjectDetailView, {
      global: {
        plugins: [createPinia()],
      },
    })

    await waitFor(() => {
      expect(getProjectDetail).toHaveBeenCalledWith(1001)
      expect(listProjectMembers).toHaveBeenCalledWith(1001)
      expect(listProjectNodes).toHaveBeenCalledWith(1001)
      expect(listNodeSubtasks).toHaveBeenCalledWith(1001, 2001, {
        responsible_user_id: undefined,
        status: undefined,
      })
    })

    expect(await screen.findByText('内部进度平台')).toBeTruthy()
    expect(screen.getByText('需求分析')).toBeTruthy()
    expect(screen.getByText('完成登录接口开发')).toBeTruthy()
    expect(screen.getAllByText('张三').length).toBeGreaterThan(0)
  })

  it('keeps the current workspace visible while a member refresh is loading', async () => {
    vi.mocked(getProjectDetail)
      .mockResolvedValueOnce({
        id: 1001,
        name: '内部进度平台',
        description: '用于内部项目进度跟踪',
        owner_user_id: 1,
        owner_real_name: '张三',
        status: 2,
        planned_start_date: '2026-03-20',
        planned_end_date: '2026-06-30',
        completed_at: null,
        created_by: 1,
        created_by_real_name: '张三',
        created_at: '2026-03-19T11:00:00+08:00',
        updated_at: '2026-03-27T09:30:00+08:00',
        member_count: 2,
        node_count: 1,
        completed_node_count: 0,
        sub_task_count: 1,
        completed_sub_task_count: 0,
        permissions: {
          can_edit_basic: true,
          can_manage_members: true,
          can_manage_nodes: true,
          can_transfer_owner: true,
          can_delete: true,
        },
      })
      .mockImplementationOnce(
        () =>
          new Promise(() => {
            return undefined
          }),
      )

    vi.mocked(listProjectMembers).mockResolvedValue({
      list: [
        {
          project_id: 1001,
          user_id: 1,
          username: 'zhangsan',
          real_name: '张三',
          system_role: 2,
          status: 1,
          joined_at: '2026-03-19T11:00:00+08:00',
          is_owner: true,
        },
        {
          project_id: 1001,
          user_id: 2,
          username: 'lisi',
          real_name: '李四',
          system_role: 1,
          status: 1,
          joined_at: '2026-03-20T11:00:00+08:00',
          is_owner: false,
        },
      ],
    })

    vi.mocked(listProjectNodes).mockResolvedValue({
      list: [
        {
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
      ],
    })

    vi.mocked(listNodeSubtasks).mockResolvedValue({
      list: [
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
          created_by: 1,
          created_at: '2026-03-27T15:00:00+08:00',
          updated_at: '2026-03-28T09:30:00+08:00',
        },
      ],
    })

    vi.mocked(removeProjectMember).mockResolvedValue(undefined)

    const screen = render(ProjectDetailView, {
      global: {
        plugins: [createPinia()],
      },
    })
    const user = userEvent.setup()

    expect(await screen.findByText('内部进度平台')).toBeTruthy()
    expect(screen.getByText('李四')).toBeTruthy()

    await user.click(screen.getByTestId('remove-member-2'))

    await waitFor(() => {
      expect(removeProjectMember).toHaveBeenCalledWith(1001, 2)
    })

    expect(screen.getByText('内部进度平台')).toBeTruthy()
    expect(screen.getByText('李四')).toBeTruthy()
    expect(screen.getByText('工作台刷新中...')).toBeTruthy()
  })
})
