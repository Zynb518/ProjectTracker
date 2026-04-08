import { mount } from '@vue/test-utils'

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

  it('renders the project name in the header without showing the project id', () => {
    const wrapper = mount(ProjectGrid, {
      props: {
        projects: [sampleProject],
      },
    })

    expect(wrapper.get('.project-card__name').text()).toBe(sampleProject.name)
    expect(wrapper.text()).not.toContain(`项目 #${sampleProject.id}`)
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

  it('keeps each project meta card slightly taller for clearer spacing', () => {
    const wrapper = mount(ProjectGrid, {
      props: {
        projects: [sampleProject],
      },
    })

    const metaCard = wrapper.get('.project-card__meta div').element

    expect(getComputedStyle(metaCard).minHeight).toBe('72px')
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

    await wrapper.findAll('button').find((button) => button.text() === '编辑')!.trigger('click')

    expect(wrapper.emitted('edit')).toEqual([[sampleProject]])
    expect(wrapper.emitted('open')).toBeUndefined()
  })
})
