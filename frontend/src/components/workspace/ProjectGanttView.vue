<script setup lang="ts">
import { computed, ref } from 'vue'

import GanttScaleSwitcher from '@/components/workspace/GanttScaleSwitcher.vue'
import type { GanttNodeSummary, GanttScale, ProjectStageGantt } from '@/types/gantt'
import { getWorkStatusLabel, getWorkStatusTone } from '@/utils/display'
import { buildGanttAxisItems, getGanttBarLayout } from '@/utils/gantt'

const props = withDefaults(defineProps<{
  error?: string | null
  gantt: ProjectStageGantt | null
  isLoading?: boolean
  scale: GanttScale
}>(), {
  error: null,
  isLoading: false,
})

const emit = defineEmits<{
  'open-node': [nodeId: number]
  retry: []
  'update:scale': [scale: GanttScale]
}>()

const hoveredNodeId = ref<number | null>(null)

const axisItems = computed(() =>
  props.gantt === null
    ? []
    : buildGanttAxisItems(
        props.gantt.project.planned_start_date,
        props.gantt.project.planned_end_date,
        props.scale,
      ),
)

const axisStartDate = computed(
  () => axisItems.value[0]?.startDate ?? props.gantt?.project.planned_start_date ?? '',
)

const canvasWidth = computed(() =>
  axisItems.value.reduce((total, item) => total + item.widthPx, 0),
)

const hoveredNode = computed(() =>
  props.gantt?.nodes.find((node) => node.id === hoveredNodeId.value) ?? null,
)

function getBarStyle(node: GanttNodeSummary) {
  const layout = getGanttBarLayout(
    axisStartDate.value,
    node.planned_start_date,
    node.planned_end_date,
    props.scale,
  )

  return {
    left: `${layout.leftPx}px`,
    width: `${layout.widthPx}px`,
  }
}

function showNodeDetail(nodeId: number) {
  hoveredNodeId.value = nodeId
}

function clearNodeDetail(nodeId: number) {
  if (hoveredNodeId.value === nodeId) {
    hoveredNodeId.value = null
  }
}
</script>

<template>
  <section class="project-gantt" data-testid="project-gantt-view">
    <header class="project-gantt__toolbar">
      <div class="project-gantt__toolbar-copy">
        <p class="project-gantt__eyebrow">Timeline Matrix</p>
        <div>
          <h3>阶段甘特图</h3>
          <p>顶部按时间粒度缩放，左侧按阶段查看，点击某个阶段即可下钻到子任务时间视图。</p>
        </div>
      </div>

      <GanttScaleSwitcher :scale="scale" @update:scale="emit('update:scale', $event)" />
    </header>

    <p v-if="isLoading" class="project-gantt__state loading-panel">阶段甘特图加载中...</p>

    <section v-else-if="error" class="project-gantt__state project-gantt__state--error">
      <div>
        <strong>阶段甘特图暂时不可用</strong>
        <p>{{ error }}</p>
      </div>

      <button type="button" @click="emit('retry')">重试</button>
    </section>

    <section v-else-if="gantt === null || gantt.nodes.length === 0" class="project-gantt__state">
      当前还没有可展示的阶段排期。
    </section>

    <div v-else class="project-gantt__body">
      <aside class="project-gantt__sidebar">
        <div class="project-gantt__sidebar-head">
          <span>阶段</span>
          <span>状态</span>
        </div>

        <div
          v-for="node in gantt.nodes"
          :key="node.id"
          :data-testid="`project-gantt-row-label-${node.id}`"
          class="project-gantt__sidebar-row"
        >
          <strong>{{ node.name }}</strong>
          <span
            :class="[
              'project-gantt__status-pill',
              `project-gantt__status-pill--${getWorkStatusTone(node.status)}`,
            ]"
          >
            {{ getWorkStatusLabel(node.status) }}
          </span>
        </div>
      </aside>

      <div class="project-gantt__timeline-shell">
        <div class="project-gantt__timeline-scroll">
          <div class="project-gantt__timeline-canvas" :style="{ width: `${canvasWidth}px` }">
            <div class="project-gantt__axis" data-testid="project-gantt-axis">
              <div
                v-for="item in axisItems"
                :key="item.key"
                class="project-gantt__axis-cell"
                :style="{ width: `${item.widthPx}px` }"
              >
                {{ item.label }}
              </div>
            </div>

            <div class="project-gantt__rows">
              <div v-for="node in gantt.nodes" :key="node.id" class="project-gantt__row">
                <div class="project-gantt__track">
                  <button
                    :data-testid="`project-gantt-stage-bar-${node.id}`"
                    :class="[
                      'project-gantt__bar',
                      `project-gantt__bar--${getWorkStatusTone(node.status)}`,
                    ]"
                    :style="getBarStyle(node)"
                    :title="`${node.name}｜${getWorkStatusLabel(node.status)}｜${node.planned_start_date} - ${node.planned_end_date}`"
                    type="button"
                    @blur="clearNodeDetail(node.id)"
                    @click="emit('open-node', node.id)"
                    @focus="showNodeDetail(node.id)"
                    @mouseleave="clearNodeDetail(node.id)"
                    @mouseenter="showNodeDetail(node.id)"
                  >
                    <span>{{ node.name }}</span>
                  </button>
                </div>
              </div>
            </div>
          </div>
        </div>

        <aside v-if="hoveredNode" class="project-gantt__detail-card">
          <span class="project-gantt__detail-eyebrow">阶段详情</span>
          <strong>{{ hoveredNode.name }}</strong>
          <p>{{ getWorkStatusLabel(hoveredNode.status) }}</p>
          <dl>
            <div>
              <dt>顺序</dt>
              <dd>#{{ hoveredNode.sequence_no }}</dd>
            </div>
            <div>
              <dt>周期</dt>
              <dd>{{ hoveredNode.planned_start_date }}<br>{{ hoveredNode.planned_end_date }}</dd>
            </div>
            <div v-if="hoveredNode.completed_at">
              <dt>完成</dt>
              <dd>{{ hoveredNode.completed_at.slice(0, 16).replace('T', ' ') }}</dd>
            </div>
          </dl>
        </aside>
      </div>
    </div>
  </section>
</template>

<style scoped>
.project-gantt {
  position: relative;
  overflow: hidden;
  display: grid;
  gap: 18px;
  padding: 24px;
  border: 1px solid color-mix(in srgb, var(--accent-line) 34%, var(--border-soft));
  border-radius: 28px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--glass-bg) 96%, transparent), color-mix(in srgb, var(--glass-bg) 88%, transparent)),
    radial-gradient(circle at 100% 0%, color-mix(in srgb, var(--accent-end) 16%, transparent), transparent 32%);
  box-shadow: var(--shadow-panel-hover);
  backdrop-filter: var(--backdrop-blur);
}

.project-gantt::before {
  content: '';
  position: absolute;
  inset: 0;
  background:
    linear-gradient(var(--grid-line) 1px, transparent 1px),
    linear-gradient(90deg, var(--grid-line) 1px, transparent 1px);
  background-size: 24px 24px;
  opacity: 0.18;
  pointer-events: none;
}

.project-gantt__toolbar,
.project-gantt__body,
.project-gantt__state {
  position: relative;
  z-index: 1;
}

.project-gantt__toolbar {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 18px;
}

.project-gantt__toolbar-copy {
  display: grid;
  gap: 10px;
}

.project-gantt__eyebrow,
.project-gantt__toolbar-copy h3,
.project-gantt__toolbar-copy p {
  margin: 0;
}

.project-gantt__eyebrow {
  font-size: 0.76rem;
  letter-spacing: 0.18em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.project-gantt__toolbar-copy h3 {
  font-size: 1.46rem;
  color: color-mix(in srgb, var(--text-main) 86%, #ffffff 14%);
}

.project-gantt__toolbar-copy p {
  color: var(--text-soft);
  line-height: 1.6;
}

.project-gantt__state {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 18px;
  min-height: 240px;
  padding: 24px;
  border: 1px solid var(--meta-surface-border);
  border-radius: 22px;
  background: var(--meta-surface-bg), var(--meta-surface-glow);
  box-shadow: var(--meta-surface-shadow);
  color: var(--text-soft);
}

.project-gantt__state--error strong,
.project-gantt__state--error p {
  margin: 0;
}

.project-gantt__state--error {
  align-items: flex-start;
}

.project-gantt__state button {
  padding: 10px 16px;
  border: 0;
  border-radius: 12px;
  background: linear-gradient(135deg, var(--accent-start), var(--accent-end));
  color: #f8fbff;
  font-weight: 600;
  cursor: pointer;
}

.project-gantt__body {
  display: grid;
  grid-template-columns: minmax(220px, 260px) minmax(0, 1fr);
  gap: 18px;
  align-items: start;
}

.project-gantt__sidebar,
.project-gantt__timeline-shell {
  min-width: 0;
}

.project-gantt__sidebar {
  display: grid;
  gap: 10px;
}

.project-gantt__sidebar-head,
.project-gantt__sidebar-row {
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  gap: 12px;
  align-items: center;
  min-height: 62px;
  padding: 14px 16px;
  border: 1px solid var(--meta-surface-border);
  border-radius: 18px;
  background: var(--meta-surface-bg), var(--meta-surface-glow);
  box-shadow: var(--meta-surface-shadow);
}

.project-gantt__sidebar-head {
  min-height: 56px;
  font-size: 0.78rem;
  letter-spacing: 0.12em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.project-gantt__sidebar-row strong {
  min-width: 0;
  overflow: hidden;
  font-size: 1rem;
  text-overflow: ellipsis;
  white-space: nowrap;
  color: var(--text-main);
}

.project-gantt__status-pill {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 74px;
  padding: 8px 12px;
  border-radius: 999px;
  font-size: 0.78rem;
  font-weight: 600;
}

.project-gantt__status-pill--pending {
  background: color-mix(in srgb, var(--text-muted) 18%, transparent);
  color: var(--text-soft);
}

.project-gantt__status-pill--active {
  background: linear-gradient(135deg, color-mix(in srgb, var(--accent-start) 24%, transparent), color-mix(in srgb, var(--accent-end) 26%, transparent));
  color: color-mix(in srgb, var(--accent-end) 84%, #ffffff 16%);
}

.project-gantt__status-pill--done {
  background: color-mix(in srgb, var(--text-muted) 20%, transparent);
  color: color-mix(in srgb, var(--text-soft) 94%, transparent);
}

.project-gantt__status-pill--delayed {
  background: color-mix(in srgb, var(--accent-warning) 18%, transparent);
  color: var(--accent-warning);
}

.project-gantt__timeline-shell {
  position: relative;
  display: grid;
  gap: 14px;
}

.project-gantt__timeline-scroll {
  overflow: auto;
  padding-bottom: 6px;
}

.project-gantt__timeline-canvas {
  min-width: 100%;
}

.project-gantt__axis {
  display: flex;
  min-height: 56px;
  margin-bottom: 10px;
}

.project-gantt__axis-cell {
  flex: 0 0 auto;
  display: flex;
  align-items: center;
  padding: 0 12px;
  border-top: 1px solid color-mix(in srgb, var(--accent-line) 20%, transparent);
  border-bottom: 1px solid color-mix(in srgb, var(--accent-line) 20%, transparent);
  border-right: 1px solid color-mix(in srgb, var(--accent-line) 16%, transparent);
  font-size: 0.8rem;
  color: var(--text-soft);
}

.project-gantt__axis-cell:first-child {
  border-left: 1px solid color-mix(in srgb, var(--accent-line) 20%, transparent);
  border-top-left-radius: 16px;
  border-bottom-left-radius: 16px;
}

.project-gantt__axis-cell:last-child {
  border-top-right-radius: 16px;
  border-bottom-right-radius: 16px;
}

.project-gantt__rows {
  display: grid;
  gap: 10px;
}

.project-gantt__row {
  min-height: 62px;
}

.project-gantt__track {
  position: relative;
  min-height: 62px;
  border: 1px solid color-mix(in srgb, var(--accent-line) 18%, transparent);
  border-radius: 18px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--glass-bg) 74%, transparent), transparent),
    repeating-linear-gradient(90deg, color-mix(in srgb, var(--grid-line) 80%, transparent) 0, color-mix(in srgb, var(--grid-line) 80%, transparent) 1px, transparent 1px, transparent 36px);
}

.project-gantt__bar {
  position: absolute;
  top: 50%;
  display: inline-flex;
  align-items: center;
  height: 38px;
  padding: 0 14px;
  border: 0;
  border-radius: 14px;
  cursor: pointer;
  transform: translateY(-50%);
  transition:
    transform 180ms ease-out,
    box-shadow 180ms ease-out,
    filter 180ms ease-out;
}

.project-gantt__bar span {
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  font-size: 0.84rem;
  font-weight: 600;
}

.project-gantt__bar:hover,
.project-gantt__bar:focus-visible {
  transform: translateY(calc(-50% - 1px));
  box-shadow: 0 16px 28px color-mix(in srgb, var(--text-main) 18%, transparent);
  filter: saturate(1.04);
  outline: none;
}

.project-gantt__bar--pending {
  background: linear-gradient(135deg, color-mix(in srgb, var(--text-muted) 36%, transparent), color-mix(in srgb, var(--text-soft) 22%, transparent));
  color: var(--text-main);
}

.project-gantt__bar--active {
  background: linear-gradient(135deg, var(--accent-start), var(--accent-end));
  color: #f6fbff;
}

.project-gantt__bar--done {
  background: linear-gradient(135deg, color-mix(in srgb, var(--text-muted) 34%, transparent), color-mix(in srgb, var(--text-soft) 18%, transparent));
  color: color-mix(in srgb, var(--text-main) 70%, transparent);
}

.project-gantt__bar--delayed {
  background: linear-gradient(135deg, color-mix(in srgb, var(--accent-warning) 76%, #ffffff 10%), color-mix(in srgb, var(--accent-warning) 58%, transparent));
  color: #fffaf2;
}

.project-gantt__detail-card {
  align-self: start;
  display: grid;
  gap: 10px;
  padding: 18px;
  border: 1px solid color-mix(in srgb, var(--accent-line) 24%, var(--border-soft));
  border-radius: 20px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--glass-bg) 96%, transparent), color-mix(in srgb, var(--glass-bg) 86%, transparent)),
    radial-gradient(circle at top right, color-mix(in srgb, var(--accent-end) 14%, transparent), transparent 38%);
  box-shadow: var(--shadow-panel);
}

.project-gantt__detail-eyebrow {
  font-size: 0.72rem;
  letter-spacing: 0.18em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.project-gantt__detail-card strong,
.project-gantt__detail-card p,
.project-gantt__detail-card dt,
.project-gantt__detail-card dd {
  margin: 0;
}

.project-gantt__detail-card strong {
  font-size: 1.04rem;
  color: var(--text-main);
}

.project-gantt__detail-card p,
.project-gantt__detail-card dt {
  color: var(--text-soft);
}

.project-gantt__detail-card dl {
  display: grid;
  gap: 8px;
}

.project-gantt__detail-card dl div {
  display: grid;
  gap: 4px;
}

.project-gantt__detail-card dd {
  color: var(--text-main);
}

@media (max-width: 1100px) {
  .project-gantt__body {
    grid-template-columns: 1fr;
  }

  .project-gantt__toolbar {
    flex-direction: column;
    align-items: stretch;
  }
}
</style>
