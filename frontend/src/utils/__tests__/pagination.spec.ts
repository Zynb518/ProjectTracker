import { describe, expect, it } from 'vitest'

import { getTotalPages, getVisiblePages } from '@/utils/pagination'

describe('pagination utils', () => {
  it('computes a minimum of one total page', () => {
    expect(getTotalPages(0, 9)).toBe(1)
    expect(getTotalPages(18, 9)).toBe(2)
    expect(getTotalPages(27, 9)).toBe(3)
  })

  it('builds a centered visible page window when possible', () => {
    expect(getVisiblePages(1, 1)).toEqual([1])
    expect(getVisiblePages(1, 6)).toEqual([1, 2, 3, 4, 5])
    expect(getVisiblePages(3, 8)).toEqual([1, 2, 3, 4, 5])
    expect(getVisiblePages(5, 8)).toEqual([3, 4, 5, 6, 7])
    expect(getVisiblePages(8, 8)).toEqual([4, 5, 6, 7, 8])
  })
})
