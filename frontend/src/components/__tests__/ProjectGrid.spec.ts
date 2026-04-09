import { mount } from '@vue/test-utils'
import { readFileSync } from 'node:fs'
import { resolve } from 'node:path'

import ProjectGrid from '@/components/projects/ProjectGrid.vue'
import type { ProjectListItem } from '@/types/project'

const sampleProject: ProjectListItem = {
  id: 1001,
  name: '统一高度验证项目名称',
  description: '用于验证项目列表卡片采用统一最小高度，避免不同内容长度导致视觉参差。',
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
}

describe('ProjectGrid', () => {
  it('uses a fixed minimum height for each project card', () => {
    const wrapper = mount(ProjectGrid, {
      props: {
        projects: [sampleProject],
      },
    })

    const card = wrapper.get('.project-card').element

    expect(getComputedStyle(card).minHeight).toBe('492px')
  })

  it('uses fixed-width project columns instead of stretching cards across the row', () => {
    const source = readFileSync(
      resolve(process.cwd(), 'src/components/projects/ProjectGrid.vue'),
      'utf8',
    )

    expect(source).toContain('grid-template-columns: repeat(auto-fill, 400px);')
    expect(source).toContain('justify-content: start;')
    expect(source).toContain('.project-grid {\n    grid-template-columns: minmax(0, 1fr);')
  })

  it('uses a dedicated non-white surface style for each project card so it does not blend into the inner meta cards', () => {
    const source = readFileSync(
      resolve(process.cwd(), 'src/components/projects/ProjectGrid.vue'),
      'utf8',
    )
    const themeSource = readFileSync(resolve(process.cwd(), 'src/styles/theme.css'), 'utf8')

    expect(source).toContain('background: var(--project-card-bg), var(--project-card-glow);')
    expect(themeSource).toContain('--project-card-bg: linear-gradient(180deg, rgba(227, 235, 246, 0.98), rgba(214, 225, 239, 0.96));')
    expect(themeSource).toContain('--project-card-glow: radial-gradient(circle at top right, rgba(0, 194, 255, 0.1), transparent 36%);')
  })

  it('renders the project name in the header without showing the project id', () => {
    const wrapper = mount(ProjectGrid, {
      props: {
        projects: [sampleProject],
      },
    })

    expect(wrapper.get('.project-card__name').text()).toBe(sampleProject.name)
    expect(wrapper.text()).not.toContain(`项目 #${sampleProject.id}`)
  })

  it('uses a brighter single-line title treatment with ellipsis so names do not collide with status tags', () => {
    const source = readFileSync(
      resolve(process.cwd(), 'src/components/projects/ProjectGrid.vue'),
      'utf8',
    )

    expect(source).toContain('font-size: 1.68rem;')
    expect(source).toContain('font-weight: 700;')
    expect(source).toContain('color: color-mix(in srgb, var(--text-main) 92%, var(--accent-end) 8%);')
    expect(source).toContain('white-space: nowrap;')
    expect(source).toContain('text-overflow: ellipsis;')
  })

  it('maps each status to a dedicated status color class', () => {
    const wrapper = mount(ProjectGrid, {
      props: {
        projects: [
          { ...sampleProject, id: 1, status: 1 },
          { ...sampleProject, id: 2, status: 2 },
          { ...sampleProject, id: 3, status: 3 },
          { ...sampleProject, id: 4, status: 4 },
        ],
      },
    })

    const statuses = wrapper.findAll('.project-card__status')

    expect(statuses[0].classes()).toContain('project-card__status--pending')
    expect(statuses[1].classes()).toContain('project-card__status--active')
    expect(statuses[2].classes()).toContain('project-card__status--done')
    expect(statuses[3].classes()).toContain('project-card__status--delayed')
  })

  it('renders the planned start and end dates on separate lines', () => {
    const wrapper = mount(ProjectGrid, {
      props: {
        projects: [sampleProject],
      },
    })

    const lines = wrapper.findAll('.project-card__schedule-line')

    expect(lines).toHaveLength(2)
    expect(lines[0].text()).toBe('2026-03-20')
    expect(lines[1].text()).toBe('2026-06-30')
  })

  it('uses icon-only meta labels while preserving accessible labels for each project detail', () => {
    const wrapper = mount(ProjectGrid, {
      props: {
        projects: [sampleProject],
      },
    })

    const labels = wrapper.findAll('.project-card__meta dt')
    const icons = wrapper.findAll('.project-card__meta-icon')

    expect(labels).toHaveLength(4)
    expect(icons).toHaveLength(4)
    expect(labels.map((label) => label.attributes('aria-label'))).toEqual([
      '负责人',
      '计划周期',
      '成员数',
      '节点 / 子任务',
    ])
  })

  it('adds hover tooltip labels to each project meta card', () => {
    const wrapper = mount(ProjectGrid, {
      props: {
        projects: [sampleProject],
      },
    })

    const metaCards = wrapper.findAll('.project-card__meta-item')

    expect(metaCards).toHaveLength(4)
    expect(metaCards.map((card) => card.attributes('data-tooltip'))).toEqual([
      '负责人',
      '计划周期',
      '成员数',
      '节点 / 子任务',
    ])
  })

  it('renders project meta icons without framed containers', () => {
    const wrapper = mount(ProjectGrid, {
      props: {
        projects: [sampleProject],
      },
    })

    const icon = wrapper.get('.project-card__meta-icon').element
    const styles = getComputedStyle(icon)

    expect(styles.borderTopWidth).toBe('0px')
    expect(styles.backgroundImage).toBe('none')
    expect(styles.boxShadow).toBe('none')
  })

  it('uses a dedicated tinted surface style for project meta cards so they stand apart from the main card', () => {
    const source = readFileSync(
      resolve(process.cwd(), 'src/components/projects/ProjectGrid.vue'),
      'utf8',
    )
    const themeSource = readFileSync(resolve(process.cwd(), 'src/styles/theme.css'), 'utf8')

    expect(source).toContain('border: 1px solid var(--meta-surface-border);')
    expect(source).toContain('background: var(--meta-surface-bg), var(--meta-surface-glow);')
    expect(source).toContain('box-shadow: var(--meta-surface-shadow);')
    expect(themeSource).toContain('--meta-surface-bg: linear-gradient(180deg, rgba(239, 245, 251, 0.96), rgba(226, 234, 245, 0.94));')
    expect(themeSource).toContain('--meta-surface-glow: radial-gradient(circle at 100% 0%, rgba(0, 194, 255, 0.14), transparent 62%);')
    expect(themeSource).toContain('--meta-surface-border:')
    expect(themeSource).toContain('--meta-surface-shadow:')
  })

  it('separates project meta content and actions into independent layout regions', () => {
    const wrapper = mount(ProjectGrid, {
      props: {
        projects: [sampleProject],
      },
    })

    const card = wrapper.get('.project-card')
    const body = card.get('.project-card__body')
    const footer = card.get('.project-card__footer')

    expect(body.get('.project-card__meta').exists()).toBe(true)
    expect(footer.getComponent({ name: 'ProjectActionsMenu' }).exists()).toBe(true)
  })

  it('keeps the three action buttons in one row with balanced spacing below the meta section', () => {
    const wrapper = mount(ProjectGrid, {
      props: {
        projects: [sampleProject],
      },
    })

    const footer = wrapper.get('.project-card__footer')
    const actionBar = footer.get('.project-actions')

    expect(getComputedStyle(footer.element).alignItems).toBe('center')
    expect(actionBar.findAll('button')).toHaveLength(3)
    expect(actionBar.classes()).not.toContain('project-actions--pair')
  })

  it('keeps all project meta cards slightly taller for clearer spacing', () => {
    const wrapper = mount(ProjectGrid, {
      props: {
        projects: [sampleProject],
      },
    })

    const metaCards = wrapper.findAll('.project-card__meta div')

    expect(getComputedStyle(metaCards[0].element).minHeight).toBe('74px')
    expect(getComputedStyle(metaCards[1].element).minHeight).toBe('74px')
    expect(getComputedStyle(metaCards[2].element).minHeight).toBe('74px')
    expect(getComputedStyle(metaCards[3].element).minHeight).toBe('74px')
  })

  it('anchors meta icons to the top of each info card even when content spans multiple lines', () => {
    const wrapper = mount(ProjectGrid, {
      props: {
        projects: [sampleProject],
      },
    })

    const metaCard = wrapper.get('.project-card__meta div').element

    expect(getComputedStyle(metaCard).justifyContent).toBe('flex-start')
  })

  it('tightens the icon-to-text spacing while pushing meta text slightly farther from the left edge', () => {
    const wrapper = mount(ProjectGrid, {
      props: {
        projects: [sampleProject],
      },
    })

    const metaText = wrapper.get('.project-card__meta dd').element
    const styles = getComputedStyle(metaText)

    expect(styles.marginTop).toBe('6px')
    expect(styles.paddingLeft).toBe('4px')
  })

  it('opens the project when a user clicks the card', async () => {
    const wrapper = mount(ProjectGrid, {
      props: {
        projects: [sampleProject],
      },
    })

    await wrapper.get('.project-card').trigger('click')

    expect(wrapper.emitted('open')).toEqual([[sampleProject]])
  })

  it('does not render a dedicated open button and does not open when action buttons are clicked', async () => {
    const wrapper = mount(ProjectGrid, {
      props: {
        projects: [sampleProject],
      },
    })

    expect(wrapper.text()).not.toContain('进入项目')

    await wrapper.findAll('button').find((button) => button.attributes('aria-label') === '编辑')!.trigger('click')

    expect(wrapper.emitted('edit')).toEqual([[sampleProject]])
    expect(wrapper.emitted('open')).toBeUndefined()
  })
})
