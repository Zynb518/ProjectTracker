import { mount } from '@vue/test-utils'

import ProjectActionsMenu from '@/components/projects/ProjectActionsMenu.vue'
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

describe('ProjectActionsMenu', () => {
  it('renders lifecycle actions based on project status', () => {
    const pendingWrapper = mount(ProjectActionsMenu, {
      props: {
        project: { ...sampleProject, status: 1 },
      },
    })
    const activeWrapper = mount(ProjectActionsMenu, {
      props: {
        project: { ...sampleProject, status: 2 },
      },
    })
    const doneWrapper = mount(ProjectActionsMenu, {
      props: {
        project: { ...sampleProject, status: 3 },
      },
    })
    const delayedWrapper = mount(ProjectActionsMenu, {
      props: {
        project: { ...sampleProject, status: 4 },
      },
    })

    expect(pendingWrapper.findAll('button').map((button) => button.text())).toEqual(['开始', '编辑', '删除'])
    expect(activeWrapper.findAll('button').map((button) => button.text())).toEqual(['完成', '编辑', '删除'])
    expect(doneWrapper.findAll('button').map((button) => button.text())).toEqual([
      '撤销完成',
      '编辑',
      '删除',
    ])
    expect(doneWrapper.findAll('button')[1].attributes('disabled')).toBeDefined()
    expect(delayedWrapper.findAll('button').map((button) => button.text())).toEqual(['完成', '编辑', '删除'])
  })

  it('emits the matching lifecycle event for the current status and blocks disabled edit', async () => {
    const pendingWrapper = mount(ProjectActionsMenu, {
      props: {
        project: { ...sampleProject, status: 1 },
      },
    })
    const activeWrapper = mount(ProjectActionsMenu, {
      props: {
        project: { ...sampleProject, status: 2 },
      },
    })
    const doneWrapper = mount(ProjectActionsMenu, {
      props: {
        project: { ...sampleProject, status: 3 },
      },
    })

    await pendingWrapper.findAll('button')[0].trigger('click')
    await activeWrapper.findAll('button')[0].trigger('click')
    await doneWrapper.findAll('button')[0].trigger('click')
    await doneWrapper.findAll('button')[1].trigger('click')

    expect(pendingWrapper.emitted('start')).toEqual([[]])
    expect(activeWrapper.emitted('complete')).toEqual([[]])
    expect(doneWrapper.emitted('reopen')).toEqual([[]])
    expect(doneWrapper.emitted('edit')).toBeUndefined()
  })
})
