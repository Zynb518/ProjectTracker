import { readFileSync } from 'node:fs'
import { resolve } from 'node:path'

import { describe, expect, it } from 'vitest'

describe('ProjectAiDraftDialog', () => {
  it('uses the same native smooth-scroll shell as the stage node rail instead of JS virtual window bookkeeping', () => {
    const source = readFileSync(
      resolve(process.cwd(), 'src/components/projects/ProjectAiDraftDialog.vue'),
      'utf8',
    )

    expect(source).toContain('v-smooth-wheel')
    expect(source).toContain('class="project-ai-dialog__tree-list smooth-scroll-surface"')
    expect(source).not.toContain('resolveVirtualWindow')
    expect(source).not.toContain('treeListResizeObserver')
    expect(source).not.toContain('project-ai-dialog__tree-list-rail')
  })

  it('skips painting offscreen draft cards and uses the lighter shared card surface from the node rail', () => {
    const source = readFileSync(
      resolve(process.cwd(), 'src/components/projects/ProjectAiDraftDialog.vue'),
      'utf8',
    )

    expect(source).toContain('.project-ai-dialog__tree-node {')
    expect(source).toContain('background: var(--dialog-control-bg), var(--card-sheen);')
    expect(source).toContain('content-visibility: auto;')
    expect(source).toContain('contain-intrinsic-size: 232px;')
    expect(source).toContain('.project-ai-dialog__tree-subtask {')
    expect(source).toContain('contain-intrinsic-size: 40px;')
  })

  it('keeps the stage header as the visual card while making the outer subtask group wrapper transparent', () => {
    const source = readFileSync(
      resolve(process.cwd(), 'src/components/projects/ProjectAiDraftDialog.vue'),
      'utf8',
    )

    expect(source).toContain('.project-ai-dialog__tree-node {')
    expect(source).toContain('background: transparent;')
    expect(source).toContain('border: 0;')
    expect(source).toContain('.project-ai-dialog__tree-node-button {')
    expect(source).toContain('border: 1px solid color-mix(in srgb, var(--accent-line) 16%, transparent);')
    expect(source).toContain('border-radius: 18px;')
    expect(source).toContain('box-shadow: var(--dialog-control-shadow);')
    expect(source).toContain('.project-ai-dialog__tree-subtasks {')
    expect(source).toContain('box-shadow: none;')
  })
})
