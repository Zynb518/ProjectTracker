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
  position: relative;
  isolation: isolate;
  overflow: hidden;
  display: inline-flex;
  align-items: center;
  gap: 10px;
  border: 1px solid var(--meta-surface-border);
  border-radius: 999px;
  padding: 6px 8px 6px 6px;
  background: var(--meta-surface-bg), var(--meta-surface-glow), var(--card-sheen);
  color: var(--text-main);
  box-shadow: var(--meta-surface-shadow);
  cursor: pointer;
  transition:
    transform 200ms ease-out,
    border-color 200ms ease-out,
    background 200ms ease-out,
    box-shadow 200ms ease-out,
    color 200ms ease-out;
}

.theme-toggle:hover {
  transform: translateY(-2px);
  border-color: var(--accent-line);
  box-shadow:
    var(--meta-surface-shadow),
    0 16px 28px color-mix(in srgb, var(--accent-end) 14%, transparent);
}

.theme-toggle__track {
  display: inline-flex;
  width: 40px;
  height: 24px;
  padding: 2px;
  border-radius: 999px;
  background: linear-gradient(180deg, color-mix(in srgb, var(--panel-bg) 96%, transparent), color-mix(in srgb, var(--panel-bg-soft) 84%, transparent));
  box-shadow:
    inset 0 0 0 1px color-mix(in srgb, var(--dialog-control-border) 94%, transparent),
    inset 0 1px 0 color-mix(in srgb, var(--text-inverse) 12%, transparent);
}

.theme-toggle__thumb {
  display: inline-grid;
  place-items: center;
  width: 20px;
  height: 20px;
  border-radius: 999px;
  background: linear-gradient(
    180deg,
    color-mix(in srgb, var(--text-inverse) 86%, var(--accent-start) 14%),
    color-mix(in srgb, var(--text-inverse) 72%, var(--panel-bg) 28%)
  );
  color: color-mix(in srgb, var(--accent-end) 82%, var(--accent-start));
  transform: translateX(v-bind('isDark ? "16px" : "0px"'));
  box-shadow:
    0 8px 18px color-mix(in srgb, var(--accent-start) 16%, transparent),
    0 0 0 1px color-mix(in srgb, var(--accent-line) 44%, transparent);
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

:global(html.dark) .theme-toggle {
  border-color: color-mix(in srgb, var(--accent-start) 18%, var(--meta-surface-border));
  box-shadow:
    var(--meta-surface-shadow),
    0 0 20px rgba(124, 211, 255, 0.08);
}

:global(html.dark) .theme-toggle__thumb {
  color: var(--accent-start);
  box-shadow:
    0 8px 18px rgba(124, 211, 255, 0.16),
    0 0 0 1px rgba(124, 211, 255, 0.12);
}

:global(html.dark) .theme-toggle__label {
  color: color-mix(in srgb, var(--text-main) 92%, var(--accent-start) 8%);
}
</style>
