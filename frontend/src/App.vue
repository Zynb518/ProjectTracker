<script setup lang="ts">
import { onMounted } from 'vue'

import MeteorSkyOverlay from '@/components/backgrounds/MeteorSkyOverlay.vue'
import SkyBackdrop from '@/components/backgrounds/SkyBackdrop.vue'
import GlobalNoticeLayer from '@/components/GlobalNoticeLayer.vue'
import { useThemeStore } from '@/stores/theme'

const themeStore = useThemeStore()

onMounted(() => {
  themeStore.initializeTheme()
})
</script>

<template>
  <div class="app-root">
    <SkyBackdrop />
    <MeteorSkyOverlay />
    <div class="app-root__content">
      <GlobalNoticeLayer />
      <RouterView v-slot="{ Component, route }">
        <Transition mode="out-in" name="route-fade">
          <component :is="Component" :key="route.fullPath" />
        </Transition>
      </RouterView>
    </div>
  </div>
</template>

<style scoped>
.app-root {
  min-height: 100vh;
  position: relative;
  isolation: isolate;
}

.app-root__content {
  position: relative;
  z-index: 2;
  min-height: 100vh;
}

.route-fade-enter-active,
.route-fade-leave-active {
  transition:
    opacity 300ms ease-out,
    transform 300ms ease-out,
    filter 300ms ease-out;
}

.route-fade-enter-from,
.route-fade-leave-to {
  opacity: 0.3;
  transform: translateY(10px);
  filter: blur(6px);
}
</style>
