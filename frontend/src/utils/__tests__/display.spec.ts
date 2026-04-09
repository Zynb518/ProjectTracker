import { describe, expect, it } from 'vitest'

import {
  calculateProgressPercent,
  getAccountStatusLabel,
  getAccountStatusTone,
  getSystemRoleLabel,
  getSystemRoleTone,
  getWorkStatusLabel,
  getWorkStatusTone,
} from '@/utils/display'

describe('display utils', () => {
  it('maps shared work statuses to labels and tones', () => {
    expect(getWorkStatusLabel(1)).toBe('未开始')
    expect(getWorkStatusLabel(2)).toBe('进行中')
    expect(getWorkStatusLabel(3)).toBe('已完成')
    expect(getWorkStatusLabel(4)).toBe('已延期')
    expect(getWorkStatusLabel(999)).toBe('未知状态')

    expect(getWorkStatusTone(1)).toBe('pending')
    expect(getWorkStatusTone(2)).toBe('active')
    expect(getWorkStatusTone(3)).toBe('done')
    expect(getWorkStatusTone(4)).toBe('delayed')
    expect(getWorkStatusTone(999)).toBe('unknown')
  })

  it('maps system roles and account statuses', () => {
    expect(getSystemRoleLabel(1)).toBe('管理员')
    expect(getSystemRoleLabel(2)).toBe('项目经理')
    expect(getSystemRoleLabel(3)).toBe('普通员工')
    expect(getSystemRoleLabel()).toBe('访客')

    expect(getSystemRoleTone(1)).toBe('admin')
    expect(getSystemRoleTone(2)).toBe('manager')
    expect(getSystemRoleTone(3)).toBe('member')
    expect(getSystemRoleTone()).toBe('member')

    expect(getAccountStatusLabel(1)).toBe('启用')
    expect(getAccountStatusLabel(2)).toBe('禁用')
    expect(getAccountStatusLabel(999)).toBe('未知状态')

    expect(getAccountStatusTone(1)).toBe('enabled')
    expect(getAccountStatusTone(2)).toBe('disabled')
    expect(getAccountStatusTone(999)).toBe('unknown')
  })

  it('calculates progress percentages safely', () => {
    expect(calculateProgressPercent(0, 0)).toBe(0)
    expect(calculateProgressPercent(2, 6)).toBe(33)
    expect(calculateProgressPercent(6, 6)).toBe(100)
    expect(calculateProgressPercent(9, 6)).toBe(100)
    expect(calculateProgressPercent(-2, 6)).toBe(0)
  })
})
