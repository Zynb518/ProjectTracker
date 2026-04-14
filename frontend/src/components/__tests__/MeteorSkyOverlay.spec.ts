import { existsSync, readFileSync } from 'node:fs'
import { resolve } from 'node:path'

describe('MeteorSkyOverlay', () => {
  it('defines a dark-only non-interactive meteor overlay with reduced-motion protection', () => {
    const componentPath = resolve(process.cwd(), 'src/components/backgrounds/MeteorSkyOverlay.vue')

    expect(existsSync(componentPath)).toBe(true)

    const source = readFileSync(componentPath, 'utf8')

    expect(source).toContain("const isDark = computed(() => themeStore.mode === 'dark')")
    expect(source).toContain('data-testid="meteor-sky-overlay"')
    expect(source).toContain('pointer-events: none;')
    expect(source).toContain('@media (prefers-reduced-motion: reduce)')
    expect(source).toContain('animation:')
  })

  it('aligns the meteor streak angle and highlight head with the down-right flight path', () => {
    const source = readFileSync(resolve(process.cwd(), 'src/components/backgrounds/MeteorSkyOverlay.vue'), 'utf8')

    expect(source).toContain('transform: translate3d(0, 0, 0) rotate(24deg) scaleX(0.72);')
    expect(source).toContain('transform-origin: right center;')
    expect(source).toContain('right: -1px;')
    expect(source).not.toContain('rotate(-24deg)')
  })
})
