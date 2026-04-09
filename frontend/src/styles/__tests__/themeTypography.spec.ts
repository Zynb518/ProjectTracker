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
    expect(themeSource).toContain('--app-bg: #0b0a1a;')
    expect(themeSource).toContain('--panel-bg: #121129;')
    expect(themeSource).toContain('--text-main: #e0e0ff;')
    expect(themeSource).toContain('--text-soft: #8a8fbf;')
    expect(themeSource).toContain('--accent-start: #00f0ff;')
    expect(themeSource).toContain('--accent-end: #9d00ff;')
    expect(themeSource).toContain('--accent-tertiary: #ff007f;')
    expect(themeSource).toContain('repeating-linear-gradient(')
    expect(themeSource).not.toContain('--panel-bg: #ffffff;')
    expect(themeSource).not.toContain('--glass-bg-strong: rgba(255, 255, 255, 0.92);')

    expect(baseSource).toContain("family=Noto+Sans+SC:wght@400;500;600;700")
    expect(baseSource).toContain('line-height: 1.55;')
    expect(baseSource).toContain('font-size: 15px;')
    expect(baseSource).toContain('font-weight: 500;')
    expect(baseSource).toContain('font-weight: 700;')
    expect(baseSource).toContain("font-family: 'Noto Sans SC', 'PingFang SC', 'Hiragino Sans GB', 'Microsoft YaHei', 'Segoe UI', sans-serif;")
    expect(baseSource).toContain('font-synthesis: weight style;')
  })
})
