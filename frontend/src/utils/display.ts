const WORK_STATUS_LABELS = {
  1: '未开始',
  2: '进行中',
  3: '已完成',
  4: '已延期',
} as const

const WORK_STATUS_TONES = {
  1: 'pending',
  2: 'active',
  3: 'done',
  4: 'delayed',
} as const

const SYSTEM_ROLE_LABELS = {
  1: '管理员',
  2: '项目经理',
  3: '普通员工',
} as const

const SYSTEM_ROLE_TONES = {
  1: 'admin',
  2: 'manager',
  3: 'member',
} as const

const ACCOUNT_STATUS_LABELS = {
  1: '启用',
  2: '禁用',
} as const

const ACCOUNT_STATUS_TONES = {
  1: 'enabled',
  2: 'disabled',
} as const

export function getWorkStatusLabel(status: number): string {
  return WORK_STATUS_LABELS[status as keyof typeof WORK_STATUS_LABELS] ?? '未知状态'
}

export function getWorkStatusTone(status: number): string {
  return WORK_STATUS_TONES[status as keyof typeof WORK_STATUS_TONES] ?? 'unknown'
}

export function getSystemRoleLabel(systemRole?: number): string {
  return SYSTEM_ROLE_LABELS[(systemRole ?? 0) as keyof typeof SYSTEM_ROLE_LABELS] ?? '访客'
}

export function getSystemRoleTone(systemRole?: number): string {
  return SYSTEM_ROLE_TONES[(systemRole ?? 0) as keyof typeof SYSTEM_ROLE_TONES] ?? 'member'
}

export function getAccountStatusLabel(status: number): string {
  return ACCOUNT_STATUS_LABELS[status as keyof typeof ACCOUNT_STATUS_LABELS] ?? '未知状态'
}

export function getAccountStatusTone(status: number): string {
  return ACCOUNT_STATUS_TONES[status as keyof typeof ACCOUNT_STATUS_TONES] ?? 'unknown'
}

export function formatDisplayDateTime(value: string): string {
  return value.replace('T', ' ').slice(0, 16)
}

export function calculateProgressPercent(completed: number, total: number): number {
  if (total <= 0) {
    return 0
  }

  return Math.max(0, Math.min(100, Math.round((completed / total) * 100)))
}
