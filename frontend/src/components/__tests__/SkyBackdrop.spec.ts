import { existsSync, readFileSync } from 'node:fs'
import { resolve } from 'node:path'

describe('SkyBackdrop', () => {
  it('defines a fixed dark-only viewport backdrop for the shared sky image', () => {
    const componentPath = resolve(process.cwd(), 'src/components/backgrounds/SkyBackdrop.vue')

    expect(existsSync(componentPath)).toBe(true)

    const source = readFileSync(componentPath, 'utf8')

    expect(source).toContain("const isDark = computed(() => themeStore.mode === 'dark')")
    expect(source).toContain('data-testid="sky-backdrop"')
    expect(source).toContain("url('../../assets/backgrounds/night-sky.jpg')")
    expect(source).toContain('position: fixed;')
    expect(source).toContain('inset: 0;')
    expect(source).toContain('pointer-events: none;')
  })
})
