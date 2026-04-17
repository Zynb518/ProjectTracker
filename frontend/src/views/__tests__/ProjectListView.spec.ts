import { createPinia, setActivePinia } from 'pinia'
import { render, waitFor, within } from '@testing-library/vue'
import userEvent from '@testing-library/user-event'
import { readFileSync } from 'node:fs'
import { resolve } from 'node:path'
import { beforeEach, describe, expect, it, vi } from 'vitest'

const pushMock = vi.fn()

vi.mock('vue-router', async () => {
  const actual = await vi.importActual<typeof import('vue-router')>('vue-router')

  return {
    ...actual,
    useRouter: () => ({
      push: pushMock,
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
    pushMock.mockReset()
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

  it('renders a gantt overview action beside the project filters', async () => {
    const screen = render(ProjectListView)

    await screen.findByText('内部进度平台')

    const shell = screen.getByTestId('project-list-hero-shell')
    const ganttButton = within(shell).getByRole('button', { name: '打开项目甘特图' })

    expect(ganttButton).toBeTruthy()
    expect(ganttButton.textContent).toContain('项目甘特图')
  })

  it('opens the project gantt dialog and fetches all filtered projects across pages', async () => {
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
        total: 101,
        page: 1,
        page_size: 9,
      })
      .mockResolvedValueOnce({
        list: Array.from({ length: 100 }, (_, index) => ({
          id: 2000 + index,
          name: `甘特图项目 ${index + 1}`,
          description: `批量项目 ${index + 1}`,
          owner_user_id: 1,
          owner_real_name: '张三',
          status: index % 2 === 0 ? 2 : 1,
          planned_start_date: '2026-03-20',
          planned_end_date: '2026-06-30',
          completed_at: null,
          member_count: 5,
          node_count: 3,
          sub_task_count: 12,
          created_at: '2026-03-19T11:00:00+08:00',
          updated_at: '2026-03-27T09:30:00+08:00',
          is_owner: true,
        })),
        total: 101,
        page: 1,
        page_size: 100,
      })
      .mockResolvedValueOnce({
        list: [
          {
            id: 2101,
            name: '甘特图项目 101',
            description: '批量项目 101',
            owner_user_id: 2,
            owner_real_name: '李四',
            status: 3,
            planned_start_date: '2026-06-01',
            planned_end_date: '2026-07-15',
            completed_at: null,
            member_count: 4,
            node_count: 2,
            sub_task_count: 8,
            created_at: '2026-03-29T11:00:00+08:00',
            updated_at: '2026-03-30T09:30:00+08:00',
            is_owner: false,
          },
        ],
        total: 101,
        page: 2,
        page_size: 100,
      })

    const screen = render(ProjectListView)
    const user = userEvent.setup()

    await screen.findByText('内部进度平台')

    await user.click(screen.getByRole('button', { name: '打开项目甘特图' }))

    await waitFor(() => {
      expect(listProjects).toHaveBeenNthCalledWith(2, {
        keyword: '',
        status: undefined,
        page: 1,
        page_size: 100,
      })
      expect(listProjects).toHaveBeenNthCalledWith(3, {
        keyword: '',
        status: undefined,
        page: 2,
        page_size: 100,
      })
    })

    expect(await screen.findByText('项目时间总览')).toBeTruthy()
    expect(screen.getByRole('button', { name: '打开项目 甘特图项目 101 的详情页' })).toBeTruthy()
  })

  it('navigates to project detail when clicking a gantt bar', async () => {
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
        total: 1,
        page: 1,
        page_size: 9,
      })
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
        total: 1,
        page: 1,
        page_size: 100,
      })

    const screen = render(ProjectListView)
    const user = userEvent.setup()

    await screen.findByText('内部进度平台')
    await user.click(screen.getByRole('button', { name: '打开项目甘特图' }))
    await screen.findByText('项目时间总览')

    await user.click(screen.getByRole('button', { name: '打开项目 内部进度平台 的详情页' }))

    expect(pushMock).toHaveBeenCalledWith('/projects/1001')
  })

  it('locks page scrolling while the project gantt dialog is open and restores it after closing', async () => {
    const screen = render(ProjectListView)
    const user = userEvent.setup()

    document.documentElement.style.overflow = ''
    document.body.style.overflow = ''

    await screen.findByText('内部进度平台')
    await user.click(screen.getByRole('button', { name: '打开项目甘特图' }))
    await screen.findByText('项目时间总览')

    expect(document.documentElement.style.overflow).toBe('hidden')
    expect(document.body.style.overflow).toBe('hidden')

    await user.click(screen.getByRole('button', { name: '关闭项目甘特图' }))

    await waitFor(() => {
      expect(screen.queryByText('项目时间总览')).toBeNull()
    })

    expect(document.documentElement.style.overflow).toBe('')
    expect(document.body.style.overflow).toBe('')
  })

  it('renders each project as one table row with summary cells beside the timeline bar', async () => {
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
        total: 1,
        page: 1,
        page_size: 9,
      })
      .mockResolvedValueOnce({
        list: Array.from({ length: 18 }, (_, index) => ({
          id: 3000 + index,
          name: `对齐项目 ${index + 1}`,
          description: `滚动对齐项目 ${index + 1}`,
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
        })),
        total: 18,
        page: 1,
        page_size: 100,
      })

    const screen = render(ProjectListView)
    const user = userEvent.setup()

    await screen.findByText('内部进度平台')
    await user.click(screen.getByRole('button', { name: '打开项目甘特图' }))
    await screen.findByText('项目时间总览')

    const tableScroll = screen.getByTestId('project-list-gantt-table-scroll')
    const firstRow = screen.getByTestId('project-list-gantt-row-3000')
    const firstBar = within(firstRow).getByRole('button', { name: '打开项目 对齐项目 1 的详情页' })

    expect(tableScroll).toBeTruthy()
    expect(firstRow.textContent).toContain('对齐项目 1')
    expect(firstRow.textContent).toContain('进行中')
    expect(firstBar).toBeTruthy()
  })

  it('uses one unified table scroll host instead of mirrored top-scroll and sidebar sync rails', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/projects/ProjectListGanttDialog.vue'), 'utf8')

    expect(source).toContain('data-testid="project-list-gantt-table-scroll"')
    expect(source).not.toContain('data-testid="project-list-gantt-top-scroll"')
    expect(source).not.toContain('data-testid="project-list-gantt-timeline-scroll"')
    expect(source).not.toContain('project-list-gantt-dialog__sidebar-scroll')
    expect(source).not.toContain('project-list-gantt-dialog__sidebar-rows-rail')
    expect(source).not.toContain('handleSidebarWheel')
    expect(source).not.toContain('syncSidebarOffset')
    expect(source).toContain('project-list-gantt-dialog__table-scroll')
    expect(source).toContain('.project-list-gantt-dialog__table-scroll {')
    expect(source).toContain('overflow-x: scroll;')
    expect(source).toContain('overflow-y: scroll;')
    expect(source).toContain('scrollbar-gutter: stable;')
    expect(source).toContain('.project-list-gantt-dialog__table-header {')
  })

  it('keeps time cells only in the top axis while project rows render without a track grid layer', async () => {
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
        total: 1,
        page: 1,
        page_size: 9,
      })
      .mockResolvedValueOnce({
        list: [
          {
            id: 6001,
            name: '背景网格项目',
            description: '背景网格项目',
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
        page_size: 100,
      })

    const screen = render(ProjectListView)
    const user = userEvent.setup()

    await screen.findByText('内部进度平台')
    await user.click(screen.getByRole('button', { name: '打开项目甘特图' }))
    await screen.findByText('项目时间总览')

    const axisCells = document.querySelectorAll('.project-list-gantt-dialog__axis-cell')
    const trackCells = document.querySelectorAll('.project-list-gantt-dialog__track-cell')
    const trackGrid = document.querySelector('.project-list-gantt-dialog__track-grid') as HTMLElement | null
    const firstRow = screen.getByTestId('project-list-gantt-row-6001')
    const firstBar = firstRow.querySelector('.project-list-gantt-dialog__bar') as HTMLElement | null

    expect(axisCells.length).toBeGreaterThan(0)
    expect(trackCells).toHaveLength(0)
    expect(trackGrid).toBeNull()
    expect(firstBar).not.toBeNull()
    expect(firstRow.textContent).toContain('背景网格项目')
    expect(firstRow.textContent).toContain('进行中')
    expect(firstBar?.style.left).not.toBe('')
    expect(firstBar?.style.width).not.toBe('')
  })

  it('merges the overview and filter areas into one shared top panel', async () => {
    const screen = render(ProjectListView)

    await screen.findByText('内部进度平台')

    const shell = screen.getByTestId('project-list-hero-shell')
    const filters = shell.querySelector('.project-filters')

    expect(shell.textContent).toContain('项目总览')
    expect(filters).not.toBeNull()
    expect(getComputedStyle(filters!).borderTopWidth).toBe('0px')
    expect(getComputedStyle(filters!).backgroundImage).toBe('none')
    expect(getComputedStyle(filters!).boxShadow).toBe('none')
  })

  it('uses galaxy-surface hero and pagination shells without heavy backdrop blur', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/views/ProjectListView.vue'), 'utf8')

    expect(source).toContain('.project-list__hero-shell {')
    expect(source).toContain('background: var(--gradient-surface), var(--project-card-glow);')
    expect(source).toContain('.project-list__pagination {')
    expect(source).not.toContain('backdrop-filter: var(--backdrop-blur);')
  })

  it('keeps the project gantt dialog on native vertical scrolling and removes always-on blur glow effects', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/projects/ProjectListGanttDialog.vue'), 'utf8')

    expect(source).toContain('data-testid="project-list-gantt-table-scroll"')
    expect(source).not.toContain(`v-smooth-wheel="{ axis: 'vertical', wheelBehavior: 'native', multiplier: 1.3 }"`)
    expect(source).not.toContain('backdrop-filter: blur(18px);')
    expect(source).not.toContain('animation: project-list-gantt-glow-drift')
    expect(source).toContain('.project-list-gantt-dialog__body-scroll {')
    expect(source).toContain('padding: 0;')
  })

  it('uses native horizontal scrolling and flatter moving-region styles in the project gantt dialog', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/projects/ProjectListGanttDialog.vue'), 'utf8')

    expect(source).not.toContain(`v-smooth-wheel="{ axis: 'horizontal', wheelBehavior: 'block' }"`)
    expect(source).toContain('.project-list-gantt-dialog__track {')
    expect(source).toContain('background: var(--panel-bg);')
    expect(source).not.toContain('.project-list-gantt-dialog__axis-cell:nth-child(even) {')
    expect(source).not.toContain('box-shadow: 0 16px 28px rgba(7, 18, 40, 0.2);')
    expect(source).toContain('.project-list-gantt-dialog__table-header {')
    expect(source).toContain('position: sticky;')
    expect(source).toContain('top: 0;')
    expect(source).toContain('.project-list-gantt-dialog__table-scroll::-webkit-scrollbar {')
    expect(source).toContain('height: 12px;')
    expect(source).toContain('width: 12px;')
    expect(source).toContain('.project-list-gantt-dialog__row-label {')
    expect(source).toContain('.project-list-gantt-dialog__row-status {')
    expect(source).toContain('.project-list-gantt-dialog__bar--pending {')
    expect(source).toContain('background: var(--work-status-pending-strong);')
    expect(source).toContain('color: var(--work-status-pending-contrast);')
    expect(source).toContain('.project-list-gantt-dialog__bar--active {')
    expect(source).toContain('background: var(--work-status-active-strong);')
    expect(source).toContain('color: var(--work-status-active-contrast);')
    expect(source).toContain('.project-list-gantt-dialog__bar--done {')
    expect(source).toContain('background: var(--work-status-done-strong);')
    expect(source).toContain('color: var(--work-status-done-contrast);')
    expect(source).toContain('.project-list-gantt-dialog__bar--delayed {')
    expect(source).toContain('background: var(--work-status-delayed-strong);')
    expect(source).toContain('color: var(--work-status-delayed-contrast);')
    expect(source).not.toContain('background: linear-gradient(135deg, var(--work-status-pending-color)')
    expect(source).not.toContain('background: linear-gradient(135deg, var(--work-status-active-color)')
    expect(source).not.toContain('background: linear-gradient(135deg, var(--work-status-done-color)')
    expect(source).not.toContain('background: linear-gradient(135deg, var(--work-status-delayed-color)')
  })

  it('renders an explicit close button label in the gantt dialog toolbar so exit is visible at a glance', async () => {
    const screen = render(ProjectListView)
    const user = userEvent.setup()

    await screen.findByText('内部进度平台')
    await user.click(screen.getByRole('button', { name: '打开项目甘特图' }))
    await screen.findByText('项目时间总览')

    expect(screen.getByRole('button', { name: '关闭项目甘特图' }).textContent).toContain('关闭')
  })

  it('reduces horizontal scroll repaint pressure by sticking one summary slab per row instead of two sticky cells', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/projects/ProjectListGanttDialog.vue'), 'utf8')

    expect(source).toContain('project-list-gantt-dialog__header-summary')
    expect(source).toContain('project-list-gantt-dialog__row-summary')
    expect(source).toContain('.project-list-gantt-dialog__row-summary {')
    expect(source).toContain('position: sticky;')
    expect(source).toContain('left: 0;')
  })

  it('removes the project-row track grid layer while keeping the axis cells for timeline coordinates', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/projects/ProjectListGanttDialog.vue'), 'utf8')

    expect(source).not.toContain('const trackGridStyle = computed(() => ({')
    expect(source).not.toContain('project-list-gantt-dialog__track-grid')
    expect(source).not.toContain("'project-list-gantt-dialog__track-cell'")
    expect(source).toContain("'project-list-gantt-dialog__axis-cell'")
  })

  it('avoids heavy row-containment hints in the project gantt dialog so small lists keep vertical scrolling lighter', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/projects/ProjectListGanttDialog.vue'), 'utf8')

    expect(source).toContain('.project-list-gantt-dialog__table-row {')
    expect(source).toContain('.project-list-gantt-dialog__row-label {')
    expect(source).toContain('.project-list-gantt-dialog__row-status {')
    expect(source).not.toContain('will-change: transform;')
    expect(source).not.toContain('content-visibility: auto;')
    expect(source).not.toContain('contain-intrinsic-size: var(--project-list-gantt-row-height);')
  })

  it('uses fog-white light-theme surfaces so the project hero, cards and pagination feel like a light overlay over the shared sky background', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/views/ProjectListView.vue'), 'utf8')

    expect(source).toContain('html.light .project-list__hero-shell,')
    expect(source).toContain('--project-list-sky-panel: linear-gradient(180deg, rgba(247, 251, 255, 0.84), rgba(235, 243, 252, 0.72));')
    expect(source).toContain('--project-list-sky-card: linear-gradient(180deg, rgba(249, 252, 255, 0.82), rgba(238, 245, 253, 0.68));')
    expect(source).toContain('--project-list-sky-text: #21324b;')
    expect(source).toContain('html.light .project-list .project-card,')
    expect(source).toContain('html.light .project-list__pagination,')
  })

  it('keeps distinct status colors in the project filter pills and reuses the shared work-status tokens so they match project cards and other status pills', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/views/ProjectListView.vue'), 'utf8')

    expect(source).toContain('html.light .project-list .project-filters__status-pill--pending,')
    expect(source).toContain('html.light .project-list .project-filters__status-pill--active,')
    expect(source).toContain('html.light .project-list .project-filters__status-pill--done,')
    expect(source).toContain('html.light .project-list .project-filters__status-pill--delayed,')
    expect(source).toContain('background: var(--work-status-pending-bg);')
    expect(source).toContain('background: var(--work-status-active-bg);')
    expect(source).toContain('background: var(--work-status-done-bg);')
    expect(source).toContain('background: var(--work-status-delayed-bg);')
    expect(source).toContain('background: var(--work-status-active-strong);')
    expect(source).toContain('color: var(--work-status-active-contrast);')
  })

  it('keeps light-theme project card hover from bleaching the fog-white surface and uses a separate streak layer instead', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/views/ProjectListView.vue'), 'utf8')

    expect(source).toContain('html.light .project-list .project-card::before,')
    expect(source).not.toContain('background: var(--project-list-sky-card-strong), var(--project-list-sky-fill);')
  })

  it('gives every project action button a more solid light-theme surface so the three-card action row feels visually unified', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/views/ProjectListView.vue'), 'utf8')

    expect(source).toContain('html.light .project-list .project-actions__button--start,')
    expect(source).toContain('html.light .project-list .project-actions__button--success,')
    expect(source).toContain('html.light .project-list .project-actions__button--warning,')
    expect(source).toContain('html.light .project-list .project-actions__button--danger,')
    expect(source).toContain('html.light .project-list .project-actions__button--neutral,')
    expect(source).toContain('background: linear-gradient(180deg, rgba(236, 246, 255, 0.9), rgba(213, 232, 249, 0.74));')
    expect(source).toContain('background: linear-gradient(180deg, rgba(255, 240, 219, 0.92), rgba(244, 215, 173, 0.78));')
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
