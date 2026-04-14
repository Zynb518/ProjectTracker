import { createPinia, setActivePinia } from 'pinia'
import { fireEvent, render, waitFor, within } from '@testing-library/vue'
import userEvent from '@testing-library/user-event'
import { readFileSync } from 'node:fs'
import { resolve } from 'node:path'
import { beforeEach, describe, expect, it, vi } from 'vitest'

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
  getProjectGanttMembers: vi.fn(),
  getProjectGanttNodes: vi.fn(),
  listProjectOwnerCandidates: vi.fn(),
  transferProjectOwner: vi.fn(),
}))

vi.mock('@/api/members', () => ({
  listProjectMembers: vi.fn(),
  listMemberCandidates: vi.fn(),
  addProjectMember: vi.fn(),
  removeProjectMember: vi.fn(),
}))

vi.mock('@/api/nodes', () => ({
  getProjectNodeDetail: vi.fn(),
  getProjectNodeGantt: vi.fn(),
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
  reopenSubtask: vi.fn(),
  submitSubtaskProgress: vi.fn(),
  listSubtaskProgressRecords: vi.fn(),
}))

import {
  getProjectDetail,
  getProjectGanttMembers,
  getProjectGanttNodes,
  listProjectOwnerCandidates,
  transferProjectOwner,
} from '@/api/projects'
import { listProjectMembers, removeProjectMember } from '@/api/members'
import {
  completeProjectNode,
  createProjectNode,
  deleteProjectNode,
  getProjectNodeDetail,
  getProjectNodeGantt,
  listProjectNodes,
  reorderProjectNodes,
  reopenProjectNode,
  startProjectNode,
  updateProjectNode,
} from '@/api/nodes'
import {
  createSubtask,
  deleteSubtask,
  listNodeSubtasks,
  reopenSubtask,
  submitSubtaskProgress,
  updateSubtask,
} from '@/api/subtasks'
import { useNotificationStore } from '@/stores/notifications'
import ProjectDetailView from '@/views/ProjectDetailView.vue'

const projectDetailFixture = {
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
}

const membersFixture = {
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
}

const nodesFixture = {
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
    {
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
    },
  ],
}

const projectGanttFixture = {
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

const memberGanttFixture = {
  project: projectGanttFixture.project,
  member_rows: [
    {
      user_id: 18,
      real_name: '王五',
      subtasks: [
        {
          subtask_id: 3001,
          name: '完成登录接口开发',
          node_id: 2002,
          node_name: '前端开发',
          status: 2,
          progress_percent: 60,
          planned_start_date: '2026-03-27',
          planned_end_date: '2026-03-31',
          completed_at: null,
        },
        {
          subtask_id: 3004,
          name: '修复筛选状态条',
          node_id: 2002,
          node_name: '前端开发',
          status: 1,
          progress_percent: 0,
          planned_start_date: '2026-03-29',
          planned_end_date: '2026-04-02',
          completed_at: null,
        },
      ],
    },
    {
      user_id: 19,
      real_name: '赵六',
      subtasks: [],
    },
  ],
}

const nodeGanttFixture = {
  project: projectGanttFixture.project,
  node: projectGanttFixture.nodes[1],
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

function mockWorkspaceData() {
  vi.mocked(getProjectDetail).mockResolvedValue(projectDetailFixture)
  vi.mocked(listProjectMembers).mockResolvedValue(membersFixture)
  vi.mocked(listProjectNodes).mockResolvedValue(nodesFixture)
  vi.mocked(listNodeSubtasks).mockResolvedValue({ list: [] })
}

describe('ProjectDetailView', () => {
  beforeEach(() => {
    vi.resetAllMocks()
  })

  it('uses lighter galaxy workspace shells without blur-driven placeholder motion', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/views/ProjectDetailView.vue'), 'utf8')

    expect(source).toContain('.project-detail__workspace-card {')
    expect(source).toContain('background: var(--gradient-surface), var(--project-card-glow);')
    expect(source).not.toContain('backdrop-filter: var(--backdrop-blur);')
    expect(source).not.toContain('animation: placeholder-float')
  })

  it('shows an error message when the workspace fails to load', async () => {
    vi.mocked(getProjectDetail).mockRejectedValue(new Error('项目详情加载失败'))
    vi.mocked(listProjectMembers).mockResolvedValue({
      list: [
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
        {
          project_id: 1001,
          user_id: 18,
          username: 'wangwu',
          real_name: '王五',
          system_role: 1,
          status: 1,
          joined_at: '2026-03-21T11:00:00+08:00',
          is_owner: false,
        },
      ],
    })
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

  it('loads the project workspace summary and opens a node drawer only after clicking an interactive node', async () => {
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
        {
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
        },
      ],
    })

    vi.mocked(listNodeSubtasks)
      .mockResolvedValueOnce({ list: [] })
      .mockResolvedValueOnce({
        list: [
          {
            id: 3001,
            node_id: 2002,
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
    const user = userEvent.setup()

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
    expect(screen.getByText('前端开发')).toBeTruthy()
    expect(screen.getAllByText('张三').length).toBeGreaterThan(0)
    expect(screen.queryByText('完成登录接口开发')).toBeNull()
    expect(screen.getByTestId('node-drawer-placeholder')).toBeTruthy()

    await user.click(screen.getByTestId('node-item-2002'))

    await waitFor(() => {
      expect(listNodeSubtasks).toHaveBeenCalledWith(1001, 2002, {
        responsible_user_id: undefined,
        status: undefined,
      })
    })

    expect(screen.getByTestId('node-drawer')).toBeTruthy()
    expect(screen.getByText('完成登录接口开发')).toBeTruthy()
    expect(screen.getByText('项目成员')).toBeTruthy()
  })

  it('renders one unified workspace card with fixed-height member panel and independently scrollable sections', async () => {
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
        {
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
        },
      ],
    })

    vi.mocked(listNodeSubtasks)
      .mockResolvedValueOnce({ list: [] })
      .mockResolvedValueOnce({
        list: [
          {
            id: 3001,
            node_id: 2002,
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
    const user = userEvent.setup()

    expect(await screen.findByText('内部进度平台')).toBeTruthy()

    await user.click(screen.getByTestId('node-item-2002'))

    const workspaceCard = await screen.findByTestId('project-workspace-card')
    const memberPanel = screen.getByTestId('member-panel')
    const memberList = screen.getByTestId('member-panel-list')
    const railScroll = screen.getByTestId('node-rail-scroll')
    const drawerContent = screen.getByTestId('node-drawer-content')

    expect(within(workspaceCard).getByTestId('node-item-2002')).toBeTruthy()
    expect(within(workspaceCard).getByTestId('node-drawer')).toBeTruthy()
    expect(getComputedStyle(workspaceCard).height).toBe('720px')
    expect(getComputedStyle(memberPanel).height).toBe('720px')
    expect(getComputedStyle(memberList).overflowY).toBe('auto')
    expect(getComputedStyle(railScroll).overflowY).toBe('auto')
    expect(getComputedStyle(drawerContent).overflowY).toBe('auto')
  })

  it('renders one merged top overview card with member, progress, and grouped metadata summaries', async () => {
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
          user_id: 18,
          username: 'wangwu',
          real_name: '王五',
          system_role: 1,
          status: 1,
          joined_at: '2026-03-21T11:00:00+08:00',
          is_owner: false,
        },
      ],
    })
    vi.mocked(listProjectNodes).mockResolvedValue({ list: [] })

    const screen = render(ProjectDetailView, {
      global: {
        plugins: [createPinia()],
      },
    })

    await waitFor(() => {
      expect(screen.getByTestId('project-overview-card')).toBeTruthy()
    })

    const overviewCard = screen.getByTestId('project-overview-card')

    expect(screen.queryByText('Project Workspace')).toBeNull()
    expect(within(overviewCard).getByText('成员')).toBeTruthy()
    expect(within(overviewCard).getByText('阶段节点')).toBeTruthy()
    expect(within(overviewCard).getByText('子任务')).toBeTruthy()
    expect(within(overviewCard).getByText('状态')).toBeTruthy()
    expect(within(overviewCard).getByText('负责人')).toBeTruthy()
    expect(within(overviewCard).getByText('周期')).toBeTruthy()
    expect(screen.getByTestId('project-meta-panel')).toBeTruthy()
  })

  it('defaults to workspace view and only loads stage gantt data after switching tabs', async () => {
    mockWorkspaceData()
    vi.mocked(getProjectGanttNodes).mockResolvedValue(projectGanttFixture)

    const screen = render(ProjectDetailView, {
      global: {
        plugins: [createPinia()],
      },
    })
    const user = userEvent.setup()

    expect(await screen.findByTestId('project-workspace-card')).toBeTruthy()
    expect(getProjectGanttNodes).not.toHaveBeenCalled()

    await user.click(screen.getByRole('button', { name: '甘特图' }))

    await waitFor(() => {
      expect(getProjectGanttNodes).toHaveBeenCalledWith(1001)
      expect(screen.getByTestId('project-gantt-view')).toBeTruthy()
    })

    await user.click(screen.getByRole('button', { name: '工作区' }))

    expect(screen.getByTestId('project-workspace-card')).toBeTruthy()
  })

  it('opens a node subtask gantt overlay on demand from the gantt view', async () => {
    mockWorkspaceData()
    vi.mocked(getProjectGanttNodes).mockResolvedValue(projectGanttFixture)
    vi.mocked(getProjectNodeGantt).mockResolvedValue(nodeGanttFixture)

    const screen = render(ProjectDetailView, {
      global: {
        plugins: [createPinia()],
      },
    })
    const user = userEvent.setup()

    await screen.findByTestId('project-workspace-card')
    await user.click(screen.getByRole('button', { name: '甘特图' }))

    await waitFor(() => {
      expect(screen.getByTestId('project-gantt-stage-bar-2002')).toBeTruthy()
    })

    expect(getProjectNodeGantt).not.toHaveBeenCalled()

    await user.click(screen.getByTestId('project-gantt-stage-bar-2002'))

    await waitFor(() => {
      expect(getProjectNodeGantt).toHaveBeenCalledWith(1001, 2002)
      expect(screen.getByTestId('node-gantt-dialog')).toBeTruthy()
    })

    expect(screen.getAllByText('完成登录接口开发').length).toBeGreaterThan(0)
  })

  it('lazily loads member gantt data only after switching to the member perspective', async () => {
    mockWorkspaceData()
    vi.mocked(getProjectGanttNodes).mockResolvedValue(projectGanttFixture)
    vi.mocked(getProjectGanttMembers).mockResolvedValue(memberGanttFixture)

    const screen = render(ProjectDetailView, {
      global: {
        plugins: [createPinia()],
      },
    })
    const user = userEvent.setup()

    await screen.findByTestId('project-workspace-card')
    await user.click(screen.getByRole('button', { name: '甘特图' }))

    await waitFor(() => {
      expect(screen.getByTestId('project-gantt-view')).toBeTruthy()
    })

    expect(getProjectGanttMembers).not.toHaveBeenCalled()

    await user.click(screen.getByRole('button', { name: '人员 / 时间' }))

    await waitFor(() => {
      expect(getProjectGanttMembers).toHaveBeenCalledWith(1001)
      expect(screen.getByTestId('project-member-gantt-view')).toBeTruthy()
    })

    expect(screen.getByText('王五')).toBeTruthy()

    await user.click(screen.getByRole('button', { name: '阶段 / 时间' }))

    await waitFor(() => {
      expect(screen.getByTestId('project-gantt-view')).toBeTruthy()
    })
  })

  it('lets managers create and edit project nodes from the timeline panel', async () => {
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
          user_id: 18,
          username: 'wangwu',
          real_name: '王五',
          system_role: 1,
          status: 1,
          joined_at: '2026-03-21T11:00:00+08:00',
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
        {
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
        },
      ],
    })
    vi.mocked(listNodeSubtasks).mockResolvedValue({ list: [] })
    vi.mocked(createProjectNode).mockResolvedValue({
      id: 2003,
      project_id: 1001,
      name: '联调测试',
      description: '联调并修复接口问题',
      sequence_no: 3,
      status: 1,
      planned_start_date: '2026-04-21',
      planned_end_date: '2026-04-28',
      completed_at: null,
      created_by: 1,
      created_at: '2026-04-01T09:00:00+08:00',
      updated_at: '2026-04-01T09:00:00+08:00',
      sub_task_count: 0,
      completed_sub_task_count: 0,
    })
    vi.mocked(getProjectNodeDetail).mockResolvedValue({
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
    })
    vi.mocked(updateProjectNode).mockResolvedValue({
      id: 2002,
      project_id: 1001,
      name: '前端开发阶段',
      description: '完成项目界面与交互实现',
      sequence_no: 2,
      status: 2,
      planned_start_date: '2026-03-26',
      planned_end_date: '2026-04-22',
      completed_at: null,
      created_by: 1,
      created_at: '2026-03-25T18:20:00+08:00',
      updated_at: '2026-03-29T10:00:00+08:00',
      sub_task_count: 6,
      completed_sub_task_count: 2,
    })

    const screen = render(ProjectDetailView, {
      global: {
        plugins: [createPinia()],
      },
    })
    const user = userEvent.setup()

    await screen.findByText('内部进度平台')

    await user.click(screen.getByTestId('create-node'))

    await user.type(screen.getByLabelText('节点名称'), '联调测试')
    await user.type(screen.getByLabelText('节点说明'), '联调并修复接口问题')
    await user.type(screen.getByLabelText('计划开始'), '2026-04-21')
    await user.type(screen.getByLabelText('计划结束'), '2026-04-28')
    await user.click(screen.getByRole('button', { name: '创建节点' }))

    await waitFor(() => {
      expect(createProjectNode).toHaveBeenCalledWith(1001, {
        name: '联调测试',
        description: '联调并修复接口问题',
        planned_start_date: '2026-04-21',
        planned_end_date: '2026-04-28',
      })
    })

    await user.click(screen.getByTestId('node-view-mode-full'))
    await user.click(screen.getByTestId('node-action-edit-2002'))

    await waitFor(() => {
      expect(getProjectNodeDetail).toHaveBeenCalledWith(1001, 2002)
    })

    const nameInput = screen.getByLabelText('节点名称')
    const endDateInput = screen.getByLabelText('计划结束')

    await user.clear(nameInput)
    await user.type(nameInput, '前端开发阶段')
    await user.clear(endDateInput)
    await user.type(endDateInput, '2026-04-22')
    await user.click(screen.getByRole('button', { name: '保存节点' }))

    await waitFor(() => {
      expect(updateProjectNode).toHaveBeenCalledWith(1001, 2002, {
        name: '前端开发阶段',
        description: '完成项目界面与交互实现',
        planned_start_date: '2026-03-26',
        planned_end_date: '2026-04-22',
      })
    })
  })

  it('opens the subtask dialog from the drawer and creates a subtask for the active node', async () => {
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
      node_count: 2,
      completed_node_count: 0,
      sub_task_count: 0,
      completed_sub_task_count: 0,
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
          user_id: 18,
          username: 'wangwu',
          real_name: '王五',
          system_role: 1,
          status: 1,
          joined_at: '2026-03-21T11:00:00+08:00',
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
          status: 1,
          planned_start_date: '2026-03-20',
          planned_end_date: '2026-03-25',
          completed_at: null,
          created_by: 1,
          created_at: '2026-03-19T11:20:00+08:00',
          updated_at: '2026-03-25T18:00:00+08:00',
          sub_task_count: 0,
          completed_sub_task_count: 0,
        },
        {
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
          sub_task_count: 0,
          completed_sub_task_count: 0,
        },
      ],
    })
    vi.mocked(listNodeSubtasks).mockResolvedValue({ list: [] })
    vi.mocked(createSubtask).mockResolvedValue({
      id: 3001,
      node_id: 2002,
      name: '实现项目卡片详情',
      description: '补齐卡片数据展示与布局',
      responsible_user_id: 0,
      status: 1,
      progress_percent: 0,
      priority: 2,
      planned_start_date: '2026-04-09',
      planned_end_date: '2026-04-12',
      completed_at: null,
      updated_at: '2026-04-08T22:00:00+08:00',
    })

    const screen = render(ProjectDetailView, {
      global: {
        plugins: [createPinia()],
      },
    })
    const user = userEvent.setup()

    await screen.findByText('内部进度平台')

    await user.click(screen.getByTestId('node-item-2002'))
    await screen.findByTestId('node-drawer')

    await user.click(screen.getByTestId('create-subtask'))

    const startInput = screen.getByLabelText('计划开始')
    const endInput = screen.getByLabelText('计划结束')
    const responsibleSelect = screen.getByLabelText('负责人') as HTMLSelectElement

    await user.type(screen.getByLabelText('子任务名称'), '实现项目卡片详情')
    await user.type(screen.getByLabelText('子任务说明'), '补齐卡片数据展示与布局')
    await user.clear(startInput)
    await user.type(startInput, '2026-04-09')
    await user.clear(endInput)
    await user.type(endInput, '2026-04-12')
    await user.selectOptions(screen.getByLabelText('优先级'), '2')

    expect(screen.queryByRole('option', { name: '暂不指定' })).toBeNull()
    expect(screen.getByRole('option', { name: '张三' })).toBeTruthy()
    expect(responsibleSelect.value).toBe('1')

    await user.click(screen.getByRole('button', { name: '创建子任务' }))

    await waitFor(() => {
      expect(createSubtask).toHaveBeenCalledWith(1001, 2002, {
        name: '实现项目卡片详情',
        description: '补齐卡片数据展示与布局',
        planned_start_date: '2026-04-09',
        planned_end_date: '2026-04-12',
        priority: 2,
        responsible_user_id: 1,
      })
    })
  })

  it('connects subtask edit/remove for unfinished items and reopen for 100% subtasks inside the active node drawer', async () => {
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
      node_count: 2,
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

    vi.mocked(listProjectMembers).mockResolvedValue({
      list: [
        {
          project_id: 1001,
          user_id: 18,
          username: 'wangwu',
          real_name: '王五',
          system_role: 1,
          status: 1,
          joined_at: '2026-03-21T11:00:00+08:00',
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
          status: 1,
          planned_start_date: '2026-03-20',
          planned_end_date: '2026-03-25',
          completed_at: null,
          created_by: 1,
          created_at: '2026-03-19T11:20:00+08:00',
          updated_at: '2026-03-25T18:00:00+08:00',
          sub_task_count: 0,
          completed_sub_task_count: 0,
        },
        {
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
          sub_task_count: 1,
          completed_sub_task_count: 0,
        },
      ],
    })
    vi.mocked(listNodeSubtasks).mockResolvedValue({
      list: [
        {
          id: 3001,
          node_id: 2002,
          name: '实现项目卡片详情',
          description: '补齐卡片数据展示与布局',
          responsible_user_id: 18,
          responsible_real_name: '王五',
          status: 1,
          progress_percent: 0,
          priority: 2,
          planned_start_date: '2026-04-09',
          planned_end_date: '2026-04-12',
          completed_at: null,
          updated_at: '2026-04-08T22:00:00+08:00',
        },
        {
          id: 3002,
          node_id: 2002,
          name: '完成登录联调验收',
          description: '收尾校验联调流程',
          responsible_user_id: 18,
          responsible_real_name: '王五',
          status: 3,
          progress_percent: 100,
          priority: 2,
          planned_start_date: '2026-04-13',
          planned_end_date: '2026-04-15',
          completed_at: '2026-04-15T18:00:00+08:00',
          updated_at: '2026-04-15T18:00:00+08:00',
        },
      ],
    })
    vi.mocked(updateSubtask).mockResolvedValue({
      id: 3001,
      node_id: 2002,
      name: '实现项目卡片详情 v2',
      description: '补齐卡片数据展示与布局并修正交互',
      responsible_user_id: 18,
      responsible_real_name: '王五',
      status: 1,
      progress_percent: 0,
      priority: 3,
      planned_start_date: '2026-04-10',
      planned_end_date: '2026-04-15',
      completed_at: null,
      updated_at: '2026-04-08T22:05:00+08:00',
    })
    vi.mocked(reopenSubtask).mockResolvedValue({ id: 3001, status: 2 })
    vi.mocked(deleteSubtask).mockResolvedValue({ id: 3001 })
    vi.spyOn(window, 'confirm').mockReturnValue(true)

    const screen = render(ProjectDetailView, {
      global: {
        plugins: [createPinia()],
      },
    })
    const user = userEvent.setup()

    await screen.findByText('内部进度平台')
    await user.click(screen.getByTestId('node-item-2002'))
    await screen.findByTestId('node-drawer')

    await user.click(screen.getByTestId('subtask-edit-3001'))

    const nameInput = screen.getByLabelText('子任务名称')
    const descriptionInput = screen.getByLabelText('子任务说明')
    const startInput = screen.getByLabelText('计划开始')
    const endInput = screen.getByLabelText('计划结束')

    await user.clear(nameInput)
    await user.type(nameInput, '实现项目卡片详情 v2')
    await user.clear(descriptionInput)
    await user.type(descriptionInput, '补齐卡片数据展示与布局并修正交互')
    await user.clear(startInput)
    await user.type(startInput, '2026-04-10')
    await user.clear(endInput)
    await user.type(endInput, '2026-04-15')
    await user.selectOptions(screen.getByLabelText('优先级'), '3')
    await user.click(screen.getByRole('button', { name: '保存子任务' }))

    await waitFor(() => {
      expect(updateSubtask).toHaveBeenCalledWith(3001, {
        name: '实现项目卡片详情 v2',
        description: '补齐卡片数据展示与布局并修正交互',
        planned_start_date: '2026-04-10',
        planned_end_date: '2026-04-15',
        priority: 3,
        responsible_user_id: 18,
      })
    })

    expect(screen.queryByTestId('subtask-start-3001')).toBeNull()
    expect(screen.getByTestId('subtask-reopen-3001').getAttribute('disabled')).toBe('')
    expect(screen.getByTestId('subtask-edit-3002').getAttribute('disabled')).toBe('')

    await user.click(screen.getByTestId('subtask-reopen-3002'))
    await waitFor(() => {
      expect(reopenSubtask).toHaveBeenCalledWith(3002)
    })

    await user.click(screen.getByTestId('subtask-remove-3001'))
    await waitFor(() => {
      expect(deleteSubtask).toHaveBeenCalledWith(3001)
    })
  })

  it('submits progress without status and keeps the slider floor at the current progress', async () => {
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
      member_count: 1,
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
    vi.mocked(listProjectMembers).mockResolvedValue({
      list: [
        {
          project_id: 1001,
          user_id: 18,
          username: 'wangwu',
          real_name: '王五',
          system_role: 1,
          status: 1,
          joined_at: '2026-03-21T11:00:00+08:00',
          is_owner: false,
        },
      ],
    })
    vi.mocked(listProjectNodes).mockResolvedValue({
      list: [
        {
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
          sub_task_count: 1,
          completed_sub_task_count: 0,
        },
      ],
    })
    vi.mocked(listNodeSubtasks).mockResolvedValue({
      list: [
        {
          id: 3001,
          node_id: 2002,
          name: '实现项目卡片详情',
          description: '补齐卡片数据展示与布局',
          responsible_user_id: 18,
          responsible_real_name: '王五',
          status: 2,
          progress_percent: 60,
          priority: 2,
          planned_start_date: '2026-04-09',
          planned_end_date: '2026-04-12',
          completed_at: null,
          updated_at: '2026-04-08T22:00:00+08:00',
        },
      ],
    })
    vi.mocked(submitSubtaskProgress).mockResolvedValue({
      subtask: { id: 3001, progress_percent: 80 },
      progress_record: {
        id: 9002,
        sub_task_id: 3001,
        operator_user_id: 1,
        operator_real_name: '张三',
        progress_note: '推进到联调',
        progress_percent: 80,
        status: 2,
        created_at: '2026-04-08T22:10:00+08:00',
      },
    })

    const screen = render(ProjectDetailView, {
      global: {
        plugins: [createPinia()],
      },
    })
    const user = userEvent.setup()

    await screen.findByText('内部进度平台')
    await user.click(screen.getByTestId('node-item-2002'))
    await screen.findByTestId('node-drawer')
    await user.click(screen.getByTestId('subtask-progress-3001'))

    const slider = screen.getByTestId('subtask-progress-range')

    expect(slider.getAttribute('min')).toBe('0')
    expect((slider as HTMLInputElement).value).toBe('60')
    await fireEvent.update(slider, '30')
    await user.type(screen.getByLabelText('进度说明'), '推进到联调')
    await fireEvent.update(slider, '80')
    await user.click(screen.getByRole('button', { name: '提交进度' }))

    await waitFor(() => {
      expect(submitSubtaskProgress).toHaveBeenCalledWith(3001, {
        progress_percent: 80,
        progress_note: '推进到联调',
      })
    })
  })

  it('calls node lifecycle, delete, and reorder APIs from timeline actions', async () => {
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

    vi.mocked(listProjectMembers).mockResolvedValue({ list: [] })
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
        {
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
        },
        {
          id: 2003,
          project_id: 1001,
          name: '联调测试',
          description: '联调并修复接口问题',
          sequence_no: 3,
          status: 1,
          planned_start_date: '2026-04-21',
          planned_end_date: '2026-04-28',
          completed_at: null,
          created_by: 1,
          created_at: '2026-04-01T09:00:00+08:00',
          updated_at: '2026-04-06T12:00:00+08:00',
          sub_task_count: 0,
          completed_sub_task_count: 0,
        },
      ],
    })
    vi.mocked(listNodeSubtasks).mockResolvedValue({ list: [] })
    vi.mocked(startProjectNode).mockResolvedValue({ id: 2003, status: 2 })
    vi.mocked(completeProjectNode).mockResolvedValue({ id: 2002, status: 3 })
    vi.mocked(reopenProjectNode).mockResolvedValue({ id: 2001, status: 2 })
    vi.mocked(deleteProjectNode).mockResolvedValue({ id: 2003 })
    vi.mocked(reorderProjectNodes).mockResolvedValue({
      project_id: 1001,
      updated_at: '2026-04-08T19:40:00+08:00',
    })

    vi.spyOn(window, 'confirm').mockReturnValue(true)

    const screen = render(ProjectDetailView, {
      global: {
        plugins: [createPinia()],
      },
    })
    const user = userEvent.setup()

    await screen.findByText('内部进度平台')

    await user.click(screen.getByTestId('node-view-mode-full'))
    await user.click(screen.getByTestId('node-action-complete-2002'))
    await waitFor(() => {
      expect(completeProjectNode).toHaveBeenCalledWith(1001, 2002)
    })

    await user.click(screen.getByTestId('node-action-reopen-2001'))
    await waitFor(() => {
      expect(reopenProjectNode).toHaveBeenCalledWith(1001, 2001)
    })

    await user.click(screen.getByTestId('node-action-start-2003'))
    await waitFor(() => {
      expect(startProjectNode).toHaveBeenCalledWith(1001, 2003)
    })

    await fireEvent.dragStart(screen.getByTestId('node-item-2003'))
    await fireEvent.dragEnter(screen.getByTestId('node-item-2002'))
    await fireEvent.drop(screen.getByTestId('node-item-2002'))

    await waitFor(() => {
      expect(reorderProjectNodes).toHaveBeenCalledWith(1001, {
        nodes: [
          { node_id: 2001, sequence_no: 1 },
          { node_id: 2003, sequence_no: 2 },
          { node_id: 2002, sequence_no: 3 },
        ],
      })
    })

    await user.click(screen.getByTestId('node-action-remove-2003'))
    await waitFor(() => {
      expect(deleteProjectNode).toHaveBeenCalledWith(1001, 2003)
    })
  })

  it('reorders nodes locally without triggering a workspace refresh', async () => {
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
        member_count: 5,
        node_count: 3,
        completed_node_count: 1,
        sub_task_count: 6,
        completed_sub_task_count: 2,
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

    vi.mocked(listProjectMembers)
      .mockResolvedValueOnce({
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
      .mockImplementationOnce(
        () =>
          new Promise(() => {
            return undefined
          }),
      )

    vi.mocked(listProjectNodes)
      .mockResolvedValueOnce({
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
          {
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
          },
          {
            id: 2003,
            project_id: 1001,
            name: '联调测试',
            description: '联调并修复接口问题',
            sequence_no: 3,
            status: 1,
            planned_start_date: '2026-04-21',
            planned_end_date: '2026-04-28',
            completed_at: null,
            created_by: 1,
            created_at: '2026-04-01T09:00:00+08:00',
            updated_at: '2026-04-06T12:00:00+08:00',
            sub_task_count: 0,
            completed_sub_task_count: 0,
          },
        ],
      })
      .mockImplementationOnce(
        () =>
          new Promise(() => {
            return undefined
          }),
      )

    vi.mocked(listNodeSubtasks).mockResolvedValue({ list: [] })
    vi.mocked(reorderProjectNodes).mockResolvedValue({
      project_id: 1001,
      updated_at: '2026-04-08T20:20:00+08:00',
    })

    const screen = render(ProjectDetailView, {
      global: {
        plugins: [createPinia()],
      },
    })

    expect(await screen.findByText('内部进度平台')).toBeTruthy()

    await fireEvent.dragStart(screen.getByTestId('node-item-2003'))
    await fireEvent.dragEnter(screen.getByTestId('node-item-2002'))
    await fireEvent.drop(screen.getByTestId('node-item-2002'))

    await waitFor(() => {
      expect(reorderProjectNodes).toHaveBeenCalledWith(1001, {
        nodes: [
          { node_id: 2001, sequence_no: 1 },
          { node_id: 2003, sequence_no: 2 },
          { node_id: 2002, sequence_no: 3 },
        ],
      })
    })

    await waitFor(() => {
      expect(screen.getAllByTestId(/node-row-/).map((item) => item.getAttribute('data-testid'))).toEqual([
        'node-row-2001',
        'node-row-2003',
        'node-row-2002',
      ])
    })

    expect(getProjectDetail).toHaveBeenCalledTimes(1)
    expect(listProjectMembers).toHaveBeenCalledTimes(1)
    expect(listProjectNodes).toHaveBeenCalledTimes(1)
    expect(screen.queryByText('工作台刷新中...')).toBeNull()
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

  it('loads owner candidates and transfers the project owner from the member panel', async () => {
    vi.mocked(getProjectDetail)
      .mockResolvedValueOnce(projectDetailFixture)
      .mockResolvedValueOnce({
        ...projectDetailFixture,
        owner_user_id: 12,
        owner_real_name: '李四',
        member_count: 2,
      })

    vi.mocked(listProjectMembers)
      .mockResolvedValueOnce(membersFixture)
      .mockResolvedValueOnce({
        list: [
          {
            project_id: 1001,
            user_id: 12,
            username: 'lisi',
            real_name: '李四',
            system_role: 2,
            status: 1,
            joined_at: '2026-04-10T09:00:00+08:00',
            is_owner: true,
          },
          {
            project_id: 1001,
            user_id: 1,
            username: 'zhangsan',
            real_name: '张三',
            system_role: 2,
            status: 1,
            joined_at: '2026-03-19T11:00:00+08:00',
            is_owner: false,
          },
        ],
      })

    vi.mocked(listProjectNodes).mockResolvedValue(nodesFixture)
    vi.mocked(listNodeSubtasks).mockResolvedValue({ list: [] })
    vi.mocked(listProjectOwnerCandidates).mockResolvedValue({
      list: [
        {
          id: 12,
          username: 'lisi',
          real_name: '李四',
          system_role: 2,
          status: 1,
          is_project_member: false,
        },
      ],
      total: 1,
      page: 1,
      page_size: 20,
    })
    vi.mocked(transferProjectOwner).mockResolvedValue({
      project_id: 1001,
      previous_owner_user_id: 1,
      owner_user_id: 12,
      owner_real_name: '李四',
      auto_added_as_member: true,
      updated_at: '2026-04-10T09:30:00+08:00',
    })

    const screen = render(ProjectDetailView, {
      global: {
        plugins: [createPinia()],
      },
    })
    const user = userEvent.setup()

    expect(await screen.findByTestId('transfer-owner-1')).toBeTruthy()

    await user.click(screen.getByTestId('transfer-owner-1'))

    await waitFor(() => {
      expect(listProjectOwnerCandidates).toHaveBeenCalledWith(1001, {
        keyword: '',
        page: 1,
        page_size: 20,
      })
    })

    await user.click(await screen.findByTestId('owner-candidate-12'))
    await user.click(screen.getByRole('button', { name: '确认转交' }))

    await waitFor(() => {
      expect(transferProjectOwner).toHaveBeenCalledWith(1001, 12)
    })

    await waitFor(() => {
      expect(getProjectDetail).toHaveBeenCalledTimes(2)
      expect(listProjectMembers).toHaveBeenCalledTimes(2)
    })

    expect(await screen.findByTestId('transfer-owner-12')).toBeTruthy()
    expect(screen.queryByText('转交项目负责人')).toBeNull()
  })
})
