<script setup lang="ts">
import { computed } from 'vue'

import type { ProjectDetail } from '@/types/project'
import {
  calculateProgressPercent,
  getWorkStatusLabel,
  getWorkStatusTone,
} from '@/utils/display'

const props = defineProps<{
  project: ProjectDetail
}>()

const nodeProgress = computed(() =>
  calculateProgressPercent(props.project.completed_node_count, props.project.node_count),
)
const subtaskProgress = computed(() =>
  calculateProgressPercent(props.project.completed_sub_task_count, props.project.sub_task_count),
)
</script>

<template>
  <section class="project-overview-card" data-testid="project-overview-card">
    <div class="project-overview-card__content">
      <div class="project-overview-card__lead">
        <p class="project-overview-card__eyebrow">Project Overview</p>
        <h2>{{ project.name }}</h2>
        <p class="project-overview-card__description">
          {{ project.description || '当前项目未填写描述。' }}
        </p>
      </div>

      <div class="project-overview-card__metrics">
        <article class="project-overview-card__metric project-overview-card__metric--summary" data-testid="member-summary">
          <span class="project-overview-card__metric-label">成员</span>
          <strong class="project-overview-card__metric-value">{{ project.member_count }}</strong>
          <p class="project-overview-card__metric-caption">当前参与协作</p>
        </article>

        <article class="project-overview-card__metric project-overview-card__metric--progress">
          <div
            class="project-overview-card__ring project-overview-card__ring--nodes"
            data-testid="node-progress-ring"
            :style="{ '--progress': `${nodeProgress}%` }"
          >
            <div class="project-overview-card__ring-core">
              <strong>{{ project.completed_node_count }} / {{ project.node_count }}</strong>
              <span>完成</span>
            </div>
          </div>
          <div class="project-overview-card__progress-copy">
            <span class="project-overview-card__metric-label">阶段节点</span>
            <strong>{{ nodeProgress }}%</strong>
          </div>
        </article>

        <article class="project-overview-card__metric project-overview-card__metric--progress">
          <div
            class="project-overview-card__ring project-overview-card__ring--subtasks"
            data-testid="subtask-progress-ring"
            :style="{ '--progress': `${subtaskProgress}%` }"
          >
            <div class="project-overview-card__ring-core">
              <strong>{{ project.completed_sub_task_count }} / {{ project.sub_task_count }}</strong>
              <span>完成</span>
            </div>
          </div>
          <div class="project-overview-card__progress-copy">
            <span class="project-overview-card__metric-label">子任务</span>
            <strong>{{ subtaskProgress }}%</strong>
          </div>
        </article>
      </div>
    </div>

    <aside class="project-overview-card__meta" data-testid="project-meta-panel">
      <div class="project-overview-card__meta-row">
        <span class="project-overview-card__meta-label">状态</span>
        <strong class="project-overview-card__meta-value">
          <span
            :class="[
              'project-overview-card__status',
              `project-overview-card__status--${getWorkStatusTone(project.status)}`,
            ]"
          >
            {{ getWorkStatusLabel(project.status) }}
          </span>
        </strong>
      </div>

      <div class="project-overview-card__meta-row">
        <span class="project-overview-card__meta-label">负责人</span>
        <strong class="project-overview-card__meta-value">{{ project.owner_real_name }}</strong>
      </div>

      <div class="project-overview-card__meta-row">
        <span class="project-overview-card__meta-label">周期</span>
        <strong class="project-overview-card__meta-value project-overview-card__meta-value--dates">
          <span>{{ project.planned_start_date }}</span>
          <span>{{ project.planned_end_date }}</span>
        </strong>
      </div>
    </aside>
  </section>
</template>

<style scoped>
.project-overview-card {
  display: grid;
  grid-template-columns: minmax(0, 1.2fr) minmax(250px, 290px);
  gap: 20px;
  padding: 28px;
  border: 1px solid color-mix(in srgb, var(--accent-line) 34%, var(--border-soft));
  border-radius: 28px;
  background: var(--project-card-bg), var(--project-card-glow);
  box-shadow: var(--shadow-panel-hover);
  backdrop-filter: var(--backdrop-blur);
}

.project-overview-card__content {
  min-width: 0;
  display: grid;
  gap: 24px;
}

.project-overview-card__lead h2,
.project-overview-card__lead p {
  margin: 0;
}

.project-overview-card__eyebrow {
  margin-bottom: 10px;
  font-size: 0.82rem;
  letter-spacing: 0.18em;
  text-transform: uppercase;
  color: color-mix(in srgb, var(--text-main) 68%, var(--text-soft));
}

.project-overview-card__lead h2 {
  font-size: clamp(2rem, 3.2vw, 2.8rem);
  line-height: 1.06;
  color: var(--text-main);
}

.project-overview-card__description {
  margin-top: 14px;
  max-width: 60ch;
  font-size: 1rem;
  line-height: 1.68;
  color: color-mix(in srgb, var(--text-main) 78%, var(--text-soft));
}

.project-overview-card__metrics {
  display: grid;
  grid-template-columns: minmax(150px, 180px) repeat(2, minmax(0, 1fr));
  gap: 14px;
}

.project-overview-card__metric {
  min-width: 0;
  min-height: 176px;
  padding: 18px;
  border: 1px solid var(--meta-surface-border);
  border-radius: 24px;
  background: var(--meta-surface-bg), var(--meta-surface-glow);
  box-shadow: var(--meta-surface-shadow);
}

.project-overview-card__metric--summary {
  display: flex;
  flex-direction: column;
  justify-content: space-between;
}

.project-overview-card__metric-label {
  font-size: 0.82rem;
  letter-spacing: 0.12em;
  text-transform: uppercase;
  color: color-mix(in srgb, var(--text-main) 68%, var(--text-soft));
}

.project-overview-card__metric-value {
  font-size: clamp(2.4rem, 4vw, 3rem);
  line-height: 1;
  color: var(--text-main);
}

.project-overview-card__metric-caption {
  margin: 0;
  font-size: 1rem;
  color: color-mix(in srgb, var(--text-main) 72%, var(--text-soft));
}

.project-overview-card__metric--progress {
  display: grid;
  justify-items: center;
  align-content: center;
  gap: 14px;
}

.project-overview-card__ring {
  width: 126px;
  height: 126px;
  padding: 10px;
  border-radius: 50%;
  background:
    conic-gradient(var(--ring-color) var(--progress), color-mix(in srgb, var(--text-muted) 18%, transparent) 0);
  box-shadow:
    0 16px 32px color-mix(in srgb, var(--ring-color) 20%, transparent),
    inset 0 0 0 1px color-mix(in srgb, var(--ring-color) 18%, transparent);
}

.project-overview-card__ring--nodes {
  --ring-color: var(--accent-end);
}

.project-overview-card__ring--subtasks {
  --ring-color: var(--accent-success);
}

.project-overview-card__ring-core {
  width: 100%;
  height: 100%;
  border-radius: 50%;
  display: grid;
  place-items: center;
  gap: 4px;
  padding: 16px;
  text-align: center;
  background: color-mix(in srgb, var(--panel-bg) 94%, transparent);
  box-shadow: inset 0 0 0 1px var(--border-soft);
}

.project-overview-card__ring-core strong {
  font-size: 1rem;
  line-height: 1.1;
  color: var(--text-main);
}

.project-overview-card__ring-core span {
  font-size: 0.78rem;
  letter-spacing: 0.1em;
  text-transform: uppercase;
  color: color-mix(in srgb, var(--text-main) 68%, var(--text-soft));
}

.project-overview-card__progress-copy {
  display: grid;
  gap: 6px;
  justify-items: center;
}

.project-overview-card__progress-copy strong {
  font-size: 1.42rem;
  line-height: 1;
  color: var(--text-main);
}

.project-overview-card__meta {
  display: grid;
  align-content: start;
  gap: 0;
  padding: 8px 0;
  border: 1px solid color-mix(in srgb, var(--accent-line) 26%, transparent);
  border-radius: 24px;
  background: color-mix(in srgb, var(--glass-bg-strong) 92%, transparent);
  box-shadow: var(--meta-surface-shadow);
}

.project-overview-card__meta-row {
  display: grid;
  gap: 10px;
  padding: 18px 20px;
  border-bottom: 1px solid var(--border-soft);
}

.project-overview-card__meta-row:last-child {
  border-bottom: 0;
}

.project-overview-card__meta-label {
  font-size: 0.82rem;
  letter-spacing: 0.12em;
  text-transform: uppercase;
  color: color-mix(in srgb, var(--text-main) 68%, var(--text-soft));
}

.project-overview-card__meta-value {
  font-size: 1.18rem;
  font-weight: 700;
  line-height: 1.35;
  color: var(--text-main);
}

.project-overview-card__meta-value--dates {
  display: grid;
  gap: 4px;
}

.project-overview-card__status {
  display: inline-flex;
  align-items: center;
  gap: 8px;
  padding: 8px 12px;
  border: 1px solid transparent;
  border-radius: 999px;
  font-size: 0.86rem;
  font-weight: 700;
  letter-spacing: 0.04em;
  line-height: 1;
  width: fit-content;
}

.project-overview-card__status::before {
  content: '';
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: currentColor;
  opacity: 0.9;
}

.project-overview-card__status--pending {
  border-color: var(--work-status-pending-border);
  background: var(--work-status-pending-bg);
  color: var(--work-status-pending-color);
}

.project-overview-card__status--active {
  border-color: var(--work-status-active-border);
  background: var(--work-status-active-bg);
  color: var(--work-status-active-color);
}

.project-overview-card__status--done {
  border-color: var(--work-status-done-border);
  background: var(--work-status-done-bg);
  color: var(--work-status-done-color);
}

.project-overview-card__status--delayed {
  border-color: var(--work-status-delayed-border);
  background: var(--work-status-delayed-bg);
  color: var(--work-status-delayed-color);
}

.project-overview-card__status--unknown {
  border-color: var(--work-status-unknown-border);
  background: var(--work-status-unknown-bg);
  color: var(--work-status-unknown-color);
}

@media (max-width: 1100px) {
  .project-overview-card {
    grid-template-columns: 1fr;
  }

  .project-overview-card__meta {
    padding: 0;
  }
}

@media (max-width: 860px) {
  .project-overview-card {
    padding: 22px;
  }

  .project-overview-card__metrics {
    grid-template-columns: 1fr;
  }

  .project-overview-card__metric {
    min-height: 0;
  }
}
</style>
