export function getTotalPages(total: number, pageSize: number): number {
  if (pageSize <= 0) {
    return 1
  }

  return Math.max(1, Math.ceil(total / pageSize))
}

export function getVisiblePages(
  currentPage: number,
  totalPages: number,
  windowSize = 5,
): number[] {
  const safeTotalPages = Math.max(1, totalPages)
  const safeCurrentPage = Math.min(Math.max(currentPage, 1), safeTotalPages)
  const safeWindowSize = Math.max(1, windowSize)
  const start = Math.max(1, safeCurrentPage - Math.floor(safeWindowSize / 2))
  const end = Math.min(safeTotalPages, start + safeWindowSize - 1)
  const normalizedStart = Math.max(1, end - safeWindowSize + 1)

  return Array.from({ length: end - normalizedStart + 1 }, (_, index) => normalizedStart + index)
}
