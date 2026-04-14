<script setup lang="ts">
import { computed } from 'vue'

import { useThemeStore } from '@/stores/theme'

const themeStore = useThemeStore()
const isDark = computed(() => themeStore.mode === 'dark')

const meteors = [
  {
    id: 1,
    top: '10%',
    left: '-14vw',
    delay: '0.8s',
    duration: '13.5s',
    travelX: '78vw',
    travelY: '34vh',
    length: 'clamp(150px, 18vw, 260px)',
  },
  {
    id: 2,
    top: '22%',
    left: '8vw',
    delay: '5.2s',
    duration: '15.8s',
    travelX: '64vw',
    travelY: '28vh',
    length: 'clamp(120px, 14vw, 220px)',
  },
  {
    id: 3,
    top: '32%',
    left: '56vw',
    delay: '9.4s',
    duration: '12.4s',
    travelX: '30vw',
    travelY: '18vh',
    length: 'clamp(110px, 12vw, 190px)',
  },
  {
    id: 4,
    top: '16%',
    left: '36vw',
    delay: '12.8s',
    duration: '16.2s',
    travelX: '46vw',
    travelY: '26vh',
    length: 'clamp(130px, 15vw, 210px)',
  },
] as const
</script>

<template>
  <div v-if="isDark" class="meteor-sky-overlay" data-testid="meteor-sky-overlay" aria-hidden="true">
    <span
      v-for="meteor in meteors"
      :key="meteor.id"
      class="meteor-sky-overlay__meteor"
      :style="{
        '--meteor-top': meteor.top,
        '--meteor-left': meteor.left,
        '--meteor-delay': meteor.delay,
        '--meteor-duration': meteor.duration,
        '--meteor-travel-x': meteor.travelX,
        '--meteor-travel-y': meteor.travelY,
        '--meteor-length': meteor.length,
      }"
    />
  </div>
</template>

<style scoped>
.meteor-sky-overlay {
  position: fixed;
  inset: 0;
  z-index: 1;
  overflow: hidden;
  pointer-events: none;
}

.meteor-sky-overlay__meteor {
  position: absolute;
  top: var(--meteor-top);
  left: var(--meteor-left);
  width: var(--meteor-length);
  height: 2px;
  border-radius: 999px;
  opacity: 0;
  transform: translate3d(0, 0, 0) rotate(24deg) scaleX(0.72);
  transform-origin: right center;
  will-change: transform, opacity;
  background: linear-gradient(
    90deg,
    rgba(132, 183, 255, 0.04) 0%,
    rgba(132, 183, 255, 0.46) 64%,
    rgba(175, 228, 255, 0.96) 92%,
    rgba(246, 251, 255, 0.98) 100%
  );
  box-shadow:
    0 0 14px rgba(124, 211, 255, 0.22),
    0 0 28px rgba(141, 120, 255, 0.08);
  animation: meteor-flight var(--meteor-duration) linear infinite;
  animation-delay: var(--meteor-delay);
}

.meteor-sky-overlay__meteor::after {
  content: '';
  position: absolute;
  right: -1px;
  top: 50%;
  width: 8px;
  height: 8px;
  border-radius: 999px;
  transform: translateY(-50%);
  background: radial-gradient(circle, rgba(246, 251, 255, 0.98), rgba(124, 211, 255, 0.3) 72%, transparent 100%);
  box-shadow:
    0 0 12px rgba(124, 211, 255, 0.46),
    0 0 20px rgba(141, 120, 255, 0.22);
}

@keyframes meteor-flight {
  0% {
    opacity: 0;
    transform: translate3d(0, 0, 0) rotate(24deg) scaleX(0.72);
  }

  6% {
    opacity: 0.92;
  }

  14% {
    opacity: 1;
    transform: translate3d(
        calc(var(--meteor-travel-x) * 0.32),
        calc(var(--meteor-travel-y) * 0.32),
        0
      )
      rotate(24deg) scaleX(1);
  }

  24% {
    opacity: 0.36;
    transform: translate3d(
        calc(var(--meteor-travel-x) * 0.78),
        calc(var(--meteor-travel-y) * 0.78),
        0
      )
      rotate(24deg) scaleX(1.04);
  }

  30%,
  100% {
    opacity: 0;
    transform: translate3d(var(--meteor-travel-x), var(--meteor-travel-y), 0) rotate(24deg) scaleX(1.06);
  }
}

@media (prefers-reduced-motion: reduce) {
  .meteor-sky-overlay {
    display: none;
  }

  .meteor-sky-overlay__meteor {
    animation: none;
  }
}
</style>
