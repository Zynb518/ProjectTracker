import { mount } from '@vue/test-utils'
import { readFileSync } from 'node:fs'
import { resolve } from 'node:path'
import { describe, expect, it, vi } from 'vitest'

import NodeGanttDialog from '@/components/workspace/NodeGanttDialog.vue'
import ProjectMemberGanttView from '@/components/workspace/ProjectMemberGanttView.vue'
import ProjectGanttView from '@/components/workspace/ProjectGanttView.vue'
import type { ProjectMemberGantt, ProjectNodeSubtaskGantt, ProjectStageGantt } from '@/types/gantt'

const sampleProjectGantt: ProjectStageGantt = {
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

const sampleNodeGantt: ProjectNodeSubtaskGantt = {
  project: sampleProjectGantt.project,
  node: sampleProjectGantt.nodes[1],
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

const sampleMemberGantt: ProjectMemberGantt = {
  project: sampleProjectGantt.project,
  member_rows: [
    {
      user_id: 18,
      real_name: '王五',
      subtasks: [
        {
          subtask_id: 4001,
          name: '登录联调',
          node_id: 2002,
          node_name: '前端开发',
          status: 2,
          progress_percent: 50,
          planned_start_date: '2026-03-27',
          planned_end_date: '2026-03-31',
          completed_at: null,
        },
        {
          subtask_id: 4002,
          name: '权限联调',
          node_id: 2002,
          node_name: '前端开发',
          status: 1,
          progress_percent: 0,
          planned_start_date: '2026-03-29',
          planned_end_date: '2026-04-03',
          completed_at: null,
        },
        {
          subtask_id: 4003,
          name: '报表修复',
          node_id: 2002,
          node_name: '前端开发',
          status: 4,
          progress_percent: 40,
          planned_start_date: '2026-04-05',
          planned_end_date: '2026-04-07',
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

describe('ProjectGanttView', () => {
  it('keeps the left stage label and status on one line with ellipsis instead of wrapping', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectGanttView.vue'), 'utf8')

    expect(source).toContain('.project-gantt__sidebar-row {')
    expect(source).toContain('overflow: hidden;')
    expect(source).toContain('.project-gantt__status-pill {')
    expect(source).toContain('flex-shrink: 0;')
    expect(source).toContain('white-space: nowrap;')
  })

  it('keeps hover detail cards out of normal layout flow so stage hover does not shift the timeline', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectGanttView.vue'), 'utf8')

    expect(source).toContain('<Teleport to="body">')
    expect(source).toContain('.project-gantt__detail-card {')
    expect(source).toContain('position: fixed;')
    expect(source).toContain(":data-placement=\"hoveredNodePlacement\"")
    expect(source).toContain(':style="detailCardStyle"')
    expect(source).toContain('max-height: calc(100vh - 48px);')
    expect(source).toContain('pointer-events: auto;')
    expect(source).toContain(".project-gantt__detail-card[data-placement='top-right'] {")
    expect(source).not.toContain('right: 24px;')
    expect(source).not.toContain('bottom: 24px;')
    expect(source).not.toContain('translateY(calc(-50% - 1px))')
  })

  it('renders a color-coded stage hover detail card with semantic summary blocks', async () => {
    vi.useFakeTimers()

    const wrapper = mount(ProjectGanttView, {
      attachTo: document.body,
      props: {
        gantt: sampleProjectGantt,
        scale: 'week',
      },
    })

    await wrapper.get('[data-testid="project-gantt-stage-bar-2002"]').trigger('mouseenter')
    await vi.advanceTimersByTimeAsync(1000)

    const detailCard = document.body.querySelector('[data-testid="project-gantt-detail-card"]')
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectGanttView.vue'), 'utf8')

    expect(detailCard).not.toBeNull()
    expect(detailCard?.querySelector('.project-gantt__detail-status')?.className).toContain(
      'project-gantt__detail-status--active',
    )
    expect(detailCard?.querySelector('.project-gantt__detail-stat')?.className).toContain(
      'project-gantt__detail-stat--active',
    )
    expect(detailCard?.textContent).toContain('进行中')
    expect(detailCard?.textContent).toContain('26 天')
    expect(source).toContain('.project-gantt__detail-status--active')
    expect(source).toContain('.project-gantt__detail-stat--active')

    wrapper.unmount()
    vi.useRealTimers()
  })

  it('uses a more compact stage hover card footprint for tighter anchor positioning', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectGanttView.vue'), 'utf8')

    expect(source).toContain('width: min(288px, calc(100vw - 32px));')
    expect(source).toContain('gap: 10px;')
    expect(source).toContain('padding: 14px;')
    expect(source).toContain('min-height: 68px;')
  })

  it('uses an opaque elevated background for the stage hover card so page content does not bleed through', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectGanttView.vue'), 'utf8')

    expect(source).toContain(
      'background: linear-gradient(160deg, color-mix(in srgb, var(--accent-end) 18%, var(--panel-bg)), color-mix(in srgb, var(--accent-start) 24%, var(--panel-bg)));',
    )
    expect(source).toContain('0 0 0 1px color-mix(in srgb, var(--accent-line) 58%, var(--accent-end)),')
  })

  it('uses resolved app background surfaces for stage detail cards and keeps active status distinct from the pale blue panel', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectGanttView.vue'), 'utf8')

    expect(source).not.toContain('var(--bg-main)')
    expect(source).toContain(
      'background: linear-gradient(180deg, color-mix(in srgb, var(--panel-bg) 92%, var(--app-bg)), color-mix(in srgb, var(--accent-end) 10%, var(--panel-bg)));',
    )
    expect(source).toContain('background: var(--work-status-active-bg);')
    expect(source).toContain('color: var(--work-status-active-color);')
  })

  it('uses shared work-status tokens for gantt pills and bars instead of gradients or neutral completed states', () => {
    const projectSource = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectGanttView.vue'), 'utf8')
    const nodeSource = readFileSync(resolve(process.cwd(), 'src/components/workspace/NodeGanttDialog.vue'), 'utf8')
    const memberSource = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectMemberGanttView.vue'), 'utf8')

    expect(projectSource).toContain('background: var(--work-status-active-strong);')
    expect(projectSource).toContain('background: var(--work-status-done-strong);')
    expect(projectSource).toContain('color: var(--work-status-done-color);')
    expect(nodeSource).toContain('background: var(--work-status-active-strong);')
    expect(nodeSource).toContain('background: var(--work-status-done-strong);')
    expect(memberSource).toContain('background: var(--work-status-done-strong);')
    expect(projectSource).not.toContain(
      ".project-gantt__bar--active {\n  background: linear-gradient(135deg, var(--accent-start), var(--accent-end));",
    )
    expect(nodeSource).not.toContain(
      ".node-gantt-dialog__bar--active {\n  background: linear-gradient(135deg, var(--accent-start), var(--accent-end));",
    )
    expect(memberSource).not.toContain(
      ".project-member-gantt__bar--active {\n  background: linear-gradient(135deg, var(--accent-start), var(--accent-end));",
    )
  })

  it('renders stage bars as the heavier primary layer while keeping subtask bars slimmer and visually recessed', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectGanttView.vue'), 'utf8')

    expect(source).toContain('.project-gantt__bar--stage {')
    expect(source).toContain('height: 40px;')
    expect(source).toContain('padding-inline: 22px;')
    expect(source).toContain('0 18px 32px color-mix(in srgb, var(--accent-end) 18%, transparent);')
    expect(source).toContain('.project-gantt__bar--subtask {')
    expect(source).toContain('height: 28px;')
    expect(source).toContain('border: 1px solid transparent;')
    expect(source).toContain('.project-gantt__bar--subtask.project-gantt__bar--active {')
    expect(source).toContain('background: var(--work-status-active-bg);')
    expect(source).toContain('color: var(--work-status-active-color);')
  })

  it('keeps stage rows at the original height while rendering expanded subtask rows in a narrower lane', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectGanttView.vue'), 'utf8')

    expect(source).toContain('--project-gantt-row-height: 62px;')
    expect(source).toContain('--project-gantt-subtask-row-height: 46px;')
    expect(source).toContain('.project-gantt__sidebar-row--subtask,')
    expect(source).toContain('height: var(--project-gantt-subtask-row-height);')
    expect(source).toContain('.project-gantt__row--subtask,')
    expect(source).not.toContain('contain-intrinsic-size: 46px;')
  })

  it('anchors the stage detail card to the hover point and flips placement near viewport edges', async () => {
    vi.useFakeTimers()

    const wrapper = mount(ProjectGanttView, {
      attachTo: document.body,
      props: {
        gantt: sampleProjectGantt,
        scale: 'week',
      },
    })

    await wrapper.get('[data-testid="project-gantt-stage-bar-2002"]').trigger('mouseenter', {
      clientX: 320,
      clientY: 260,
    })
    await vi.advanceTimersByTimeAsync(1000)

    let detailCard = document.body.querySelector('[data-testid="project-gantt-detail-card"]') as HTMLElement | null

    expect(detailCard).not.toBeNull()
    expect(detailCard?.style.left).toBe('320px')
    expect(detailCard?.style.top).toBe('260px')
    expect(detailCard?.dataset.placement).toBe('top-right')

    await wrapper.get('[data-testid="project-gantt-stage-bar-2002"]').trigger('mouseenter', {
      clientX: 980,
      clientY: 60,
    })
    await vi.advanceTimersByTimeAsync(1000)

    detailCard = document.body.querySelector('[data-testid="project-gantt-detail-card"]') as HTMLElement | null
    expect(detailCard?.dataset.placement).toBe('bottom-left')

    wrapper.unmount()
    vi.useRealTimers()
  })

  it('keeps the fixed stage detail card visible while moving from the bar into the card', async () => {
    vi.useFakeTimers()

    const wrapper = mount(ProjectGanttView, {
      attachTo: document.body,
      props: {
        gantt: sampleProjectGantt,
        scale: 'week',
      },
    })

    await wrapper.get('[data-testid="project-gantt-stage-bar-2002"]').trigger('mouseenter')
    expect(document.body.querySelector('[data-testid="project-gantt-detail-card"]')).toBeNull()
    await vi.advanceTimersByTimeAsync(999)
    expect(document.body.querySelector('[data-testid="project-gantt-detail-card"]')).toBeNull()
    await vi.advanceTimersByTimeAsync(1)

    const detailCard = document.body.querySelector('[data-testid="project-gantt-detail-card"]')
    expect(detailCard).not.toBeNull()

    await wrapper.get('[data-testid="project-gantt-stage-bar-2002"]').trigger('mouseleave')
    expect(document.body.querySelector('[data-testid="project-gantt-detail-card"]')).not.toBeNull()

    detailCard?.dispatchEvent(new MouseEvent('mouseenter', { bubbles: true }))
    await vi.advanceTimersByTimeAsync(240)
    expect(document.body.querySelector('[data-testid="project-gantt-detail-card"]')).not.toBeNull()

    detailCard?.dispatchEvent(new MouseEvent('mouseleave', { bubbles: true }))
    await vi.advanceTimersByTimeAsync(240)
    expect(document.body.querySelector('[data-testid="project-gantt-detail-card"]')).toBeNull()

    wrapper.unmount()
    vi.useRealTimers()
  })

  it('uses a 1000ms delayed hover before showing stage detail cards so resize intent does not immediately conflict', async () => {
    vi.useFakeTimers()

    const wrapper = mount(ProjectGanttView, {
      attachTo: document.body,
      props: {
        gantt: sampleProjectGantt,
        scale: 'week',
      },
    })

    await wrapper.get('[data-testid="project-gantt-stage-bar-2002"]').trigger('mouseenter')
    expect(document.body.querySelector('[data-testid="project-gantt-detail-card"]')).toBeNull()

    await vi.advanceTimersByTimeAsync(999)
    expect(document.body.querySelector('[data-testid="project-gantt-detail-card"]')).toBeNull()

    await vi.advanceTimersByTimeAsync(1)
    expect(document.body.querySelector('[data-testid="project-gantt-detail-card"]')).not.toBeNull()

    wrapper.unmount()
    vi.useRealTimers()
  })

  it('renders fixed stage labels and a matching time axis', () => {
    const wrapper = mount(ProjectGanttView, {
      props: {
        gantt: sampleProjectGantt,
        expandedNodeIds: [],
        loadingNodeIds: [],
        nodeLoadErrors: {},
        nodeSubtasksById: {},
        scale: 'week',
      },
    })

    expect(wrapper.get('[data-testid="project-gantt-view"]').exists()).toBe(true)
    expect(wrapper.get('[data-testid="project-gantt-axis"]').findAll('.project-gantt__axis-cell').length).toBe(15)
    expect(wrapper.get('[data-testid="project-gantt-row-label-2001"]').text()).toContain('需求分析')
    expect(wrapper.get('[data-testid="project-gantt-row-label-2002"]').text()).toContain('前端开发')
  })

  it('maps stage bars to semantic status classes', () => {
    const wrapper = mount(ProjectGanttView, {
      props: {
        gantt: sampleProjectGantt,
        expandedNodeIds: [],
        loadingNodeIds: [],
        nodeLoadErrors: {},
        nodeSubtasksById: {},
        scale: 'week',
      },
    })

    expect(wrapper.get('[data-testid="project-gantt-stage-bar-2001"]').classes()).toContain('project-gantt__bar--done')
    expect(wrapper.get('[data-testid="project-gantt-stage-bar-2002"]').classes()).toContain('project-gantt__bar--active')
  })

  it('emits scale changes from the gantt toolbar switcher', async () => {
    const wrapper = mount(ProjectGanttView, {
      props: {
        gantt: sampleProjectGantt,
        expandedNodeIds: [],
        loadingNodeIds: [],
        nodeLoadErrors: {},
        nodeSubtasksById: {},
        scale: 'week',
      },
    })

    await wrapper.get('[data-testid="gantt-scale-day"]').trigger('click')

    expect(wrapper.emitted('update:scale')).toEqual([['day']])
  })

  it('keeps fixed padded day labels readable and renders the trailing week cell as a full week', () => {
    const dayWrapper = mount(ProjectGanttView, {
      props: {
        gantt: sampleProjectGantt,
        scale: 'day',
      },
    })
    const weekWrapper = mount(ProjectGanttView, {
      props: {
        gantt: sampleProjectGantt,
        scale: 'week',
      },
    })
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectGanttView.vue'), 'utf8')
    const dayAxisCell = dayWrapper.get('[data-testid="project-gantt-axis"]').find('.project-gantt__axis-cell')
    const weekAxisCells = weekWrapper.get('[data-testid="project-gantt-axis"]').findAll('.project-gantt__axis-cell')
    const trailingWeekCell = weekAxisCells.at(-1)

    expect(dayAxisCell.text()).toBe('03/20')
    expect(dayAxisCell.classes()).toContain('project-gantt__axis-cell--day')
    expect(trailingWeekCell?.text()).toBe('06/26')
    expect(trailingWeekCell?.attributes('style')).toContain('width: 84px;')
    expect(source).toContain('.project-gantt__axis-cell--day {')
    expect(source).toContain('font-variant-numeric: tabular-nums;')
    expect(source).toContain('background: var(--dialog-control-bg), var(--card-sheen);')
    expect(source).toContain('color-mix(in srgb, var(--accent-end) 10%, var(--dialog-control-bg))')
    expect(source).toContain('color-mix(in srgb, var(--accent-start) 8%, var(--dialog-control-bg))')
    expect(source).not.toContain('.project-gantt__axis-cell--tail {')
    expect(source).not.toContain('padding-right: 28px;')
  })

  it('uses the shared axis as the only time coordinate layer so stage rows do not render a per-row track grid', () => {
    const wrapper = mount(ProjectGanttView, {
      props: {
        gantt: sampleProjectGantt,
        expandedNodeIds: [],
        loadingNodeIds: [],
        nodeLoadErrors: {},
        nodeSubtasksById: {},
        scale: 'week',
      },
    })
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectGanttView.vue'), 'utf8')
    const axisCells = wrapper.get('[data-testid="project-gantt-axis"]').findAll('.project-gantt__axis-cell')

    expect(axisCells.length).toBeGreaterThan(0)
    expect(wrapper.find('[data-testid="project-gantt-track-grid-2001"]').exists()).toBe(false)
    expect(source).not.toContain('project-gantt__track-grid')
    expect(source).not.toContain('project-gantt__track-cell')
  })

  it('keeps desktop vertical scrolling inside the right stage timeline host so the shared headers stay visible', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectGanttView.vue'), 'utf8')

    expect(source).toContain('.project-gantt__body-scroll {')
    expect(source).toContain('max-height: 80vh;')
    expect(source).toContain('overflow: hidden;')
    expect(source).toContain('class="project-gantt__body-scroll smooth-scroll-surface"')
    expect(source).not.toContain(`v-smooth-wheel="{ axis: 'horizontal', wheelBehavior: 'block' }"`)
    expect(source).toContain('project-gantt__sidebar-scroll')
    expect(source).toContain('.project-gantt__sidebar-scroll {')
    expect(source).toContain('.project-gantt__top-scroll {')
    expect(source).toContain('position: sticky;')
    expect(source).toContain('top: 0;')
    expect(source).toContain('.project-gantt__axis-scroll {')
    expect(source).toContain('top: var(--project-gantt-top-scroll-height);')
    expect(source).toContain('.project-gantt__timeline-scroll {')
    expect(source).toContain('overflow-y: auto;')
  })

  it('uses native horizontal scrolling that stays in the timeline region instead of stretching under the stage sidebar', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectGanttView.vue'), 'utf8')

    expect(source).toContain('data-testid="project-gantt-axis-scroll"')
    expect(source).toContain('project-gantt__top-scroll')
    expect(source).toContain('.project-gantt__top-scroll {')
    expect(source).toContain('overflow-x: auto;')
    expect(source).toContain('.project-gantt__timeline-scroll {')
    expect(source).toContain('overflow-x: auto;')
    expect(source).toContain('.project-gantt__top-scroll::-webkit-scrollbar {')
    expect(source).toContain('height: var(--project-gantt-top-scroll-height);')
    expect(source).not.toContain('scrollbar-width: none;')
  })

  it('visually merges the stage sidebar and timeline into one shared shell with only a center divider', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectGanttView.vue'), 'utf8')

    expect(source).toContain('.project-gantt__body {')
    expect(source).toContain('gap: 0;')
    expect(source).toContain('border: 1px solid var(--meta-surface-border);')
    expect(source).toContain('border-radius: 22px;')
    expect(source).toContain('.project-gantt__sidebar,\n.project-gantt__timeline-shell {')
    expect(source).toContain('border: 0;')
    expect(source).toContain('border-radius: 0;')
    expect(source).toContain('background: transparent;')
    expect(source).toContain('.project-gantt__sidebar {')
    expect(source).toContain('border-right: 1px solid var(--meta-surface-border);')
  })

  it('keeps the left stage header stack aligned with the right top scrollbar plus axis header heights', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectGanttView.vue'), 'utf8')

    expect(source).toContain('--project-gantt-top-scroll-height: 18px;')
    expect(source).toContain('project-gantt__sidebar-top-spacer')
    expect(source).toContain('.project-gantt__sidebar-top-spacer {')
    expect(source).toContain('height: var(--project-gantt-top-scroll-height);')
    expect(source).toContain('.project-gantt__top-scroll {')
    expect(source).toContain('height: var(--project-gantt-top-scroll-height);')
    expect(source).toContain('.project-gantt__sidebar-head {')
    expect(source).toContain('top: var(--project-gantt-top-scroll-height);')
    expect(source).toContain('.project-gantt__axis-scroll {')
    expect(source).toContain('top: var(--project-gantt-top-scroll-height);')
  })

  it('keeps the left stage rows visually synced when the right timeline scrolls vertically', () => {
    const wrapper = mount(ProjectGanttView, {
      props: {
        gantt: sampleProjectGantt,
        expandedNodeIds: [],
        loadingNodeIds: [],
        nodeLoadErrors: {},
        nodeSubtasksById: {},
        scale: 'week',
      },
    })

    const sidebarRail = wrapper.get('.project-gantt__sidebar-rows-rail').element as HTMLElement
    const rowsScroll = wrapper.get('[data-testid="project-gantt-rows-scroll"]').element as HTMLElement

    rowsScroll.scrollTop = 96
    rowsScroll.dispatchEvent(new Event('scroll'))

    expect(sidebarRail.style.transform).toBe('translateY(-96px)')

    wrapper.unmount()
  })

  it('keeps the sticky stage axis free of backdrop blur and avoids heavy row visibility hints that make scrolling feel sticky', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectGanttView.vue'), 'utf8')
    const axisBlock = source.match(/\.project-gantt__axis \{[\s\S]*?\n\}/)?.[0]
    const rowBlock = source.match(/\.project-gantt__row \{[\s\S]*?\n\}/)?.[0]
    const sidebarRowBlocks = [...source.matchAll(/\.project-gantt__sidebar-row \{[\s\S]*?\n\}/g)].map((match) => match[0])

    expect(axisBlock).toBeTruthy()
    expect(axisBlock).not.toContain('backdrop-filter')
    expect(rowBlock).toBeTruthy()
    expect(rowBlock).toContain('contain: layout paint;')
    expect(rowBlock).not.toContain('content-visibility: auto;')
    expect(rowBlock).not.toContain('contain-intrinsic-size:')
    expect(sidebarRowBlocks.length).toBeGreaterThan(0)
    expect(sidebarRowBlocks.some((block) => block.includes('content-visibility: auto;'))).toBe(false)
    expect(sidebarRowBlocks.some((block) => block.includes('contain-intrinsic-size:'))).toBe(false)
  })

  it('uses galaxy meta surfaces for the project and member gantt workspace shells instead of the old frosted glass wrappers', () => {
    const projectSource = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectGanttView.vue'), 'utf8')
    const memberSource = readFileSync(
      resolve(process.cwd(), 'src/components/workspace/ProjectMemberGanttView.vue'),
      'utf8',
    )

    expect(projectSource).toContain('background: var(--meta-surface-bg), var(--meta-surface-glow), var(--card-sheen);')
    expect(projectSource).toContain('box-shadow: var(--meta-surface-shadow);')
    expect(projectSource).not.toContain('backdrop-filter: var(--backdrop-blur);')
    expect(memberSource).toContain('background: var(--meta-surface-bg), var(--meta-surface-glow), var(--card-sheen);')
    expect(memberSource).toContain('box-shadow: var(--meta-surface-shadow);')
    expect(memberSource).not.toContain('backdrop-filter: var(--backdrop-blur);')
  })

  it('keeps the sticky stage time axis horizontally synced with the rows scroller', () => {
    const wrapper = mount(ProjectGanttView, {
      props: {
        gantt: sampleProjectGantt,
        expandedNodeIds: [],
        loadingNodeIds: [],
        nodeLoadErrors: {},
        nodeSubtasksById: {},
        scale: 'week',
      },
    })

    const axisScroll = wrapper.get('[data-testid="project-gantt-axis-scroll"]').element as HTMLElement
    const rowsScroll = wrapper.get('[data-testid="project-gantt-rows-scroll"]').element as HTMLElement

    rowsScroll.scrollLeft = 120
    rowsScroll.dispatchEvent(new Event('scroll'))
    expect(axisScroll.scrollLeft).toBe(120)

    axisScroll.scrollLeft = 84
    axisScroll.dispatchEvent(new Event('scroll'))
    expect(rowsScroll.scrollLeft).toBe(84)

    wrapper.unmount()
  })

  it('ignores mirrored stage gantt scroll events so horizontal sync does not bounce back', () => {
    const wrapper = mount(ProjectGanttView, {
      props: {
        gantt: sampleProjectGantt,
        expandedNodeIds: [],
        loadingNodeIds: [],
        nodeLoadErrors: {},
        nodeSubtasksById: {},
        scale: 'week',
      },
    })

    const axisScroll = wrapper.get('[data-testid="project-gantt-axis-scroll"]').element as HTMLElement
    const rowsScroll = wrapper.get('[data-testid="project-gantt-rows-scroll"]').element as HTMLElement

    axisScroll.scrollLeft = 0
    rowsScroll.scrollLeft = 48
    rowsScroll.dispatchEvent(new Event('scroll'))
    expect(axisScroll.scrollLeft).toBe(48)

    axisScroll.dispatchEvent(new Event('scroll'))
    expect(rowsScroll.scrollLeft).toBe(48)

    axisScroll.scrollLeft = 24
    axisScroll.dispatchEvent(new Event('scroll'))
    expect(rowsScroll.scrollLeft).toBe(24)

    wrapper.unmount()
  })

  it('does not lose stage gantt sync after a mirrored rows scroll event followed by another fast axis drag', () => {
    const wrapper = mount(ProjectGanttView, {
      props: {
        gantt: sampleProjectGantt,
        expandedNodeIds: [],
        loadingNodeIds: [],
        nodeLoadErrors: {},
        nodeSubtasksById: {},
        scale: 'week',
      },
    })

    const axisScroll = wrapper.get('[data-testid="project-gantt-axis-scroll"]').element as HTMLElement
    const rowsScroll = wrapper.get('[data-testid="project-gantt-rows-scroll"]').element as HTMLElement

    axisScroll.scrollLeft = 156
    axisScroll.dispatchEvent(new Event('scroll'))
    expect(rowsScroll.scrollLeft).toBe(156)

    rowsScroll.dispatchEvent(new Event('scroll'))

    axisScroll.scrollLeft = 0
    axisScroll.dispatchEvent(new Event('scroll'))
    expect(rowsScroll.scrollLeft).toBe(0)

    wrapper.unmount()
  })

  it('renders subtask bars inline beneath expanded stages while keeping the same shared time axis', () => {
    const wrapper = mount(ProjectGanttView, {
      props: {
        gantt: sampleProjectGantt,
        expandedNodeIds: [2002],
        loadingNodeIds: [],
        nodeLoadErrors: {},
        nodeSubtasksById: {
          2002: sampleNodeGantt.subtasks,
        },
        scale: 'week',
      },
    })

    expect(wrapper.get('[data-testid="project-gantt-subtask-bar-3001"]').text()).toContain('完成登录接口开发')
    expect(wrapper.find('[data-testid="project-gantt-subtask-track-grid-3001"]').exists()).toBe(false)
  })

  it('emits the target node id when a stage bar is clicked so the parent can toggle subtree expansion', async () => {
    const wrapper = mount(ProjectGanttView, {
      props: {
        gantt: sampleProjectGantt,
        expandedNodeIds: [],
        loadingNodeIds: [],
        nodeLoadErrors: {},
        nodeSubtasksById: {},
        scale: 'week',
      },
    })

    await wrapper.get('[data-testid="project-gantt-stage-bar-2002"]').trigger('click')

    expect(wrapper.emitted('toggle-node')).toEqual([[2002]])
  })

  it('emits resized stage dates from the handle without toggling subtree expansion', async () => {
    const wrapper = mount(ProjectGanttView, {
      props: {
        canEditSchedule: true,
        gantt: sampleProjectGantt,
        expandedNodeIds: [],
        loadingNodeIds: [],
        nodeLoadErrors: {},
        nodeSubtasksById: {},
        scale: 'week',
      },
    })

    await wrapper.get('[data-testid="project-gantt-stage-resize-end-2002"]').trigger('mousedown', {
      button: 0,
      clientX: 240,
    })

    window.dispatchEvent(new MouseEvent('mousemove', {
      bubbles: true,
      buttons: 1,
      clientX: 264,
    }))
    window.dispatchEvent(new MouseEvent('mouseup', {
      bubbles: true,
      clientX: 264,
    }))

    expect(wrapper.emitted('resize-node')).toEqual([[
      {
        nodeId: 2002,
        plannedStartDate: '2026-03-26',
        plannedEndDate: '2026-04-22',
      },
    ]])
    expect(wrapper.emitted('toggle-node')).toBeUndefined()
  })

  it('emits resized subtask dates from the handle using day-level snapping even when the gantt is in week scale', async () => {
    const wrapper = mount(ProjectGanttView, {
      props: {
        canEditSchedule: true,
        gantt: sampleProjectGantt,
        expandedNodeIds: [2002],
        loadingNodeIds: [],
        nodeLoadErrors: {},
        nodeSubtasksById: {
          2002: sampleNodeGantt.subtasks,
        },
        scale: 'week',
      },
    })

    await wrapper.get('[data-testid="project-gantt-subtask-resize-start-3001"]').trigger('mousedown', {
      button: 0,
      clientX: 180,
    })

    window.dispatchEvent(new MouseEvent('mousemove', {
      bubbles: true,
      buttons: 1,
      clientX: 156,
    }))
    window.dispatchEvent(new MouseEvent('mouseup', {
      bubbles: true,
      clientX: 156,
    }))

    expect(wrapper.emitted('resize-subtask')).toEqual([[
      {
        nodeId: 2002,
        subtaskId: 3001,
        plannedStartDate: '2026-03-26',
        plannedEndDate: '2026-03-31',
      },
    ]])
  })

  it('emits expand-all and collapse-all actions from the toolbar controls', async () => {
    const wrapper = mount(ProjectGanttView, {
      props: {
        gantt: sampleProjectGantt,
        expandedNodeIds: [],
        loadingNodeIds: [],
        nodeLoadErrors: {},
        nodeSubtasksById: {},
        scale: 'week',
      },
    })

    await wrapper.get('[data-testid="project-gantt-expand-all"]').trigger('click')
    await wrapper.get('[data-testid="project-gantt-collapse-all"]').trigger('click')

    expect(wrapper.emitted('expand-all')).toEqual([[]])
    expect(wrapper.emitted('collapse-all')).toEqual([[]])
  })

  it('keeps expand and collapse controls beside the shared scale switcher and removes zoom buttons from the scale UI', () => {
    const projectSource = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectGanttView.vue'), 'utf8')
    const scaleSource = readFileSync(resolve(process.cwd(), 'src/components/workspace/GanttScaleSwitcher.vue'), 'utf8')

    expect(projectSource).toContain('class="project-gantt__toolbar-scale-row"')
    expect(projectSource).toContain('<GanttScaleSwitcher :scale="scale" @update:scale="emit(\'update:scale\', $event)" />')
    expect(projectSource).toContain('<div class="project-gantt__tree-actions">')
    expect(projectSource).toContain('.project-gantt__toolbar-scale-row {')
    expect(scaleSource).not.toContain('data-testid="gantt-scale-zoom-in"')
    expect(scaleSource).not.toContain('data-testid="gantt-scale-zoom-out"')
    expect(scaleSource).not.toContain('aria-label="放大时间粒度"')
    expect(scaleSource).not.toContain('aria-label="缩小时间粒度"')
    expect(scaleSource).not.toContain('.gantt-scale-switcher__zoom')
  })

  it('adds dedicated resize handles and a delayed hover constant to separate edit intent from detail-card hover intent', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectGanttView.vue'), 'utf8')

    expect(source).toContain('const DETAIL_SHOW_DELAY_MS = 1000')
    expect(source).toContain('project-gantt__resize-handle')
    expect(source).toContain("emit('resize-node'")
    expect(source).toContain("emit('resize-subtask'")
  })
})

describe('NodeGanttDialog', () => {
  it('keeps the left subtask label and status on one line with ellipsis instead of wrapping', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/NodeGanttDialog.vue'), 'utf8')

    expect(source).toContain('.node-gantt-dialog__sidebar-row {')
    expect(source).toContain('overflow: hidden;')
    expect(source).toContain('.node-gantt-dialog__status-pill {')
    expect(source).toContain('flex-shrink: 0;')
    expect(source).toContain('white-space: nowrap;')
  })

  it('keeps subtask hover detail cards out of normal layout flow so hover stays stable', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/NodeGanttDialog.vue'), 'utf8')

    expect(source).toContain('<Teleport to="body">')
    expect(source).toContain('.node-gantt-dialog__detail-card {')
    expect(source).toContain('position: fixed;')
    expect(source).toContain(":data-placement=\"hoveredSubtaskPlacement\"")
    expect(source).toContain(':style="detailCardStyle"')
    expect(source).toContain('max-height: calc(100vh - 48px);')
    expect(source).toContain('pointer-events: auto;')
    expect(source).toContain(".node-gantt-dialog__detail-card[data-placement='top-right'] {")
    expect(source).not.toContain('right: 24px;')
    expect(source).not.toContain('bottom: 24px;')
    expect(source).not.toContain('translateY(calc(-50% - 1px))')
  })

  it('renders a color-coded subtask hover detail card with priority and progress emphasis', async () => {
    const wrapper = mount(NodeGanttDialog, {
      attachTo: document.body,
      props: {
        modelValue: true,
        gantt: sampleNodeGantt,
        scale: 'week',
      },
    })

    await wrapper.get('[data-testid="node-gantt-bar-3001"]').trigger('mouseenter')

    const detailCard = document.body.querySelector('[data-testid="node-gantt-detail-card"]')
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/NodeGanttDialog.vue'), 'utf8')

    expect(detailCard).not.toBeNull()
    expect(detailCard?.querySelector('.node-gantt-dialog__detail-status')?.className).toContain(
      'node-gantt-dialog__detail-status--active',
    )
    expect(detailCard?.querySelector('.node-gantt-dialog__detail-priority')?.className).toContain(
      'node-gantt-dialog__detail-priority--medium',
    )
    expect(
      detailCard?.querySelector('[data-testid="node-gantt-detail-progress-fill"]')?.getAttribute('style'),
    ).toContain(
      'width: 60%;',
    )
    expect(source).toContain('.node-gantt-dialog__detail-progress-track')
    expect(source).toContain('.node-gantt-dialog__detail-priority--medium')

    wrapper.unmount()
  })

  it('uses a more compact subtask hover card footprint for tighter anchor positioning', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/NodeGanttDialog.vue'), 'utf8')

    expect(source).toContain('width: min(288px, calc(100vw - 32px));')
    expect(source).toContain('gap: 10px;')
    expect(source).toContain('padding: 14px;')
    expect(source).toContain('min-height: 68px;')
    expect(source).toContain('height: 8px;')
  })

  it('uses an opaque elevated background for the subtask hover card so page content does not bleed through', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/NodeGanttDialog.vue'), 'utf8')

    expect(source).toContain(
      'background: linear-gradient(160deg, color-mix(in srgb, var(--accent-end) 18%, var(--panel-bg)), color-mix(in srgb, var(--accent-start) 24%, var(--panel-bg)));',
    )
    expect(source).toContain('0 0 0 1px color-mix(in srgb, var(--accent-line) 58%, var(--accent-end)),')
  })

  it('uses resolved app background surfaces for subtask detail cards and keeps active status distinct from the pale blue panel', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/NodeGanttDialog.vue'), 'utf8')

    expect(source).not.toContain('var(--bg-main)')
    expect(source).toContain(
      'background: linear-gradient(180deg, color-mix(in srgb, var(--panel-bg) 92%, var(--app-bg)), color-mix(in srgb, var(--accent-end) 10%, var(--panel-bg)));',
    )
    expect(source).toContain('background: var(--work-status-active-bg);')
    expect(source).toContain('color: var(--work-status-active-color);')
  })

  it('uses the dialog surface palette for the main subtask gantt panel instead of the neutral glass background', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/NodeGanttDialog.vue'), 'utf8')

    expect(source).toContain('border: 1px solid var(--dialog-surface-border);')
    expect(source).toContain('background: var(--dialog-surface-bg), var(--dialog-surface-glow), var(--card-sheen);')
    expect(source).toContain('box-shadow: var(--dialog-surface-shadow);')
    expect(source).toContain('background: var(--dialog-control-bg);')
    expect(source).toContain('border: 1px solid var(--dialog-control-border);')
  })

  it('keeps only owner and schedule cards in the subtask detail grid instead of repeating priority as another card', async () => {
    const wrapper = mount(NodeGanttDialog, {
      attachTo: document.body,
      props: {
        modelValue: true,
        gantt: sampleNodeGantt,
        scale: 'week',
      },
    })

    await wrapper.get('[data-testid="node-gantt-bar-3001"]').trigger('mouseenter')

    const detailCard = document.body.querySelector('[data-testid="node-gantt-detail-card"]')
    const priorityMentions = detailCard?.textContent?.match(/优先级/g) ?? []

    expect(detailCard?.querySelectorAll('.node-gantt-dialog__detail-grid-item')).toHaveLength(2)
    expect(priorityMentions).toHaveLength(1)

    wrapper.unmount()
  })

  it('anchors the subtask detail card to the hover point and flips placement near viewport edges', async () => {
    const wrapper = mount(NodeGanttDialog, {
      attachTo: document.body,
      props: {
        modelValue: true,
        gantt: sampleNodeGantt,
        scale: 'week',
      },
    })

    await wrapper.get('[data-testid="node-gantt-bar-3001"]').trigger('mouseenter', {
      clientX: 340,
      clientY: 280,
    })

    let detailCard = document.body.querySelector('[data-testid="node-gantt-detail-card"]') as HTMLElement | null

    expect(detailCard).not.toBeNull()
    expect(detailCard?.style.left).toBe('340px')
    expect(detailCard?.style.top).toBe('280px')
    expect(detailCard?.dataset.placement).toBe('top-right')

    await wrapper.get('[data-testid="node-gantt-bar-3001"]').trigger('mouseenter', {
      clientX: 980,
      clientY: 72,
    })

    detailCard = document.body.querySelector('[data-testid="node-gantt-detail-card"]') as HTMLElement | null
    expect(detailCard?.dataset.placement).toBe('bottom-left')

    wrapper.unmount()
  })

  it('keeps the fixed subtask detail card visible while moving from the bar into the card', async () => {
    vi.useFakeTimers()

    const wrapper = mount(NodeGanttDialog, {
      attachTo: document.body,
      props: {
        modelValue: true,
        gantt: sampleNodeGantt,
        scale: 'week',
      },
    })

    await wrapper.get('[data-testid="node-gantt-bar-3001"]').trigger('mouseenter')

    const detailCard = document.body.querySelector('[data-testid="node-gantt-detail-card"]')
    expect(detailCard).not.toBeNull()

    await wrapper.get('[data-testid="node-gantt-bar-3001"]').trigger('mouseleave')
    expect(document.body.querySelector('[data-testid="node-gantt-detail-card"]')).not.toBeNull()

    detailCard?.dispatchEvent(new MouseEvent('mouseenter', { bubbles: true }))
    await vi.advanceTimersByTimeAsync(240)
    expect(document.body.querySelector('[data-testid="node-gantt-detail-card"]')).not.toBeNull()

    detailCard?.dispatchEvent(new MouseEvent('mouseleave', { bubbles: true }))
    await vi.advanceTimersByTimeAsync(240)
    expect(document.body.querySelector('[data-testid="node-gantt-detail-card"]')).toBeNull()

    wrapper.unmount()
    vi.useRealTimers()
  })

  it('renders a floating subtask gantt timeline for the selected node', () => {
    const wrapper = mount(NodeGanttDialog, {
      props: {
        modelValue: true,
        gantt: sampleNodeGantt,
        scale: 'week',
      },
    })

    expect(wrapper.get('[data-testid="node-gantt-dialog"]').exists()).toBe(true)
    expect(wrapper.get('[data-testid="node-gantt-axis"]').findAll('.node-gantt-dialog__axis-cell').length).toBeGreaterThan(0)
    expect(wrapper.get('[data-testid="node-gantt-row-label-3001"]').text()).toContain('完成登录接口开发')
    expect(wrapper.get('[data-testid="node-gantt-bar-3001"]').exists()).toBe(true)
  })

  it('keeps fixed padded day labels readable and renders the trailing week cell as a full week in the subtask gantt dialog', () => {
    const dayWrapper = mount(NodeGanttDialog, {
      props: {
        modelValue: true,
        gantt: sampleNodeGantt,
        scale: 'day',
      },
    })
    const weekWrapper = mount(NodeGanttDialog, {
      props: {
        modelValue: true,
        gantt: sampleNodeGantt,
        scale: 'week',
      },
    })
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/NodeGanttDialog.vue'), 'utf8')
    const dayAxisCell = dayWrapper.get('[data-testid="node-gantt-axis"]').find('.node-gantt-dialog__axis-cell')
    const weekAxisCells = weekWrapper.get('[data-testid="node-gantt-axis"]').findAll('.node-gantt-dialog__axis-cell')
    const trailingWeekCell = weekAxisCells.at(-1)

    expect(dayAxisCell.text()).toBe('03/26')
    expect(dayAxisCell.classes()).toContain('node-gantt-dialog__axis-cell--day')
    expect(trailingWeekCell?.text()).toBe('04/16')
    expect(trailingWeekCell?.attributes('style')).toContain('width: 84px;')
    expect(source).toContain('.node-gantt-dialog__axis-cell--day {')
    expect(source).toContain('font-variant-numeric: tabular-nums;')
    expect(source).toContain(
      'background: linear-gradient(180deg, color-mix(in srgb, var(--accent-end) 12%, var(--dialog-control-bg-strong)), color-mix(in srgb, var(--accent-start) 6%, var(--dialog-control-bg)));',
    )
    expect(source).not.toContain('.node-gantt-dialog__axis-cell--tail {')
    expect(source).not.toContain('padding-right: 28px;')
  })

  it('renders subtask track cells that align with the axis instead of fixed stripe dividers', () => {
    const wrapper = mount(NodeGanttDialog, {
      props: {
        modelValue: true,
        gantt: sampleNodeGantt,
        scale: 'week',
      },
    })
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/NodeGanttDialog.vue'), 'utf8')
    const axisCells = wrapper.get('[data-testid="node-gantt-axis"]').findAll('.node-gantt-dialog__axis-cell')
    const trackCells = wrapper.get('[data-testid="node-gantt-track-grid-3001"]').findAll('.node-gantt-dialog__track-cell')

    expect(trackCells).toHaveLength(axisCells.length)
    expect(trackCells[0]?.attributes('style')).toBe(axisCells[0]?.attributes('style'))
    expect(source).toContain('.node-gantt-dialog__track-grid {')
    expect(source).toContain('.node-gantt-dialog__track-cell {')
    expect(source).not.toContain('repeating-linear-gradient(90deg')
  })

  it('keeps desktop vertical scrolling inside the right subtask timeline host so the shared headers stay visible', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/NodeGanttDialog.vue'), 'utf8')

    expect(source).toContain('.node-gantt-dialog {')
    expect(source).toContain('max-height: min(80vh, calc(100vh - 32px));')
    expect(source).toContain('.node-gantt-dialog__body-scroll {')
    expect(source).toContain('overflow: hidden;')
    expect(source).toContain('class="node-gantt-dialog__body-scroll smooth-scroll-surface"')
    expect(source).not.toContain(`v-smooth-wheel="{ axis: 'horizontal', wheelBehavior: 'block' }"`)
    expect(source).toContain('node-gantt-dialog__sidebar-scroll')
    expect(source).toContain('.node-gantt-dialog__sidebar-scroll {')
    expect(source).toContain('.node-gantt-dialog__top-scroll {')
    expect(source).toContain('position: sticky;')
    expect(source).toContain('top: 0;')
    expect(source).toContain('.node-gantt-dialog__axis-scroll {')
    expect(source).toContain('top: var(--node-gantt-top-scroll-height);')
    expect(source).toContain('.node-gantt-dialog__timeline-scroll {')
    expect(source).toContain('overflow-y: auto;')
  })

  it('uses native horizontal scrolling that stays in the subtask timeline region instead of stretching under the sidebar', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/NodeGanttDialog.vue'), 'utf8')

    expect(source).toContain('data-testid="node-gantt-axis-scroll"')
    expect(source).toContain('node-gantt-dialog__top-scroll')
    expect(source).toContain('.node-gantt-dialog__top-scroll {')
    expect(source).toContain('overflow-x: auto;')
    expect(source).toContain('.node-gantt-dialog__timeline-scroll {')
    expect(source).toContain('overflow-x: auto;')
    expect(source).toContain('.node-gantt-dialog__top-scroll::-webkit-scrollbar {')
    expect(source).toContain('height: var(--node-gantt-top-scroll-height);')
    expect(source).not.toContain('scrollbar-width: none;')
  })

  it('visually merges the subtask sidebar and timeline into one shared shell with only a center divider', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/NodeGanttDialog.vue'), 'utf8')

    expect(source).toContain('.node-gantt-dialog__body {')
    expect(source).toContain('gap: 0;')
    expect(source).toContain('border: 1px solid var(--dialog-control-border);')
    expect(source).toContain('border-radius: 22px;')
    expect(source).toContain('.node-gantt-dialog__sidebar,\n.node-gantt-dialog__timeline-shell {')
    expect(source).toContain('border: 0;')
    expect(source).toContain('border-radius: 0;')
    expect(source).toContain('background: transparent;')
    expect(source).toContain('.node-gantt-dialog__sidebar {')
    expect(source).toContain('border-right: 1px solid var(--dialog-control-border);')
  })

  it('keeps the left subtask header stack aligned with the right top scrollbar plus axis header heights', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/NodeGanttDialog.vue'), 'utf8')

    expect(source).toContain('--node-gantt-top-scroll-height: 18px;')
    expect(source).toContain('node-gantt-dialog__sidebar-top-spacer')
    expect(source).toContain('.node-gantt-dialog__sidebar-top-spacer {')
    expect(source).toContain('height: var(--node-gantt-top-scroll-height);')
    expect(source).toContain('.node-gantt-dialog__top-scroll {')
    expect(source).toContain('height: var(--node-gantt-top-scroll-height);')
    expect(source).toContain('.node-gantt-dialog__sidebar-head {')
    expect(source).toContain('top: var(--node-gantt-top-scroll-height);')
    expect(source).toContain('.node-gantt-dialog__axis-scroll {')
    expect(source).toContain('top: var(--node-gantt-top-scroll-height);')
  })

  it('keeps the left subtask rows visually synced when the right timeline scrolls vertically', () => {
    const wrapper = mount(NodeGanttDialog, {
      props: {
        modelValue: true,
        gantt: sampleNodeGantt,
        scale: 'week',
      },
    })

    const sidebarRail = wrapper.get('.node-gantt-dialog__sidebar-rows-rail').element as HTMLElement
    const rowsScroll = wrapper.get('[data-testid="node-gantt-rows-scroll"]').element as HTMLElement

    rowsScroll.scrollTop = 96
    rowsScroll.dispatchEvent(new Event('scroll'))

    expect(sidebarRail.style.transform).toBe('translateY(-96px)')

    wrapper.unmount()
  })

  it('keeps the sticky subtask axis free of backdrop blur and isolates each row paint to reduce scroll jank', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/NodeGanttDialog.vue'), 'utf8')
    const axisBlock = source.match(/\.node-gantt-dialog__axis \{[\s\S]*?\n\}/)?.[0]
    const rowBlock = source.match(/\.node-gantt-dialog__row \{[\s\S]*?\n\}/)?.[0]
    const sidebarRowBlocks = [...source.matchAll(/\.node-gantt-dialog__sidebar-row \{[\s\S]*?\n\}/g)].map((match) => match[0])

    expect(axisBlock).toBeTruthy()
    expect(axisBlock).not.toContain('backdrop-filter')
    expect(rowBlock).toBeTruthy()
    expect(rowBlock).toContain('contain: layout paint;')
    expect(rowBlock).toContain('content-visibility: auto;')
    expect(rowBlock).toContain('contain-intrinsic-size: 62px;')
    expect(sidebarRowBlocks.length).toBeGreaterThan(0)
    expect(sidebarRowBlocks.some((block) => block.includes('content-visibility: auto;'))).toBe(true)
    expect(sidebarRowBlocks.some((block) => block.includes('contain-intrinsic-size: 62px;'))).toBe(true)
  })

  it('keeps the sticky subtask time axis horizontally synced with the rows scroller', () => {
    const wrapper = mount(NodeGanttDialog, {
      props: {
        modelValue: true,
        gantt: sampleNodeGantt,
        scale: 'week',
      },
    })

    const axisScroll = wrapper.get('[data-testid="node-gantt-axis-scroll"]').element as HTMLElement
    const rowsScroll = wrapper.get('[data-testid="node-gantt-rows-scroll"]').element as HTMLElement

    rowsScroll.scrollLeft = 96
    rowsScroll.dispatchEvent(new Event('scroll'))
    expect(axisScroll.scrollLeft).toBe(96)

    axisScroll.scrollLeft = 48
    axisScroll.dispatchEvent(new Event('scroll'))
    expect(rowsScroll.scrollLeft).toBe(48)
  })

  it('does not lose subtask gantt sync after a mirrored rows scroll event followed by another fast axis drag', () => {
    const wrapper = mount(NodeGanttDialog, {
      props: {
        modelValue: true,
        gantt: sampleNodeGantt,
        scale: 'week',
      },
    })

    const axisScroll = wrapper.get('[data-testid="node-gantt-axis-scroll"]').element as HTMLElement
    const rowsScroll = wrapper.get('[data-testid="node-gantt-rows-scroll"]').element as HTMLElement

    axisScroll.scrollLeft = 156
    axisScroll.dispatchEvent(new Event('scroll'))
    expect(rowsScroll.scrollLeft).toBe(156)

    rowsScroll.dispatchEvent(new Event('scroll'))

    axisScroll.scrollLeft = 0
    axisScroll.dispatchEvent(new Event('scroll'))
    expect(rowsScroll.scrollLeft).toBe(0)
  })
})

describe('ProjectMemberGanttView', () => {
  it('keeps member subtask hover detail cards out of normal layout flow so timeline hover stays stable', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectMemberGanttView.vue'), 'utf8')

    expect(source).toContain('<Teleport to="body">')
    expect(source).toContain('.project-member-gantt__detail-card {')
    expect(source).toContain('position: fixed;')
    expect(source).toContain(":data-placement=\"hoveredSubtaskPlacement\"")
    expect(source).toContain(':style="detailCardStyle"')
    expect(source).toContain('max-height: calc(100vh - 48px);')
    expect(source).toContain('pointer-events: auto;')
    expect(source).toContain(".project-member-gantt__detail-card[data-placement='top-right'] {")
    expect(source).not.toContain('right: 24px;')
    expect(source).not.toContain('bottom: 24px;')
    expect(source).not.toContain('translateY(calc(-50% - 1px))')
  })

  it('renders a color-coded member subtask hover detail card with owner, stage and progress emphasis', async () => {
    const wrapper = mount(ProjectMemberGanttView, {
      attachTo: document.body,
      props: {
        gantt: sampleMemberGantt,
        perspective: 'member',
        scale: 'week',
      },
    })

    await wrapper.get('[data-testid="project-member-gantt-bar-4001"]').trigger('mouseenter')

    const detailCard = document.body.querySelector('[data-testid="project-member-gantt-detail-card"]')
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectMemberGanttView.vue'), 'utf8')

    expect(detailCard).not.toBeNull()
    expect(detailCard?.querySelector('.project-member-gantt__detail-status')?.className).toContain(
      'project-member-gantt__detail-status--active',
    )
    expect(
      detailCard?.querySelector('[data-testid="project-member-gantt-detail-progress-fill"]')?.getAttribute('style'),
    ).toContain('width: 50%;')
    expect(detailCard?.textContent).toContain('登录联调')
    expect(detailCard?.textContent).toContain('前端开发')
    expect(detailCard?.textContent).toContain('王五')
    expect(detailCard?.textContent).toContain('50%')
    expect(source).toContain('.project-member-gantt__detail-progress-track')
    expect(source).toContain('.project-member-gantt__detail-status--active')

    wrapper.unmount()
  })

  it('anchors the member subtask detail card to the hover point and flips placement near viewport edges', async () => {
    const wrapper = mount(ProjectMemberGanttView, {
      attachTo: document.body,
      props: {
        gantt: sampleMemberGantt,
        perspective: 'member',
        scale: 'week',
      },
    })

    await wrapper.get('[data-testid="project-member-gantt-bar-4001"]').trigger('mouseenter', {
      clientX: 360,
      clientY: 300,
    })

    let detailCard = document.body.querySelector('[data-testid="project-member-gantt-detail-card"]') as HTMLElement | null

    expect(detailCard).not.toBeNull()
    expect(detailCard?.style.left).toBe('360px')
    expect(detailCard?.style.top).toBe('300px')
    expect(detailCard?.dataset.placement).toBe('top-right')

    await wrapper.get('[data-testid="project-member-gantt-bar-4001"]').trigger('mouseenter', {
      clientX: 980,
      clientY: 72,
    })

    detailCard = document.body.querySelector('[data-testid="project-member-gantt-detail-card"]') as HTMLElement | null
    expect(detailCard?.dataset.placement).toBe('bottom-left')

    wrapper.unmount()
  })

  it('keeps the fixed member subtask detail card visible while moving from the bar into the card', async () => {
    vi.useFakeTimers()

    const wrapper = mount(ProjectMemberGanttView, {
      attachTo: document.body,
      props: {
        gantt: sampleMemberGantt,
        perspective: 'member',
        scale: 'week',
      },
    })

    await wrapper.get('[data-testid="project-member-gantt-bar-4001"]').trigger('mouseenter')

    const detailCard = document.body.querySelector('[data-testid="project-member-gantt-detail-card"]')
    expect(detailCard).not.toBeNull()

    await wrapper.get('[data-testid="project-member-gantt-bar-4001"]').trigger('mouseleave')
    expect(document.body.querySelector('[data-testid="project-member-gantt-detail-card"]')).not.toBeNull()

    detailCard?.dispatchEvent(new MouseEvent('mouseenter', { bubbles: true }))
    await vi.advanceTimersByTimeAsync(240)
    expect(document.body.querySelector('[data-testid="project-member-gantt-detail-card"]')).not.toBeNull()

    detailCard?.dispatchEvent(new MouseEvent('mouseleave', { bubbles: true }))
    await vi.advanceTimersByTimeAsync(240)
    expect(document.body.querySelector('[data-testid="project-member-gantt-detail-card"]')).toBeNull()

    wrapper.unmount()
    vi.useRealTimers()
  })

  it('renders fixed member rows and a matching time axis for the member gantt perspective', () => {
    const wrapper = mount(ProjectMemberGanttView, {
      props: {
        gantt: sampleMemberGantt,
        perspective: 'member',
        scale: 'week',
      },
    })

    expect(wrapper.get('[data-testid="project-member-gantt-view"]').exists()).toBe(true)
    expect(wrapper.get('[data-testid="project-member-gantt-axis"]').findAll('.project-member-gantt__axis-cell').length).toBe(15)
    expect(wrapper.get('[data-testid="project-member-gantt-row-label-18"]').text()).toContain('王五')
    expect(wrapper.get('[data-testid="project-member-gantt-row-label-19"]').text()).toContain('赵六')
  })

  it('packs overlapping member subtasks into separate internal lanes instead of drawing them on top of each other', () => {
    const wrapper = mount(ProjectMemberGanttView, {
      props: {
        gantt: sampleMemberGantt,
        perspective: 'member',
        scale: 'week',
      },
    })

    const firstBar = wrapper.get('[data-testid="project-member-gantt-bar-4001"]')
    const secondBar = wrapper.get('[data-testid="project-member-gantt-bar-4002"]')
    const memberRow = wrapper.get('[data-testid="project-member-gantt-row-18"]')

    expect(firstBar.attributes('style')).toContain('top:')
    expect(secondBar.attributes('style')).toContain('top:')
    expect(firstBar.attributes('style')).not.toBe(secondBar.attributes('style'))
    expect(memberRow.attributes('style')).toContain('min-height:')
    expect(memberRow.attributes('style')).toContain('height: 104px;')
  })

  it('stretches each member track to the computed row height so extra lanes remain visible', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectMemberGanttView.vue'), 'utf8')

    expect(source).toContain('function getRowStyle(rowHeightPx: number) {')
    expect(source).toContain('height: `${rowHeightPx}px`')
    expect(source).toContain('.project-member-gantt__track {')
    expect(source).toContain('height: 100%;')
  })

  it('keeps desktop vertical scrolling inside the right member timeline host so the shared headers stay visible', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectMemberGanttView.vue'), 'utf8')

    expect(source).toContain('.project-member-gantt__body-scroll {')
    expect(source).toContain('--project-member-gantt-viewport-height: 620px;')
    expect(source).toContain('max-height: var(--project-member-gantt-viewport-height);')
    expect(source).toContain('overflow: hidden;')
    expect(source).toContain('class="project-member-gantt__body-scroll smooth-scroll-surface"')
    expect(source).not.toContain(`v-smooth-wheel="{ axis: 'horizontal', wheelBehavior: 'block' }"`)
    expect(source).toContain('project-member-gantt__sidebar-scroll')
    expect(source).toContain('.project-member-gantt__sidebar-scroll {')
    expect(source).toContain('.project-member-gantt__top-scroll {')
    expect(source).toContain('position: sticky;')
    expect(source).toContain('top: 0;')
    expect(source).toContain('.project-member-gantt__axis-scroll {')
    expect(source).toContain('top: var(--project-member-gantt-top-scroll-height);')
    expect(source).toContain('.project-member-gantt__timeline-scroll {')
    expect(source).toContain('overflow-y: auto;')
  })

  it('keeps the sticky member time axis horizontally synced with the rows scroller', () => {
    const wrapper = mount(ProjectMemberGanttView, {
      props: {
        gantt: sampleMemberGantt,
        perspective: 'member',
        scale: 'week',
      },
    })

    const axisScroll = wrapper.get('[data-testid="project-member-gantt-axis-scroll"]').element as HTMLElement
    const rowsScroll = wrapper.get('[data-testid="project-member-gantt-rows-scroll"]').element as HTMLElement

    rowsScroll.scrollLeft = 132
    rowsScroll.dispatchEvent(new Event('scroll'))
    expect(axisScroll.scrollLeft).toBe(132)

    axisScroll.scrollLeft = 84
    axisScroll.dispatchEvent(new Event('scroll'))
    expect(rowsScroll.scrollLeft).toBe(84)
  })

  it('does not lose member gantt sync after a mirrored rows scroll event followed by another fast axis drag', () => {
    const wrapper = mount(ProjectMemberGanttView, {
      props: {
        gantt: sampleMemberGantt,
        perspective: 'member',
        scale: 'week',
      },
    })

    const axisScroll = wrapper.get('[data-testid="project-member-gantt-axis-scroll"]').element as HTMLElement
    const rowsScroll = wrapper.get('[data-testid="project-member-gantt-rows-scroll"]').element as HTMLElement

    axisScroll.scrollLeft = 156
    axisScroll.dispatchEvent(new Event('scroll'))
    expect(rowsScroll.scrollLeft).toBe(156)

    rowsScroll.dispatchEvent(new Event('scroll'))

    axisScroll.scrollLeft = 0
    axisScroll.dispatchEvent(new Event('scroll'))
    expect(rowsScroll.scrollLeft).toBe(0)
  })

  it('places the sticky member time axis outside the rows scroller so vertical pinning is not blocked by horizontal overflow', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectMemberGanttView.vue'), 'utf8')

    expect(source).toContain('project-member-gantt-axis-scroll')
    expect(source).toContain('project-member-gantt-rows-scroll')
    expect(source).toContain('project-member-gantt__top-scroll')
    expect(source).toContain('.project-member-gantt__axis-scroll {')
    expect(source).toContain('position: sticky;')
    expect(source).toContain('.project-member-gantt__timeline-scroll {')
    expect(source).toContain('overflow-x: auto;')
  })

  it('uses native horizontal scrolling that stays in the member timeline region instead of stretching under the sidebar', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectMemberGanttView.vue'), 'utf8')

    expect(source).toContain('data-testid="project-member-gantt-axis-scroll"')
    expect(source).toContain('project-member-gantt__top-scroll')
    expect(source).toContain('.project-member-gantt__top-scroll {')
    expect(source).toContain('overflow-x: auto;')
    expect(source).toContain('.project-member-gantt__timeline-scroll {')
    expect(source).toContain('overflow-x: auto;')
    expect(source).toContain('.project-member-gantt__top-scroll::-webkit-scrollbar {')
    expect(source).toContain('height: var(--project-member-gantt-top-scroll-height);')
    expect(source).not.toContain('scrollbar-width: none;')
  })

  it('visually merges the member sidebar and timeline into one shared shell with only a center divider', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectMemberGanttView.vue'), 'utf8')

    expect(source).toContain('.project-member-gantt__body {')
    expect(source).toContain('gap: 0;')
    expect(source).toContain('border: 1px solid var(--meta-surface-border);')
    expect(source).toContain('border-radius: 22px;')
    expect(source).toContain('.project-member-gantt__sidebar,\n.project-member-gantt__timeline-shell {')
    expect(source).toContain('border: 0;')
    expect(source).toContain('border-radius: 0;')
    expect(source).toContain('background: transparent;')
    expect(source).toContain('.project-member-gantt__sidebar {')
    expect(source).toContain('border-right: 1px solid var(--meta-surface-border);')
  })

  it('keeps the left member header stack aligned with the right top scrollbar plus axis header heights', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectMemberGanttView.vue'), 'utf8')

    expect(source).toContain('--project-member-gantt-top-scroll-height: 18px;')
    expect(source).toContain('project-member-gantt__sidebar-top-spacer')
    expect(source).toContain('.project-member-gantt__sidebar-top-spacer {')
    expect(source).toContain('height: var(--project-member-gantt-top-scroll-height);')
    expect(source).toContain('.project-member-gantt__top-scroll {')
    expect(source).toContain('height: var(--project-member-gantt-top-scroll-height);')
    expect(source).toContain('.project-member-gantt__sidebar-head {')
    expect(source).toContain('top: var(--project-member-gantt-top-scroll-height);')
    expect(source).toContain('.project-member-gantt__axis-scroll {')
    expect(source).toContain('top: var(--project-member-gantt-top-scroll-height);')
  })

  it('keeps the left member rows visually synced when the right timeline scrolls vertically', () => {
    const wrapper = mount(ProjectMemberGanttView, {
      props: {
        gantt: sampleMemberGantt,
        perspective: 'member',
        scale: 'week',
      },
    })

    const sidebarRail = wrapper.get('.project-member-gantt__sidebar-rows-rail').element as HTMLElement
    const rowsScroll = wrapper.get('[data-testid="project-member-gantt-rows-scroll"]').element as HTMLElement

    rowsScroll.scrollTop = 96
    rowsScroll.dispatchEvent(new Event('scroll'))

    expect(sidebarRail.style.transform).toBe('translateY(-96px)')

    wrapper.unmount()
  })

  it('uses token-based axis and track surfaces for the member gantt instead of old glass-bg mixes', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectMemberGanttView.vue'), 'utf8')

    expect(source).toContain('background: var(--dialog-control-bg), var(--card-sheen);')
    expect(source).not.toContain('color-mix(in srgb, var(--glass-bg) 96%, transparent)')
  })

  it('keeps the sticky member axis free of backdrop blur and isolates each row paint to reduce scroll jank', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectMemberGanttView.vue'), 'utf8')
    const axisBlock = source.match(/\.project-member-gantt__axis \{[\s\S]*?\n\}/)?.[0]
    const rowBlock = source.match(/\.project-member-gantt__row \{[\s\S]*?\n\}/)?.[0]
    const sidebarRowBlocks = [...source.matchAll(/\.project-member-gantt__sidebar-row \{[\s\S]*?\n\}/g)].map((match) => match[0])

    expect(axisBlock).toBeTruthy()
    expect(axisBlock).not.toContain('backdrop-filter')
    expect(rowBlock).toBeTruthy()
    expect(rowBlock).toContain('contain: layout paint;')
    expect(rowBlock).toContain('content-visibility: auto;')
    expect(sidebarRowBlocks.length).toBeGreaterThan(0)
    expect(sidebarRowBlocks.some((block) => block.includes('content-visibility: auto;'))).toBe(true)
    expect(sidebarRowBlocks.some((block) => block.includes('contain-intrinsic-size: 62px;'))).toBe(true)
  })
})
