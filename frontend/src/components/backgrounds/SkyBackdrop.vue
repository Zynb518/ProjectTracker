<script setup lang="ts">
import { computed } from 'vue'

import { useThemeStore } from '@/stores/theme'

const themeStore = useThemeStore()
const isDark = computed(() => themeStore.mode === 'dark')
</script>

<template>
  <div v-if="isDark" class="sky-backdrop" data-testid="sky-backdrop" aria-hidden="true" />
</template>

<style scoped>
.sky-backdrop {
  position: fixed;
  inset: 0;
  z-index: 0;
  pointer-events: none;
  background:
    linear-gradient(180deg, rgba(2, 5, 13, 0.24), rgba(4, 7, 16, 0.42), rgba(6, 10, 20, 0.58)),
    radial-gradient(circle at 18% 18%, rgba(124, 211, 255, 0.12), transparent 24%),
    radial-gradient(circle at 82% 14%, rgba(141, 120, 255, 0.1), transparent 22%),
    url('../../assets/backgrounds/night-sky.jpg');
  background-position: center center;
  background-size: auto, auto, auto, cover;
  background-repeat: no-repeat;
}
</style>
