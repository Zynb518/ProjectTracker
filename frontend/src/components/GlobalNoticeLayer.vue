<script setup lang="ts">
import { storeToRefs } from 'pinia'

import { useNotificationStore } from '@/stores/notifications'

const notificationStore = useNotificationStore()
const { notices } = storeToRefs(notificationStore)
</script>

<template>
  <Teleport to="body">
    <div v-if="notices.length > 0" class="notice-layer">
      <TransitionGroup name="notice">
        <section
          v-for="notice in notices"
          :key="notice.id"
          :class="['notice-card', `notice-card--${notice.kind}`]"
          role="alert"
        >
          <span class="notice-card__icon" aria-hidden="true">!</span>
          <div class="notice-card__body">
            <p class="notice-card__label">操作提醒</p>
            <p class="notice-card__message">{{ notice.message }}</p>
          </div>
        </section>
      </TransitionGroup>
    </div>
  </Teleport>
</template>

<style scoped>
.notice-layer {
  position: fixed;
  top: 28px;
  left: 50%;
  z-index: 2000;
  width: min(100vw - 32px, 520px);
  transform: translateX(-50%);
  pointer-events: none;
}

.notice-card {
  position: relative;
  overflow: hidden;
  display: grid;
  grid-template-columns: auto minmax(0, 1fr);
  gap: 12px;
  align-items: start;
  margin-bottom: 12px;
  padding: 14px 16px;
  border: 1px solid color-mix(in srgb, var(--accent-danger) 24%, var(--meta-surface-border));
  border-radius: 14px;
  background: var(--meta-surface-bg), var(--meta-surface-glow), var(--card-sheen);
  box-shadow: var(--meta-surface-shadow);
  color: var(--text-main);
}

.notice-card__icon {
  display: inline-grid;
  place-items: center;
  width: 24px;
  height: 24px;
  border-radius: 999px;
  background: linear-gradient(
    180deg,
    color-mix(in srgb, var(--accent-danger) 24%, transparent),
    color-mix(in srgb, var(--accent-tertiary) 18%, transparent)
  );
  color: var(--accent-danger);
  font-weight: 700;
  line-height: 1;
  box-shadow: 0 0 14px color-mix(in srgb, var(--accent-danger) 16%, transparent);
}

.notice-card__body {
  min-width: 0;
  display: grid;
  gap: 4px;
}

.notice-card__label,
.notice-card__message {
  margin: 0;
}

.notice-card__label {
  font-size: 0.72rem;
  font-weight: 700;
  letter-spacing: 0.12em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.notice-card__message {
  line-height: 1.5;
  word-break: break-word;
}

.notice-enter-active,
.notice-leave-active {
  transition:
    opacity 0.45s ease,
    transform 0.45s ease,
    filter 0.45s ease;
}

.notice-enter-from,
.notice-leave-to {
  opacity: 0;
  transform: translateY(-14px) scale(0.96);
  filter: blur(4px);
}

:global(html.dark) .notice-card {
  box-shadow:
    var(--meta-surface-shadow),
    0 18px 32px rgba(2, 6, 18, 0.32),
    0 0 22px color-mix(in srgb, var(--accent-danger) 10%, transparent);
}

:global(html.dark) .notice-card__message {
  color: color-mix(in srgb, var(--text-main) 94%, var(--accent-start) 6%);
}
</style>
