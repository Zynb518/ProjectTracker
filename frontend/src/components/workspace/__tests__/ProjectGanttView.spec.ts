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
    const wrapper = mount(ProjectGanttView, {
      attachTo: document.body,
      props: {
        gantt: sampleProjectGantt,
        scale: 'week',
      },
    })

    await wrapper.get('[data-testid="project-gantt-stage-bar-2002"]').trigger('mouseenter')

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
    expect(source).toContain(
      'background: linear-gradient(135deg, color-mix(in srgb, var(--accent-start) 82%, #0d2754 18%), color-mix(in srgb, var(--accent-end) 70%, #0f2c5b 30%));',
    )
  })

  it('anchors the stage detail card to the hover point and flips placement near viewport edges', async () => {
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

    let detailCard = document.body.querySelector('[data-testid="project-gantt-detail-card"]') as HTMLElement | null

    expect(detailCard).not.toBeNull()
    expect(detailCard?.style.left).toBe('320px')
    expect(detailCard?.style.top).toBe('260px')
    expect(detailCard?.dataset.placement).toBe('top-right')

    await wrapper.get('[data-testid="project-gantt-stage-bar-2002"]').trigger('mouseenter', {
      clientX: 980,
      clientY: 60,
    })

    detailCard = document.body.querySelector('[data-testid="project-gantt-detail-card"]') as HTMLElement | null
    expect(detailCard?.dataset.placement).toBe('bottom-left')

    wrapper.unmount()
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

  it('renders fixed stage labels and a matching time axis', () => {
    const wrapper = mount(ProjectGanttView, {
      props: {
        gantt: sampleProjectGantt,
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
    expect(source).toContain('linear-gradient(180deg, color-mix(in srgb, var(--accent-end) 16%, transparent), color-mix(in srgb, var(--glass-bg) 92%, transparent))')
    expect(source).not.toContain('.project-gantt__axis-cell--tail {')
    expect(source).not.toContain('padding-right: 28px;')
  })

  it('renders track cells that align with the axis instead of fixed stripe dividers', () => {
    const wrapper = mount(ProjectGanttView, {
      props: {
        gantt: sampleProjectGantt,
        scale: 'week',
      },
    })
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectGanttView.vue'), 'utf8')
    const axisCells = wrapper.get('[data-testid="project-gantt-axis"]').findAll('.project-gantt__axis-cell')
    const trackCells = wrapper.get('[data-testid="project-gantt-track-grid-2001"]').findAll('.project-gantt__track-cell')

    expect(trackCells).toHaveLength(axisCells.length)
    expect(trackCells[0]?.attributes('style')).toBe(axisCells[0]?.attributes('style'))
    expect(source).toContain('.project-gantt__track-grid {')
    expect(source).toContain('.project-gantt__track-cell {')
    expect(source).not.toContain('repeating-linear-gradient(90deg')
  })

  it('uses an internal 80vh scrolling viewport with a sticky stage axis', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectGanttView.vue'), 'utf8')

    expect(source).toContain('.project-gantt__body-scroll {')
    expect(source).toContain('max-height: 80vh;')
    expect(source).toContain('overflow: auto;')
    expect(source).toContain('.project-gantt__axis-scroll {')
    expect(source).toContain('position: sticky;')
    expect(source).toContain('top: 0;')
    expect(source).toContain("wheelBehavior: 'block'")
  })

  it('keeps the stage gantt horizontal scrollbar at the top sticky axis instead of the rows area', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectGanttView.vue'), 'utf8')

    expect(source).toContain('.project-gantt__axis-scroll {')
    expect(source).toContain('overflow-x: scroll;')
    expect(source).toContain('scrollbar-gutter: stable;')
    expect(source).toContain('.project-gantt__timeline-scroll {')
    expect(source).toContain('overflow-x: hidden;')
    expect(source).toContain('.project-gantt__axis-scroll::-webkit-scrollbar {')
    expect(source).toContain('height: 12px;')
    expect(source).not.toContain('scrollbar-width: none;')
  })

  it('keeps the sticky stage time axis horizontally synced with the rows scroller', () => {
    const wrapper = mount(ProjectGanttView, {
      props: {
        gantt: sampleProjectGantt,
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
  })

  it('does not lose stage gantt sync after a mirrored rows scroll event followed by another fast axis drag', () => {
    const wrapper = mount(ProjectGanttView, {
      props: {
        gantt: sampleProjectGantt,
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
  })

  it('emits the selected node id when a stage bar is clicked', async () => {
    const wrapper = mount(ProjectGanttView, {
      props: {
        gantt: sampleProjectGantt,
        scale: 'week',
      },
    })

    await wrapper.get('[data-testid="project-gantt-stage-bar-2002"]').trigger('click')

    expect(wrapper.emitted('open-node')).toEqual([[2002]])
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
    expect(source).toContain(
      'background: linear-gradient(135deg, color-mix(in srgb, var(--accent-start) 82%, #0d2754 18%), color-mix(in srgb, var(--accent-end) 70%, #0f2c5b 30%));',
    )
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

  it('uses an 80vh constrained dialog viewport with a sticky subtask axis', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/NodeGanttDialog.vue'), 'utf8')

    expect(source).toContain('.node-gantt-dialog {')
    expect(source).toContain('max-height: min(80vh, calc(100vh - 32px));')
    expect(source).toContain('.node-gantt-dialog__body-scroll {')
    expect(source).toContain('overflow: auto;')
    expect(source).toContain('.node-gantt-dialog__axis-scroll {')
    expect(source).toContain('position: sticky;')
    expect(source).toContain('top: 0;')
    expect(source).toContain("wheelBehavior: 'block'")
  })

  it('keeps the subtask gantt horizontal scrollbar at the top sticky axis instead of the rows area', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/NodeGanttDialog.vue'), 'utf8')

    expect(source).toContain('.node-gantt-dialog__axis-scroll {')
    expect(source).toContain('overflow-x: scroll;')
    expect(source).toContain('scrollbar-gutter: stable;')
    expect(source).toContain('.node-gantt-dialog__timeline-scroll {')
    expect(source).toContain('overflow-x: hidden;')
    expect(source).toContain('.node-gantt-dialog__axis-scroll::-webkit-scrollbar {')
    expect(source).toContain('height: 12px;')
    expect(source).not.toContain('scrollbar-width: none;')
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

  it('uses a fixed-height internal scrolling viewport so member rows do not stretch the whole page', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectMemberGanttView.vue'), 'utf8')

    expect(source).toContain('.project-member-gantt__body-scroll {')
    expect(source).toContain('max-height: 620px;')
    expect(source).toContain('overflow: auto;')
    expect(source).toContain('.project-member-gantt__axis-scroll {')
    expect(source).toContain('position: sticky;')
    expect(source).toContain('top: 0;')
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
    expect(source).toContain('.project-member-gantt__axis-scroll {')
    expect(source).toContain('position: sticky;')
    expect(source).toContain('.project-member-gantt__timeline-scroll {')
    expect(source).toContain('overflow-x: hidden;')
    expect(source).toContain("wheelBehavior: 'block'")
  })

  it('keeps the member gantt horizontal scrollbar at the top sticky axis instead of the rows area', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectMemberGanttView.vue'), 'utf8')

    expect(source).toContain('.project-member-gantt__axis-scroll {')
    expect(source).toContain('overflow-x: scroll;')
    expect(source).toContain('scrollbar-gutter: stable;')
    expect(source).toContain('.project-member-gantt__timeline-scroll {')
    expect(source).toContain('overflow-x: hidden;')
    expect(source).toContain('.project-member-gantt__axis-scroll::-webkit-scrollbar {')
    expect(source).toContain('height: 12px;')
    expect(source).not.toContain('scrollbar-width: none;')
  })
})
