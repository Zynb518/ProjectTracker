/**
 * Exports data to CSV and triggers download in browser
 * @param headers CSV header labels, e.g. ['Name', 'Description']
 * @param rows CSV row arrays, e.g. [['Row1Col1', 'Row1Col2']]
 * @param filename File name, e.g. 'data.csv'
 */
export function exportToCSV(headers: string[], rows: string[][], filename: string) {
  const formatCell = (val: any) => {
    if (val === null || val === undefined) {
      return '""'
    }
    const str = String(val)
    const clean = str.replace(/"/g, '""')
    return `"${clean}"`
  }

  const headerLine = headers.map(formatCell).join(',')
  const rowLines = rows.map(row => row.map(formatCell).join(','))
  const csvContent = [headerLine, ...rowLines].join('\n')

  // Use UTF-8 BOM so Excel displays Chinese characters correctly without garbled text
  const BOM = '\uFEFF'
  const blob = new Blob([BOM + csvContent], { type: 'text/csv;charset=utf-8;' })
  const url = URL.createObjectURL(blob)
  
  const link = document.createElement('a')
  link.setAttribute('href', url)
  link.setAttribute('download', filename)
  link.style.visibility = 'hidden'
  document.body.appendChild(link)
  link.click()
  document.body.removeChild(link)
  URL.revokeObjectURL(url)
}
