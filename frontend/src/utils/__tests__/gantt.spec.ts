import { describe, expect, it } from 'vitest'

import {
  buildGanttAxisItems,
  getGanttBarLayout,
  getZoomedGanttScale,
} from '@/utils/gantt'

describe('gantt utils', () => {
  it('builds an inclusive day axis for the project range', () => {
    const items = buildGanttAxisItems('2026-03-20', '2026-03-22', 'day')

    expect(items.map((item) => item.label)).toEqual(['03/20', '03/21', '03/22'])
    expect(items.map((item) => item.widthPx)).toEqual([36, 36, 36])
  })

  it('builds week cells in seven-day chunks from the project start date', () => {
    const items = buildGanttAxisItems('2026-03-20', '2026-04-05', 'week')

    expect(items).toHaveLength(3)
    expect(items.map((item) => item.label)).toEqual(['03/20', '03/27', '04/03'])
    expect(items.map((item) => item.widthPx)).toEqual([84, 84, 36])
  })

  it('builds month cells aligned to calendar months for wider gantt views', () => {
    const items = buildGanttAxisItems('2026-03-20', '2026-05-05', 'month')

    expect(items.map((item) => item.label)).toEqual(['2026-03', '2026-04', '2026-05'])
    expect(items.map((item) => item.widthPx)).toEqual([124, 120, 124])
  })

  it('calculates bar offsets and widths from inclusive day spans', () => {
    const layout = getGanttBarLayout('2026-03-20', '2026-03-26', '2026-04-02', 'week')

    expect(layout.leftPx).toBe(72)
    expect(layout.widthPx).toBe(96)
  })

  it('keeps a single-day item visible in the most detailed scale', () => {
    const layout = getGanttBarLayout('2026-03-20', '2026-03-20', '2026-03-20', 'day')

    expect(layout.leftPx).toBe(0)
    expect(layout.widthPx).toBe(36)
  })

  it('zooms between day, week, and month without leaving the supported bounds', () => {
    expect(getZoomedGanttScale('week', 'in')).toBe('day')
    expect(getZoomedGanttScale('week', 'out')).toBe('month')
    expect(getZoomedGanttScale('day', 'in')).toBe('day')
    expect(getZoomedGanttScale('month', 'out')).toBe('month')
  })
})
