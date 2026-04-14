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
    expect(themeSource).toContain('--app-bg: #060916;')
    expect(themeSource).toContain('--panel-bg: #0d1323;')
    expect(themeSource).toContain('--text-main: #ecf3ff;')
    expect(themeSource).toContain('--text-soft: #93a6c7;')
    expect(themeSource).toContain('--accent-start: #7cd3ff;')
    expect(themeSource).toContain('--accent-end: #8d78ff;')
    expect(themeSource).toContain('--accent-tertiary: #d9abff;')
    expect(themeSource).not.toContain('repeating-linear-gradient(')
    expect(themeSource).not.toContain('animation: scanline-drift')
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

  it('defines a shared semantic work-status palette with blue active states in both themes', () => {
    const themeSource = readFileSync(resolve(process.cwd(), 'src/styles/theme.css'), 'utf8')

    expect(themeSource).toContain('--work-status-pending-color:')
    expect(themeSource).toContain('--work-status-active-color: #0a66ff;')
    expect(themeSource).toContain('--work-status-done-color: var(--accent-success);')
    expect(themeSource).toContain('--work-status-delayed-color: var(--accent-warning);')
    expect(themeSource).toContain('--work-status-active-strong: #0a66ff;')
    expect(themeSource).toContain('--work-status-active-color: #67b7ff;')
    expect(themeSource).toContain('--work-status-active-strong: #67b7ff;')
  })
})
