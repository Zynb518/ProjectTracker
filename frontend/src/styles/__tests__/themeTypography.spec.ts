import { readFileSync } from 'node:fs'
import { resolve } from 'node:path'

describe('theme typography tokens', () => {
  it('uses stronger text contrast and heavier default typography across themes', () => {
    const themeSource = readFileSync(resolve(process.cwd(), 'src/styles/theme.css'), 'utf8')
    const baseSource = readFileSync(resolve(process.cwd(), 'src/styles/base.css'), 'utf8')

    expect(themeSource).toContain('--panel-bg: #f3f7fc;')
    expect(themeSource).toContain('--glass-bg-strong: rgba(239, 245, 252, 0.9);')
    expect(themeSource).toContain('--text-main: #102038;')
    expect(themeSource).toContain('--text-soft: #344963;')
    expect(themeSource).toContain('--panel-bg: #101724;')
    expect(themeSource).toContain('--text-main: #eef4ff;')
    expect(themeSource).toContain('--text-soft: #bcc9dd;')
    expect(themeSource).not.toContain('--panel-bg: #ffffff;')
    expect(themeSource).not.toContain('--glass-bg-strong: rgba(255, 255, 255, 0.92);')

    expect(baseSource).toContain("family=Inter:wght@400;500;600;700")
    expect(baseSource).toContain('line-height: 1.55;')
    expect(baseSource).toContain('font-size: 15px;')
    expect(baseSource).toContain('font-weight: 500;')
    expect(baseSource).toContain('font-weight: 700;')
  })
})
