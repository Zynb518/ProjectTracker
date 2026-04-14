import { readFileSync } from 'node:fs'
import { resolve } from 'node:path'

import { describe, expect, it } from 'vitest'

describe('ThemeToggle', () => {
  it('uses the shared galaxy meta surface instead of the old blur-heavy chip shell', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/ThemeToggle.vue'), 'utf8')

    expect(source).toContain('background: var(--meta-surface-bg), var(--meta-surface-glow), var(--card-sheen);')
    expect(source).toContain('box-shadow: var(--meta-surface-shadow);')
    expect(source).not.toContain('backdrop-filter: blur(16px);')
  })

  it('uses Chinese mode labels so the control language stays consistent with the rest of the interface', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/ThemeToggle.vue'), 'utf8')

    expect(source).toContain("{{ isDark ? '暗色' : '浅色' }}")
    expect(source).not.toContain("{{ isDark ? 'Dark' : 'Light' }}")
  })
})
