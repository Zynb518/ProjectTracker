import { readFileSync } from 'node:fs'
import { resolve } from 'node:path'

describe('theme typography tokens', () => {
  it('uses stronger text contrast and heavier default typography across themes', () => {
    const themeSource = readFileSync(resolve(process.cwd(), 'src/styles/theme.css'), 'utf8')
    const baseSource = readFileSync(resolve(process.cwd(), 'src/styles/base.css'), 'utf8')

    expect(themeSource).toContain('--text-main: #16233d;')
    expect(themeSource).toContain('--text-soft: #43546d;')
    expect(themeSource).toContain('--text-main: #f3f7ff;')
    expect(themeSource).toContain('--text-soft: #a8b5ca;')

    expect(baseSource).toContain("family=Inter:wght@400;500;600;700")
    expect(baseSource).toContain('font-weight: 500;')
    expect(baseSource).toContain('font-weight: 700;')
  })
})
