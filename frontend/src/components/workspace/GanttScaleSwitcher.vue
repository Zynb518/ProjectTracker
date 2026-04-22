<script setup lang="ts">
import type { GanttScale } from '@/types/gantt'

defineProps<{
  scale: GanttScale
}>()

const emit = defineEmits<{
  'update:scale': [scale: GanttScale]
}>()

function updateScale(scale: GanttScale) {
  emit('update:scale', scale)
}
</script>

<template>
  <div class="gantt-scale-switcher">
    <div class="gantt-scale-switcher__track" role="tablist" aria-label="甘特图时间粒度">
      <button
        :class="['gantt-scale-switcher__option', { 'is-active': scale === 'day' }]"
        data-testid="gantt-scale-day"
        type="button"
        @click="updateScale('day')"
      >
        天
      </button>
      <button
        :class="['gantt-scale-switcher__option', { 'is-active': scale === 'week' }]"
        data-testid="gantt-scale-week"
        type="button"
        @click="updateScale('week')"
      >
        周
      </button>
      <button
        :class="['gantt-scale-switcher__option', { 'is-active': scale === 'month' }]"
        data-testid="gantt-scale-month"
        type="button"
        @click="updateScale('month')"
      >
        月
      </button>
    </div>
  </div>
</template>

<style scoped>
.gantt-scale-switcher {
  display: inline-flex;
  align-items: center;
}

.gantt-scale-switcher__track {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  padding: 6px;
  border: 1px solid color-mix(in srgb, var(--accent-line) 26%, var(--border-soft));
  border-radius: 999px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--glass-bg) 94%, transparent), color-mix(in srgb, var(--glass-bg) 78%, transparent)),
    radial-gradient(circle at top, color-mix(in srgb, var(--accent-end) 12%, transparent), transparent 62%);
  box-shadow:
    inset 0 1px 0 color-mix(in srgb, #ffffff 28%, transparent),
    0 12px 28px color-mix(in srgb, var(--accent-end) 10%, transparent);
}

.gantt-scale-switcher__option {
  border: 0;
  color: var(--text-soft);
  cursor: pointer;
  transition:
    transform 180ms ease-out,
    color 180ms ease-out,
    background 180ms ease-out,
    box-shadow 180ms ease-out;
}

.gantt-scale-switcher__option {
  min-width: 48px;
  padding: 10px 14px;
  border-radius: 999px;
  background: transparent;
  font-size: 0.82rem;
  font-weight: 600;
}

.gantt-scale-switcher__option:hover {
  transform: translateY(-1px);
  color: var(--text-main);
}

.gantt-scale-switcher__option.is-active {
  color: #f8fbff;
  background: linear-gradient(135deg, var(--accent-start), var(--accent-end));
  box-shadow:
    0 14px 30px color-mix(in srgb, var(--accent-end) 26%, transparent),
    inset 0 1px 0 color-mix(in srgb, #ffffff 32%, transparent);
}
</style>
