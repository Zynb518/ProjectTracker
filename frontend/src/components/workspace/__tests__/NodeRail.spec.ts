import { readFileSync } from 'node:fs'
import { resolve } from 'node:path'
import { mount } from '@vue/test-utils'
import { describe, expect, it } from 'vitest'

import NodeRail from '@/components/workspace/NodeRail.vue'
import type { ProjectNode } from '@/types/node'

const sampleNodes: ProjectNode[] = [
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
    status: 4,
    planned_start_date: '2026-04-21',
    planned_end_date: '2026-04-28',
    completed_at: null,
    created_by: 1,
    created_at: '2026-04-01T09:00:00+08:00',
    updated_at: '2026-04-06T12:00:00+08:00',
    sub_task_count: 5,
    completed_sub_task_count: 1,
  },
  {
    id: 2004,
    project_id: 1001,
    name: '上线验收',
    description: '准备上线验收与交付',
    sequence_no: 4,
    status: 1,
    planned_start_date: '2026-04-29',
    planned_end_date: '2026-05-03',
    completed_at: null,
    created_by: 1,
    created_at: '2026-04-02T09:00:00+08:00',
    updated_at: '2026-04-02T09:00:00+08:00',
    sub_task_count: 3,
    completed_sub_task_count: 0,
  },
]

describe('NodeRail', () => {
  it('defaults to compact cards and allows switching back to the full card layout', async () => {
    const wrapper = mount(NodeRail, {
      props: {
        canManage: true,
        nodes: sampleNodes,
        selectedNodeId: null,
      },
    })

    const activeCard = wrapper.get('[data-testid="node-item-2002"]')

    expect(wrapper.get('[data-testid="node-view-mode-compact"]').attributes('aria-pressed')).toBe('true')
    expect(wrapper.get('[data-testid="node-view-mode-full"]').attributes('aria-pressed')).toBe('false')
    expect(activeCard.text()).toContain('前端开发')
    expect(activeCard.text()).toContain('进行中')
    expect(activeCard.text()).not.toContain('Phase 2')
    expect(activeCard.text()).not.toContain('子任务完成')
    expect(activeCard.find('.node-rail__progress').exists()).toBe(false)

    await wrapper.get('[data-testid="node-view-mode-full"]').trigger('click')

    expect(wrapper.get('[data-testid="node-view-mode-compact"]').attributes('aria-pressed')).toBe('false')
    expect(wrapper.get('[data-testid="node-view-mode-full"]').attributes('aria-pressed')).toBe('true')
    expect(activeCard.text()).toContain('Phase 2')
    expect(activeCard.text()).toContain('2 / 6 子任务完成')
    expect(activeCard.find('.node-rail__progress').exists()).toBe(true)
  })

  it('removes the node count copy and uses compact icon switchers with hover labels', () => {
    const wrapper = mount(NodeRail, {
      props: {
        canManage: true,
        nodes: sampleNodes,
        selectedNodeId: null,
      },
    })
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/NodeRail.vue'), 'utf8')

    expect(wrapper.text()).not.toContain(`${sampleNodes.length} 个节点`)
    expect(wrapper.get('[data-testid="node-view-mode-compact"]').attributes('aria-label')).toBe('精简视图')
    expect(wrapper.get('[data-testid="node-view-mode-compact"]').attributes('data-tooltip')).toBe('精简视图')
    expect(wrapper.get('[data-testid="node-view-mode-full"]').attributes('aria-label')).toBe('完整视图')
    expect(wrapper.get('[data-testid="node-view-mode-full"]').attributes('data-tooltip')).toBe('完整视图')
    expect(wrapper.get('[data-testid="node-view-mode-compact"]').text()).toBe('')
    expect(wrapper.get('[data-testid="node-view-mode-full"]').text()).toBe('')
    expect(source).toContain('overflow: visible;')
    expect(source).toContain('z-index: 12;')
    expect(source).toContain('z-index: 24;')
    expect(source).toContain('.node-rail__view-button[data-tooltip]::before')
    expect(source).toContain('.node-rail__view-button[data-tooltip]::after')
    expect(source).not.toContain('{{ props.nodes.length }} 个节点')
  })

  it('keeps node names and status pills on one row so long names do not push statuses out of the card', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/NodeRail.vue'), 'utf8')

    expect(source).toContain('.node-rail__item-top,')
    expect(source).toContain('.node-rail__compact-head,')
    expect(source).toContain('overflow: hidden;')
    expect(source).toContain('.node-rail__status,')
    expect(source).toContain('flex-shrink: 0;')
    expect(source).toContain('white-space: nowrap;')
    expect(source).toContain('.node-rail__name--compact {')
    expect(source).toContain('text-overflow: ellipsis;')
  })

  it('shows a hover detail card as a fixed top-layer overlay on the right side of the hovered node', async () => {
    const wrapper = mount(NodeRail, {
      attachTo: document.body,
      props: {
        nodes: sampleNodes,
        selectedNodeId: null,
      },
    })

    const hoveredItem = wrapper.get('[data-testid="node-item-2002"]')
    Object.defineProperty(hoveredItem.element, 'getBoundingClientRect', {
      configurable: true,
      value: () => ({
        x: 180,
        y: 120,
        width: 260,
        height: 84,
        top: 120,
        left: 180,
        right: 440,
        bottom: 204,
        toJSON: () => null,
      }),
    })

    expect(wrapper.find('[data-testid="node-hover-card"]').exists()).toBe(false)

    await hoveredItem.trigger('mouseenter')

    expect(wrapper.find('[data-testid="node-hover-card"]').exists()).toBe(false)

    const hoverCard = document.body.querySelector('[data-testid="node-hover-card"]') as HTMLElement | null

    expect(hoverCard).not.toBeNull()
    expect(hoverCard?.textContent).toContain('完成项目界面与交互实现')
    expect(hoverCard?.className).toContain('node-rail__hover-card--right')
    expect(hoverCard?.style.position).toBe('fixed')
    expect(hoverCard?.style.pointerEvents).toBe('none')
    expect(hoverCard?.style.zIndex).toBe('240')

    wrapper.unmount()
  })

  it('emits selection only for unfinished timeline nodes', async () => {
    const wrapper = mount(NodeRail, {
      props: {
        nodes: sampleNodes,
        selectedNodeId: null,
      },
    })

    await wrapper.get('[data-testid="node-item-2001"]').trigger('click')
    await wrapper.get('[data-testid="node-item-2002"]').trigger('click')
    await wrapper.get('[data-testid="node-item-2003"]').trigger('click')
    await wrapper.get('[data-testid="node-item-2004"]').trigger('click')

    expect(wrapper.emitted('select')).toEqual([[2002], [2003], [2004]])
  })

  it('renders management controls and emits node lifecycle actions for manageable rows', async () => {
    const wrapper = mount(NodeRail, {
      props: {
        canManage: true,
        nodes: sampleNodes,
        selectedNodeId: null,
      },
    })

    expect(wrapper.get('[data-testid="node-item-2002"]').find('.node-actions__main').exists()).toBe(false)

    await wrapper.get('[data-testid="node-view-mode-full"]').trigger('click')
    await wrapper.get('[data-testid="create-node"]').trigger('click')
    await wrapper.get('[data-testid="node-action-complete-2002"]').trigger('click')
    await wrapper.get('[data-testid="node-action-edit-2002"]').trigger('click')
    await wrapper.get('[data-testid="node-action-remove-2002"]').trigger('click')
    await wrapper.get('[data-testid="node-action-reopen-2001"]').trigger('click')
    await wrapper.get('[data-testid="node-action-start-2004"]').trigger('click')

    expect(wrapper.get('[data-testid="create-node"]').attributes('aria-label')).toBe('新建节点')
    expect(wrapper.get('[data-testid="create-node"]').text()).toBe('')
    expect(wrapper.emitted('create')).toEqual([[]])
    expect(wrapper.emitted('complete')).toEqual([[2002]])
    expect(wrapper.emitted('edit')).toEqual([[2002]])
    expect(wrapper.emitted('remove')).toEqual([[2002]])
    expect(wrapper.emitted('reopen')).toEqual([[2001]])
    expect(wrapper.emitted('start')).toEqual([[2004]])
    expect(wrapper.get('[data-testid="node-item-2002"] [data-testid="node-action-edit-2002"]').exists()).toBe(
      true,
    )
    expect(wrapper.get('[data-testid="node-item-2002"]').find('.node-actions').exists()).toBe(false)
    expect(wrapper.get('[data-testid="node-action-edit-2001"]').attributes('disabled')).toBeDefined()
    expect(wrapper.get('[data-testid="node-action-remove-2001"]').attributes('disabled')).toBeDefined()
  })

  it('shows only the drag handle icon without drag order text on draggable nodes', async () => {
    const wrapper = mount(NodeRail, {
      props: {
        canManage: true,
        nodes: sampleNodes,
        selectedNodeId: null,
      },
    })

    await wrapper.get('[data-testid="node-view-mode-full"]').trigger('click')

    expect(wrapper.get('[data-testid="node-item-2002"]').text()).not.toContain('Drag')
    expect(wrapper.get('[data-testid="node-item-2003"]').text()).not.toContain('Drag')
    expect(wrapper.get('[data-testid="node-item-2004"]').text()).not.toContain('Drag')
  })

  it('emits reordered node sequence after dragging one unfinished card over another unfinished card', async () => {
    const wrapper = mount(NodeRail, {
      props: {
        canManage: true,
        nodes: sampleNodes,
        selectedNodeId: null,
      },
    })

    await wrapper.get('[data-testid="node-item-2004"]').trigger('dragstart')
    await wrapper.get('[data-testid="node-item-2002"]').trigger('dragenter')
    await wrapper.get('[data-testid="node-item-2002"]').trigger('drop')

    expect(wrapper.emitted('reorder')).toEqual([
      [
        {
          nodes: [
            { node_id: 2001, sequence_no: 1 },
            { node_id: 2004, sequence_no: 2 },
            { node_id: 2002, sequence_no: 3 },
            { node_id: 2003, sequence_no: 4 },
          ],
        },
      ],
    ])
  })
})
