import type { GanttScale } from '@/types/gantt'

export interface GanttAxisItem {
  key: string
  label: string
  startDate: string
  endDate: string
  widthPx: number
}

export interface GanttBarLayout {
  leftPx: number
  widthPx: number
}

export interface GanttLaneLayout<T> {
  item: T
  laneIndex: number
}

const DAY_MS = 24 * 60 * 60 * 1000

const PIXELS_PER_DAY = {
  day: 64,
  week: 12,
  month: 4,
} as const

const SCALE_ORDER: GanttScale[] = ['day', 'week', 'month']

function parseDate(value: string): Date {
  const [year, month, day] = value.split('-').map(Number)
  return new Date(Date.UTC(year, month - 1, day))
}

function toDateString(value: Date): string {
  return value.toISOString().slice(0, 10)
}

function addDays(value: Date, amount: number): Date {
  return new Date(value.getTime() + amount * DAY_MS)
}

function addMonths(value: Date, amount: number): Date {
  return new Date(Date.UTC(value.getUTCFullYear(), value.getUTCMonth() + amount, 1))
}

function startOfMonth(value: Date): Date {
  return new Date(Date.UTC(value.getUTCFullYear(), value.getUTCMonth(), 1))
}

function endOfMonth(value: Date): Date {
  return new Date(Date.UTC(value.getUTCFullYear(), value.getUTCMonth() + 1, 0))
}

function diffInDays(start: Date, end: Date): number {
  return Math.round((end.getTime() - start.getTime()) / DAY_MS)
}

function getMonthDayLabel(value: Date): string {
  const month = `${value.getUTCMonth() + 1}`.padStart(2, '0')
  const day = `${value.getUTCDate()}`.padStart(2, '0')
  return `${month}/${day}`
}

function getMonthLabel(value: Date): string {
  const month = `${value.getUTCMonth() + 1}`.padStart(2, '0')
  return `${value.getUTCFullYear()}-${month}`
}

function normalizeRange(startDate: string, endDate: string) {
  const parsedStart = parseDate(startDate)
  const parsedEnd = parseDate(endDate)

  if (parsedStart.getTime() <= parsedEnd.getTime()) {
    return { start: parsedStart, end: parsedEnd }
  }

  return { start: parsedEnd, end: parsedStart }
}

interface GanttLaneItemLike {
  planned_start_date: string
  planned_end_date: string
}

export function getPixelsPerDay(scale: GanttScale): number {
  return PIXELS_PER_DAY[scale]
}

export function getZoomedGanttScale(
  currentScale: GanttScale,
  direction: 'in' | 'out',
): GanttScale {
  const currentIndex = SCALE_ORDER.indexOf(currentScale)
  const offset = direction === 'in' ? -1 : 1
  const nextIndex = Math.min(Math.max(currentIndex + offset, 0), SCALE_ORDER.length - 1)

  return SCALE_ORDER[nextIndex]
}

export function buildGanttAxisItems(
  startDate: string,
  endDate: string,
  scale: GanttScale,
): GanttAxisItem[] {
  const { start, end } = normalizeRange(startDate, endDate)
  const pixelsPerDay = getPixelsPerDay(scale)

  if (scale === 'day') {
    const totalDays = diffInDays(start, end)

    return Array.from({ length: totalDays + 1 }, (_, index) => {
      const cellDate = addDays(start, index)

      return {
        key: toDateString(cellDate),
        label: getMonthDayLabel(cellDate),
        startDate: toDateString(cellDate),
        endDate: toDateString(cellDate),
        widthPx: pixelsPerDay,
      }
    })
  }

  if (scale === 'week') {
    const items: GanttAxisItem[] = []
    let cursor = start

    while (cursor.getTime() <= end.getTime()) {
      const cellEnd = addDays(cursor, 6)

      items.push({
        key: `${toDateString(cursor)}:${toDateString(cellEnd)}`,
        label: getMonthDayLabel(cursor),
        startDate: toDateString(cursor),
        endDate: toDateString(cellEnd),
        widthPx: 7 * pixelsPerDay,
      })

      cursor = addDays(cursor, 7)
    }

    return items
  }

  const axisStart = startOfMonth(start)
  const axisEnd = endOfMonth(end)
  const items: GanttAxisItem[] = []
  let cursor = axisStart

  while (cursor.getTime() <= axisEnd.getTime()) {
    const cellEnd = endOfMonth(cursor)
    const spanDays = diffInDays(cursor, cellEnd) + 1

    items.push({
      key: `${cursor.getUTCFullYear()}-${cursor.getUTCMonth() + 1}`,
      label: getMonthLabel(cursor),
      startDate: toDateString(cursor),
      endDate: toDateString(cellEnd),
      widthPx: spanDays * pixelsPerDay,
    })

    cursor = addMonths(cursor, 1)
  }

  return items
}

export function getGanttBarLayout(
  axisStartDate: string,
  itemStartDate: string,
  itemEndDate: string,
  scale: GanttScale,
): GanttBarLayout {
  const axisStart = parseDate(axisStartDate)
  const { start, end } = normalizeRange(itemStartDate, itemEndDate)
  const clampedStart = new Date(Math.max(axisStart.getTime(), start.getTime()))
  const leftDays = diffInDays(axisStart, clampedStart)
  const spanDays = diffInDays(clampedStart, end) + 1
  const pixelsPerDay = getPixelsPerDay(scale)

  return {
    leftPx: Math.max(0, leftDays) * pixelsPerDay,
    widthPx: Math.max(1, spanDays) * pixelsPerDay,
  }
}

export function packGanttLaneItems<T extends GanttLaneItemLike>(items: T[]): Array<GanttLaneLayout<T>> {
  const laneEndTimes: number[] = []

  return items.map((item) => {
    const { start, end } = normalizeRange(item.planned_start_date, item.planned_end_date)
    let laneIndex = laneEndTimes.findIndex((endTime) => start.getTime() > endTime)

    if (laneIndex === -1) {
      laneIndex = laneEndTimes.length
      laneEndTimes.push(end.getTime())
    } else {
      laneEndTimes[laneIndex] = end.getTime()
    }

    return {
      item,
      laneIndex,
    }
  })
}
