import { mount } from '@vue/test-utils'

import MemberPanel from '@/components/members/MemberPanel.vue'

describe('MemberPanel', () => {
  it('emits remove when an operator clicks the remove button', async () => {
    const wrapper = mount(MemberPanel, {
      props: {
        canManage: true,
        members: [
          {
            project_id: 1001,
            user_id: 12,
            username: 'lisi',
            real_name: '李四',
            system_role: 3,
            status: 1,
            joined_at: '2026-03-20T09:00:00+08:00',
            is_owner: false,
          },
        ],
      },
    })

    await wrapper.get('[data-testid="remove-member-12"]').trigger('click')

    expect(wrapper.emitted('remove')).toEqual([[12]])
  })

  it('emits add when an operator clicks the add member button', async () => {
    const wrapper = mount(MemberPanel, {
      props: {
        canManage: true,
        members: [],
      },
    })

    await wrapper.get('[data-testid="add-member"]').trigger('click')

    expect(wrapper.emitted('add')).toEqual([[]])
  })
})
