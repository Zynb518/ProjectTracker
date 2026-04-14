import { readFileSync } from 'node:fs'
import { resolve } from 'node:path'
import { render } from '@testing-library/vue'
import { describe, expect, it } from 'vitest'

import ProjectHero from '@/components/workspace/ProjectHero.vue'
import type { ProjectDetail } from '@/types/project'

const sampleProject: ProjectDetail = {
  id: 1001,
  name: '内部进度平台',
  description: '用于内部项目进度跟踪',
  owner_user_id: 1,
  owner_real_name: '张三',
  status: 2,
  planned_start_date: '2026-03-20',
  planned_end_date: '2026-06-30',
  completed_at: null,
  created_by: 1,
  created_by_real_name: '张三',
  created_at: '2026-03-19T11:00:00+08:00',
  updated_at: '2026-03-27T09:30:00+08:00',
  member_count: 5,
  node_count: 3,
  completed_node_count: 1,
  sub_task_count: 12,
  completed_sub_task_count: 4,
  permissions: {
    can_edit_basic: true,
    can_manage_members: true,
    can_manage_nodes: true,
    can_transfer_owner: true,
    can_delete: true,
  },
}

describe('ProjectHero', () => {
  it('uses larger, higher-contrast typography for overview copy and labels', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectHero.vue'), 'utf8')

    expect(source).toContain('font-size: 0.82rem;')
    expect(source).toContain('color: color-mix(in srgb, var(--text-main) 78%, var(--text-soft));')
    expect(source).toContain('font-size: 1rem;')
  })

  it('uses valid surface backgrounds instead of mixing gradient tokens through color-mix', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectHero.vue'), 'utf8')

    expect(source).toContain('background: var(--project-card-bg), var(--project-card-glow);')
    expect(source).toContain('background: var(--meta-surface-bg), var(--meta-surface-glow), var(--card-sheen);')
    expect(source).not.toContain('color-mix(in srgb, var(--project-card-bg)')
    expect(source).not.toContain('backdrop-filter: var(--backdrop-blur);')
  })

  it('uses the shared semantic work-status tokens for the project status pill instead of the brand gradient', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/workspace/ProjectHero.vue'), 'utf8')

    expect(source).toContain('background: var(--work-status-active-bg);')
    expect(source).toContain('color: var(--work-status-active-color);')
    expect(source).toContain('background: var(--work-status-done-bg);')
    expect(source).toContain('color: var(--work-status-done-color);')
    expect(source).not.toContain(".project-overview-card__status--active {\n  background: var(--gradient-primary);")
  })

  it('renders member summary, two circular progress indicators, and one grouped metadata panel', () => {
    const screen = render(ProjectHero, {
      props: {
        project: sampleProject,
      },
    })

    expect(screen.getByTestId('project-overview-card')).toBeTruthy()
    expect(screen.getByTestId('member-summary')).toBeTruthy()
    expect(screen.getByTestId('node-progress-ring')).toBeTruthy()
    expect(screen.getByTestId('subtask-progress-ring')).toBeTruthy()
    expect(screen.getByTestId('project-meta-panel')).toBeTruthy()
    expect(screen.getByText('1 / 3')).toBeTruthy()
    expect(screen.getByText('4 / 12')).toBeTruthy()
    expect(screen.getByText('张三')).toBeTruthy()
    expect(screen.getByText('2026-03-20')).toBeTruthy()
    expect(screen.getByText('2026-06-30')).toBeTruthy()
  })
})
