import { describe, expect, it } from 'vitest'

import {
  buildGanttAxisItems,
  getGanttBarLayout,
  getZoomedGanttScale,
  packGanttLaneItems,
} from '@/utils/gantt'

describe('gantt utils', () => {
  it('builds an inclusive day axis with padded labels sized for fixed MM/DD copy', () => {
    const items = buildGanttAxisItems('2026-03-20', '2026-03-22', 'day')

    expect(items.map((item) => item.label)).toEqual(['03/20', '03/21', '03/22'])
    expect(items.map((item) => item.widthPx)).toEqual([64, 64, 64])
  })

  it('builds week cells in seven-day chunks from the project start date', () => {
    const items = buildGanttAxisItems('2026-03-20', '2026-04-05', 'week')

    expect(items).toHaveLength(3)
    expect(items.map((item) => item.label)).toEqual(['03/20', '03/27', '04/03'])
    expect(items.map((item) => item.endDate)).toEqual(['2026-03-26', '2026-04-02', '2026-04-09'])
    expect(items.map((item) => item.widthPx)).toEqual([84, 84, 84])
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
    expect(layout.widthPx).toBe(64)
  })

  it('zooms between day, week, and month without leaving the supported bounds', () => {
    expect(getZoomedGanttScale('week', 'in')).toBe('day')
    expect(getZoomedGanttScale('week', 'out')).toBe('month')
    expect(getZoomedGanttScale('day', 'in')).toBe('day')
    expect(getZoomedGanttScale('month', 'out')).toBe('month')
  })

  it('packs overlapping items into separate lanes while keeping input order stable', () => {
    const layouts = packGanttLaneItems([
      {
        id: 'a',
        planned_start_date: '2026-03-27',
        planned_end_date: '2026-03-31',
      },
      {
        id: 'b',
        planned_start_date: '2026-03-29',
        planned_end_date: '2026-04-02',
      },
      {
        id: 'c',
        planned_start_date: '2026-04-03',
        planned_end_date: '2026-04-04',
      },
    ])

    expect(layouts.map((layout) => [layout.item.id, layout.laneIndex])).toEqual([
      ['a', 0],
      ['b', 1],
      ['c', 0],
    ])
  })

  it('reuses the same lane when items do not overlap in time', () => {
    const layouts = packGanttLaneItems([
      {
        id: 'a',
        planned_start_date: '2026-03-20',
        planned_end_date: '2026-03-21',
      },
      {
        id: 'b',
        planned_start_date: '2026-03-22',
        planned_end_date: '2026-03-23',
      },
      {
        id: 'c',
        planned_start_date: '2026-03-24',
        planned_end_date: '2026-03-26',
      },
    ])

    expect(layouts.every((layout) => layout.laneIndex === 0)).toBe(true)
  })
})
