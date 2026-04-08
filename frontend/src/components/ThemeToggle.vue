<script setup lang="ts">
import { computed } from 'vue'

import { useThemeStore } from '@/stores/theme'

const themeStore = useThemeStore()

const isDark = computed(() => themeStore.mode === 'dark')
const buttonLabel = computed(() => (isDark.value ? '切换浅色主题' : '切换深色主题'))
</script>

<template>
  <button
    :aria-label="buttonLabel"
    :title="buttonLabel"
    class="theme-toggle"
    data-testid="theme-toggle"
    type="button"
    @click="themeStore.toggleTheme()"
  >
    <span class="theme-toggle__track">
      <span class="theme-toggle__thumb">
        <svg
          v-if="isDark"
          aria-hidden="true"
          class="theme-toggle__icon"
          viewBox="0 0 24 24"
        >
          <path
            d="M21 12.8A9 9 0 1 1 11.2 3c-.1.4-.2.9-.2 1.4a8.4 8.4 0 0 0 8.4 8.4c.5 0 1 0 1.6-.2Z"
            fill="none"
            stroke="currentColor"
            stroke-linecap="round"
            stroke-linejoin="round"
            stroke-width="1.5"
          />
        </svg>
        <svg v-else aria-hidden="true" class="theme-toggle__icon" viewBox="0 0 24 24">
          <circle
            cx="12"
            cy="12"
            r="4"
            fill="none"
            stroke="currentColor"
            stroke-width="1.5"
          />
          <path
            d="M12 2.5v2.2M12 19.3v2.2M21.5 12h-2.2M4.7 12H2.5M18.7 5.3l-1.6 1.6M6.9 17.1l-1.6 1.6M18.7 18.7l-1.6-1.6M6.9 6.9 5.3 5.3"
            fill="none"
            stroke="currentColor"
            stroke-linecap="round"
            stroke-width="1.5"
          />
        </svg>
      </span>
    </span>
    <span class="theme-toggle__label">{{ isDark ? 'Dark' : 'Light' }}</span>
  </button>
</template>

<style scoped>
.theme-toggle {
  display: inline-flex;
  align-items: center;
  gap: 10px;
  border: 1px solid var(--border-soft);
  border-radius: 999px;
  padding: 6px 8px 6px 6px;
  background: var(--glass-bg-strong);
  color: var(--text-main);
  box-shadow: var(--shadow-panel);
  backdrop-filter: blur(16px);
  cursor: pointer;
  transition:
    transform 200ms ease-out,
    border-color 200ms ease-out,
    background-color 200ms ease-out,
    box-shadow 200ms ease-out,
    color 200ms ease-out;
}

.theme-toggle:hover {
  transform: translateY(-2px);
  border-color: var(--accent-line);
  box-shadow: var(--shadow-panel-hover);
}

.theme-toggle__track {
  display: inline-flex;
  width: 40px;
  height: 24px;
  padding: 2px;
  border-radius: 999px;
  background: var(--gradient-primary-soft);
}

.theme-toggle__thumb {
  display: inline-grid;
  place-items: center;
  width: 20px;
  height: 20px;
  border-radius: 999px;
  background: rgba(255, 255, 255, 0.9);
  color: #0a66ff;
  transform: translateX(v-bind('isDark ? "16px" : "0px"'));
  transition: transform 200ms ease-out;
}

.theme-toggle__icon {
  width: 14px;
  height: 14px;
}

.theme-toggle__label {
  font-size: 0.82rem;
  font-weight: 600;
  letter-spacing: 0.04em;
  text-transform: uppercase;
}
</style>
