import { createPinia, setActivePinia } from 'pinia'
import { render, waitFor } from '@testing-library/vue'
import userEvent from '@testing-library/user-event'
import { beforeEach, describe, expect, it, vi } from 'vitest'

vi.mock('@/api/users', async () => {
  const actual = await vi.importActual<typeof import('@/api/users')>('@/api/users')

  return {
    ...actual,
    createUser: vi.fn(),
    getUserDetail: vi.fn(),
    listUsers: vi.fn(),
    updateUser: vi.fn(),
    updateUserStatus: vi.fn(),
  }
})

import { createUser, getUserDetail, listUsers, updateUser, updateUserStatus } from '@/api/users'
import UserManagementView from '@/views/UserManagementView.vue'

const userListResponse = {
  list: [
    {
      id: 11,
      username: 'admin',
      real_name: '系统管理员',
      system_role: 1,
      email: 'admin@example.com',
      phone: '13800000000',
      status: 1,
      created_at: '2026-03-27T10:00:00+08:00',
      updated_at: '2026-03-27T10:00:00+08:00',
    },
    {
      id: 12,
      username: 'lisi',
      real_name: '李四',
      system_role: 2,
      email: 'lisi@example.com',
      phone: '13800000001',
      status: 2,
      created_at: '2026-03-27T10:00:00+08:00',
      updated_at: '2026-03-27T10:00:00+08:00',
    },
  ],
  total: 12,
  page: 1,
  page_size: 10,
}

describe('UserManagementView', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
    vi.resetAllMocks()
    vi.mocked(listUsers).mockResolvedValue(userListResponse)
    vi.mocked(createUser).mockResolvedValue({
      id: 18,
      username: 'wangwu',
      real_name: '王五',
      system_role: 3,
      email: 'wangwu@example.com',
      phone: '13800000008',
      status: 1,
      created_at: '2026-03-27T11:00:00+08:00',
      updated_at: '2026-03-27T11:00:00+08:00',
    })
    vi.mocked(getUserDetail).mockResolvedValue({
      id: 12,
      username: 'lisi',
      real_name: '李四',
      system_role: 2,
      email: 'lisi@example.com',
      phone: '13800000001',
      status: 2,
      created_at: '2026-03-27T10:00:00+08:00',
      updated_at: '2026-03-27T10:00:00+08:00',
    })
    vi.mocked(updateUser).mockResolvedValue({
      id: 12,
      username: 'lisi',
      real_name: '李四-更新',
      system_role: 2,
      email: 'lisi-updated@example.com',
      phone: '13800000009',
      status: 2,
      updated_at: '2026-03-27T12:00:00+08:00',
    })
    vi.mocked(updateUserStatus).mockResolvedValue({
      id: 12,
      status: 1,
      updated_at: '2026-03-27T12:30:00+08:00',
    })
  })

  it('loads users and re-queries when filters or pages change', async () => {
    const screen = render(UserManagementView, {
      global: {
        plugins: [createPinia()],
      },
    })
    const user = userEvent.setup()

    await waitFor(() => {
      expect(listUsers).toHaveBeenCalledWith({
        keyword: undefined,
        page: 1,
        page_size: 10,
        status: undefined,
        system_role: undefined,
      })
    })

    expect(await screen.findByText('系统管理员')).toBeTruthy()
    expect(screen.getByText('李四')).toBeTruthy()

    await user.type(screen.getByLabelText('搜索用户'), '李')
    await user.keyboard('{Enter}')

    await waitFor(() => {
      expect(listUsers).toHaveBeenLastCalledWith({
        keyword: '李',
        page: 1,
        page_size: 10,
        status: undefined,
        system_role: undefined,
      })
    })

    await user.selectOptions(screen.getByTestId('user-management-role'), '2')

    await waitFor(() => {
      expect(listUsers).toHaveBeenLastCalledWith({
        keyword: '李',
        page: 1,
        page_size: 10,
        status: undefined,
        system_role: 2,
      })
    })

    await user.click(screen.getByRole('button', { name: '尾页' }))

    await waitFor(() => {
      expect(listUsers).toHaveBeenLastCalledWith({
        keyword: '李',
        page: 2,
        page_size: 10,
        status: undefined,
        system_role: 2,
      })
    })
  })

  it('creates a user and refreshes the list', async () => {
    const screen = render(UserManagementView, {
      global: {
        plugins: [createPinia()],
      },
    })
    const user = userEvent.setup()

    await screen.findByText('系统管理员')

    vi.mocked(listUsers)
      .mockResolvedValueOnce(userListResponse)
      .mockResolvedValueOnce(userListResponse)

    await user.click(screen.getByRole('button', { name: '新建用户' }))
    await user.type(screen.getByLabelText('登录用户名'), 'wangwu')
    await user.type(screen.getByLabelText('初始密码'), '123456')
    await user.type(screen.getByLabelText('真实姓名'), '王五')
    await user.selectOptions(screen.getByLabelText('系统角色'), '3')
    await user.type(screen.getByLabelText('邮箱'), 'wangwu@example.com')
    await user.type(screen.getByLabelText('手机号'), '13800000008')
    await user.click(screen.getByRole('button', { name: '创建用户' }))

    await waitFor(() => {
      expect(createUser).toHaveBeenCalledWith({
        email: 'wangwu@example.com',
        password: '123456',
        phone: '13800000008',
        real_name: '王五',
        system_role: 3,
        username: 'wangwu',
      })
    })

    await waitFor(() => {
      expect(listUsers).toHaveBeenCalledTimes(2)
    })
  })

  it('updates user info and toggles user status', async () => {
    const screen = render(UserManagementView, {
      global: {
        plugins: [createPinia()],
      },
    })
    const user = userEvent.setup()

    await screen.findByText('系统管理员')

    vi.mocked(listUsers)
      .mockResolvedValueOnce(userListResponse)
      .mockResolvedValueOnce(userListResponse)
      .mockResolvedValueOnce(userListResponse)

    await user.click(screen.getByRole('button', { name: '编辑用户 李四' }))

    await waitFor(() => {
      expect(getUserDetail).toHaveBeenCalledWith(12)
    })

    const realNameInput = screen.getByLabelText('真实姓名')
    await user.clear(realNameInput)
    await user.type(realNameInput, '李四-更新')

    const emailInput = screen.getByLabelText('邮箱')
    await user.clear(emailInput)
    await user.type(emailInput, 'lisi-updated@example.com')
    await user.click(screen.getByRole('button', { name: '保存修改' }))

    await waitFor(() => {
      expect(updateUser).toHaveBeenCalledWith(12, {
        email: 'lisi-updated@example.com',
        phone: '13800000001',
        real_name: '李四-更新',
        system_role: 2,
      })
    })

    await user.click(screen.getByRole('button', { name: '启用用户 李四' }))

    await waitFor(() => {
      expect(updateUserStatus).toHaveBeenCalledWith(12, {
        status: 1,
      })
    })

    await waitFor(() => {
      expect(listUsers).toHaveBeenCalledTimes(3)
    })
  })
})
