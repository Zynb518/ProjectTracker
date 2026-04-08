<script setup lang="ts">
import ProjectActionsMenu from '@/components/projects/ProjectActionsMenu.vue'
import type { ProjectListItem } from '@/types/project'

defineProps<{
  projects: ProjectListItem[]
}>()

defineEmits<{
  open: [project: ProjectListItem]
  edit: [project: ProjectListItem]
  start: [project: ProjectListItem]
  complete: [project: ProjectListItem]
  reopen: [project: ProjectListItem]
  remove: [project: ProjectListItem]
}>()

function statusLabel(status: number) {
  return (
    {
      1: '未开始',
      2: '进行中',
      3: '已完成',
      4: '已延期',
    }[status] ?? '未知状态'
  )
}

function statusTone(status: number) {
  return (
    {
      1: 'pending',
      2: 'active',
      3: 'done',
      4: 'delayed',
    }[status] ?? 'unknown'
  )
}
</script>

<template>
  <section class="project-grid">
    <article
      v-for="project in projects"
      :key="project.id"
      class="project-card"
      tabindex="0"
      @click="$emit('open', project)"
      @keydown.enter.prevent="$emit('open', project)"
      @keydown.space.prevent="$emit('open', project)"
    >
      <div class="project-card__header">
        <div class="project-card__title">
          <h2 class="project-card__name" :title="project.name">{{ project.name }}</h2>
        </div>

        <span
          :class="['project-card__status', `project-card__status--${statusTone(project.status)}`]"
        >
          {{ statusLabel(project.status) }}
        </span>
      </div>

      <p class="project-card__description" :title="project.description || '当前项目未填写描述。'">
        {{ project.description || '当前项目未填写描述。' }}
      </p>

      <div class="project-card__body">
        <dl class="project-card__meta">
          <div>
            <dt>负责人</dt>
            <dd>
              <span class="project-card__meta-value" :title="project.owner_real_name">
                {{ project.owner_real_name }}
              </span>
            </dd>
          </div>
          <div>
            <dt>计划周期</dt>
            <dd>
              <span
                class="project-card__schedule"
                :title="`${project.planned_start_date} - ${project.planned_end_date}`"
              >
                <span class="project-card__schedule-line">{{ project.planned_start_date }}</span>
                <span class="project-card__schedule-line">{{ project.planned_end_date }}</span>
              </span>
            </dd>
          </div>
          <div>
            <dt>成员数</dt>
            <dd>
              <span class="project-card__meta-value" :title="String(project.member_count)">
                {{ project.member_count }}
              </span>
            </dd>
          </div>
          <div>
            <dt>节点 / 子任务</dt>
            <dd>
              <span
                class="project-card__meta-value"
                :title="`${project.node_count} / ${project.sub_task_count}`"
              >
                {{ project.node_count }} / {{ project.sub_task_count }}
              </span>
            </dd>
          </div>
        </dl>
      </div>

      <div class="project-card__footer">
        <ProjectActionsMenu
          :project="project"
          @complete="$emit('complete', project)"
          @edit="$emit('edit', project)"
          @remove="$emit('remove', project)"
          @reopen="$emit('reopen', project)"
          @start="$emit('start', project)"
        />
      </div>
    </article>
  </section>
</template>

<style scoped>
.project-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(min(320px, 100%), 1fr));
  grid-auto-rows: 492px;
  gap: 18px;
}

.project-card {
  position: relative;
  min-width: 0;
  display: grid;
  grid-template-rows: auto 84px minmax(0, 1fr) 112px;
  gap: 14px;
  height: 492px;
  min-height: 492px;
  overflow: hidden;
  padding: 24px;
  border: 1px solid var(--border-soft);
  border-radius: 16px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--glass-bg) 92%, transparent), var(--glass-bg)),
    radial-gradient(circle at top right, rgba(0, 194, 255, 0.1), transparent 24%);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
  cursor: pointer;
  transition:
    transform 200ms ease-out,
    box-shadow 200ms ease-out,
    border-color 200ms ease-out,
    background-color 200ms ease-out;
}

.project-card::after {
  content: '';
  position: absolute;
  inset: 0;
  border-radius: inherit;
  background:
    linear-gradient(135deg, rgba(255, 255, 255, 0.22), transparent 40%),
    radial-gradient(circle at 85% 12%, rgba(0, 194, 255, 0.16), transparent 28%);
  opacity: 0;
  pointer-events: none;
  transition:
    opacity 200ms ease-out,
    transform 200ms ease-out;
  transform: translateY(8px);
}

.project-card:hover,
.project-card:focus-visible {
  transform: translateY(-2px);
  border-color: var(--accent-line);
  box-shadow: var(--shadow-panel-hover);
}

.project-card:hover::after,
.project-card:focus-visible::after {
  opacity: 1;
  transform: translateY(0);
}

.project-card:focus-visible {
  outline: 2px solid var(--accent-line);
  outline-offset: 3px;
}

.project-card__header {
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto;
  align-items: start;
  gap: 14px;
}

.project-card__header h2,
.project-card__header p {
  margin: 0;
}

.project-card__title {
  min-width: 0;
}

.project-card__name {
  font-size: 1.35rem;
  line-height: 1.18;
  overflow: hidden;
  overflow-wrap: anywhere;
  word-break: break-word;
  display: -webkit-box;
  -webkit-line-clamp: 2;
  -webkit-box-orient: vertical;
}

.project-card__status {
  display: inline-flex;
  align-self: start;
  justify-self: end;
  align-items: center;
  gap: 8px;
  padding: 8px 14px 8px 12px;
  border: 1px solid transparent;
  border-radius: 20px;
  font-size: 0.78rem;
  font-weight: 600;
  letter-spacing: 0.06em;
  line-height: 1;
  white-space: nowrap;
  text-transform: uppercase;
}

.project-card__status::before {
  content: '';
  width: 8px;
  height: 8px;
  border-radius: 999px;
  background: currentColor;
  flex: none;
  opacity: 0.88;
}

.project-card__status--pending {
  border-color: color-mix(in srgb, var(--accent-neutral) 30%, transparent);
  background: color-mix(in srgb, var(--accent-neutral) 18%, transparent);
  color: var(--text-soft);
}

.project-card__status--active {
  border-color: rgba(0, 194, 255, 0.24);
  background: var(--gradient-primary);
  color: var(--text-inverse);
}

.project-card__status--done {
  border-color: color-mix(in srgb, var(--accent-success) 28%, transparent);
  background: color-mix(in srgb, var(--accent-success) 18%, transparent);
  color: var(--accent-success);
}

.project-card__status--delayed {
  border-color: color-mix(in srgb, var(--accent-warning) 28%, transparent);
  background: color-mix(in srgb, var(--accent-warning) 18%, transparent);
  color: var(--accent-warning);
}

.project-card__status--unknown {
  border-color: var(--border-soft);
  background: color-mix(in srgb, var(--panel-bg) 85%, transparent);
  color: var(--text-soft);
}

.project-card__description {
  margin: 0;
  line-height: 1.56;
  color: var(--text-soft);
  min-height: calc(1.56em * 3);
  max-height: calc(1.56em * 3);
  overflow: hidden;
  overflow-wrap: anywhere;
  word-break: break-word;
  display: -webkit-box;
  -webkit-line-clamp: 3;
  -webkit-box-orient: vertical;
}

.project-card__body {
  min-height: 0;
  overflow: hidden;
}

.project-card__meta {
  height: 100%;
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  grid-auto-rows: minmax(0, 1fr);
  align-content: stretch;
  gap: 10px;
  margin: 0;
}

.project-card__meta div {
  min-width: 0;
  min-height: 72px;
  display: flex;
  flex-direction: column;
  justify-content: center;
  padding: 12px 12px;
  border: 1px solid var(--border-soft);
  border-radius: 14px;
  background: color-mix(in srgb, var(--panel-bg) 88%, transparent);
}

.project-card__meta dt {
  font-size: 0.72rem;
  letter-spacing: 0.08em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.project-card__meta dd {
  margin: 6px 0 0;
  min-width: 0;
  font-weight: 600;
}

.project-card__meta-value {
  display: block;
  overflow: hidden;
  overflow-wrap: anywhere;
  word-break: break-word;
  display: -webkit-box;
  -webkit-line-clamp: 2;
  -webkit-box-orient: vertical;
}

.project-card__schedule {
  display: grid;
  gap: 2px;
}

.project-card__schedule-line {
  display: block;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.project-card__footer {
  min-height: 0;
  display: flex;
  align-items: flex-end;
}

.project-card__footer :deep(.project-actions) {
  width: 100%;
  margin-top: 0;
}

@media (max-width: 720px) {
  .project-card__header {
    grid-template-columns: 1fr;
  }

  .project-card__status {
    justify-self: start;
  }

  .project-card__meta {
    grid-template-columns: 1fr;
  }
}
</style>
