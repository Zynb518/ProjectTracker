import { defineStore } from 'pinia'

export type ThemeMode = 'light' | 'dark'

const STORAGE_KEY = 'project-tracker-theme'

function resolvePreferredTheme(): ThemeMode {
  if (typeof window !== 'undefined' && typeof window.matchMedia === 'function') {
    return window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light'
  }

  return 'light'
}

export const useThemeStore = defineStore('theme', {
  state: () => ({
    mode: 'light' as ThemeMode,
    initialized: false,
  }),
  actions: {
    applyTheme() {
      if (typeof document === 'undefined') {
        return
      }

      document.documentElement.classList.remove('light', 'dark')
      document.documentElement.classList.add(this.mode)
      document.documentElement.dataset.theme = this.mode
    },
    initializeTheme() {
      if (this.initialized) {
        this.applyTheme()
        return
      }

      if (typeof window !== 'undefined') {
        const stored = window.localStorage.getItem(STORAGE_KEY)

        if (stored === 'light' || stored === 'dark') {
          this.mode = stored
        } else {
          this.mode = resolvePreferredTheme()
        }
      } else {
        this.mode = 'light'
      }

      this.initialized = true
      this.applyTheme()
    },
    setTheme(mode: ThemeMode) {
      this.mode = mode
      this.initialized = true

      if (typeof window !== 'undefined') {
        window.localStorage.setItem(STORAGE_KEY, mode)
      }

      this.applyTheme()
    },
    toggleTheme() {
      this.setTheme(this.mode === 'dark' ? 'light' : 'dark')
    },
  },
})
