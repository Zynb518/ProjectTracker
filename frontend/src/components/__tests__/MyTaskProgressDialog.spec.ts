import { readFileSync } from 'node:fs'
import { resolve } from 'node:path'

import { describe, expect, it } from 'vitest'

describe('MyTaskProgressDialog', () => {
  it('uses the galaxy dialog stack for the shell and nested editor surfaces without the old inner blur panel', () => {
    const source = readFileSync(
      resolve(process.cwd(), 'src/components/my-tasks/MyTaskProgressDialog.vue'),
      'utf8',
    )

    expect(source).toContain('background: var(--dialog-surface-glow), var(--dialog-surface-bg), var(--card-sheen);')
    expect(source).toContain('background: var(--meta-surface-bg), var(--meta-surface-glow), var(--card-sheen);')
    expect(source).toContain('backdrop-filter: blur(12px);')
    expect(source).not.toContain('backdrop-filter: var(--backdrop-blur);')
  })
})
