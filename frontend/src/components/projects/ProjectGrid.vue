<script setup lang="ts">
import ProjectActionsMenu from '@/components/projects/ProjectActionsMenu.vue'
import type { ProjectListItem } from '@/types/project'
import { getWorkStatusLabel, getWorkStatusTone } from '@/utils/display'

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
          :class="['project-card__status', `project-card__status--${getWorkStatusTone(project.status)}`]"
        >
          {{ getWorkStatusLabel(project.status) }}
        </span>
      </div>

      <p class="project-card__description" :title="project.description || '当前项目未填写描述。'">
        {{ project.description || '当前项目未填写描述。' }}
      </p>

      <div class="project-card__body">
        <dl class="project-card__meta">
          <div class="project-card__meta-item" data-tooltip="负责人">
            <dt aria-label="负责人">
              <span class="project-card__meta-icon" aria-hidden="true">
                <svg viewBox="0 0 24 24">
                  <path
                    d="M12 12a4 4 0 1 0 0-8 4 4 0 0 0 0 8Z"
                    fill="none"
                    stroke="currentColor"
                    stroke-width="1.7"
                  />
                  <path
                    d="M5 20a7 7 0 0 1 14 0"
                    fill="none"
                    stroke="currentColor"
                    stroke-linecap="round"
                    stroke-width="1.7"
                  />
                </svg>
              </span>
            </dt>
            <dd>
              <span class="project-card__meta-value" :title="project.owner_real_name">
                {{ project.owner_real_name }}
              </span>
            </dd>
          </div>
          <div class="project-card__meta-item" data-tooltip="计划周期">
            <dt aria-label="计划周期">
              <span class="project-card__meta-icon" aria-hidden="true">
                <svg viewBox="0 0 24 24">
                  <path
                    d="M7 4.5v3M17 4.5v3M4.5 9.5h15"
                    fill="none"
                    stroke="currentColor"
                    stroke-linecap="round"
                    stroke-width="1.7"
                  />
                  <rect
                    x="4.5"
                    y="6.5"
                    width="15"
                    height="13"
                    rx="3"
                    fill="none"
                    stroke="currentColor"
                    stroke-width="1.7"
                  />
                </svg>
              </span>
            </dt>
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
          <div class="project-card__meta-item" data-tooltip="成员数">
            <dt aria-label="成员数">
              <span class="project-card__meta-icon" aria-hidden="true">
                <svg viewBox="0 0 24 24">
                  <path
                    d="M9 10a3 3 0 1 0 0-6 3 3 0 0 0 0 6ZM16.5 11a2.5 2.5 0 1 0 0-5 2.5 2.5 0 0 0 0 5Z"
                    fill="none"
                    stroke="currentColor"
                    stroke-width="1.7"
                  />
                  <path
                    d="M3.5 19a5.5 5.5 0 0 1 11 0M14 18.5a4.5 4.5 0 0 1 6.5-3.9"
                    fill="none"
                    stroke="currentColor"
                    stroke-linecap="round"
                    stroke-width="1.7"
                  />
                </svg>
              </span>
            </dt>
            <dd>
              <span class="project-card__meta-value" :title="String(project.member_count)">
                {{ project.member_count }}
              </span>
            </dd>
          </div>
          <div class="project-card__meta-item" data-tooltip="节点 / 子任务">
            <dt aria-label="节点 / 子任务">
              <span class="project-card__meta-icon" aria-hidden="true">
                <svg viewBox="0 0 24 24">
                  <rect
                    x="4"
                    y="4.5"
                    width="6"
                    height="5"
                    rx="1.6"
                    fill="none"
                    stroke="currentColor"
                    stroke-width="1.7"
                  />
                  <rect
                    x="14"
                    y="4.5"
                    width="6"
                    height="5"
                    rx="1.6"
                    fill="none"
                    stroke="currentColor"
                    stroke-width="1.7"
                  />
                  <rect
                    x="9"
                    y="14.5"
                    width="6"
                    height="5"
                    rx="1.6"
                    fill="none"
                    stroke="currentColor"
                    stroke-width="1.7"
                  />
                  <path
                    d="M12 9.5v3M7 12.5h10"
                    fill="none"
                    stroke="currentColor"
                    stroke-linecap="round"
                    stroke-width="1.7"
                  />
                </svg>
              </span>
            </dt>
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
  grid-template-columns: repeat(auto-fill, 400px);
  justify-content: start;
  grid-auto-rows: 492px;
  gap: 18px;
}

.project-card {
  position: relative;
  min-width: 0;
  display: grid;
  grid-template-rows: auto 84px minmax(0, 1fr) 88px;
  gap: 14px;
  height: 492px;
  min-height: 492px;
  overflow: hidden;
  padding: 24px;
  border: 1px solid var(--border-soft);
  border-radius: 16px;
  background: var(--project-card-bg), var(--project-card-glow);
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
  background: var(--card-sheen);
  opacity: 0;
  pointer-events: none;
  transition:
    opacity 200ms ease-out,
    transform 200ms ease-out;
  transform: translateY(8px);
}

.project-card:hover,
.project-card:focus-visible {
  transform: translateY(-4px);
  border-color: color-mix(in srgb, var(--accent-line) 82%, #ffffff 18%);
  box-shadow:
    0 28px 54px color-mix(in srgb, #0f172a 18%, transparent),
    0 12px 26px color-mix(in srgb, var(--accent-end) 16%, transparent);
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
  font-size: 1.68rem;
  font-weight: 700;
  line-height: 1.08;
  letter-spacing: -0.02em;
  color: color-mix(in srgb, var(--text-main) 92%, var(--accent-end) 8%);
  overflow: hidden;
  display: block;
  white-space: nowrap;
  text-overflow: ellipsis;
  transition:
    color 200ms ease-out,
    text-shadow 200ms ease-out;
}

.project-card:hover .project-card__name,
.project-card:focus-visible .project-card__name {
  color: color-mix(in srgb, var(--text-main) 98%, #ffffff 2%);
  text-shadow: 0 1px 0 color-mix(in srgb, #ffffff 22%, transparent);
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
  transition:
    transform 200ms ease-out,
    box-shadow 200ms ease-out,
    border-color 200ms ease-out,
    background-color 200ms ease-out,
    color 200ms ease-out;
}

.project-card:hover .project-card__status,
.project-card:focus-visible .project-card__status {
  transform: translateY(-1px);
  box-shadow:
    inset 0 1px 0 color-mix(in srgb, #ffffff 18%, transparent),
    0 10px 18px color-mix(in srgb, var(--accent-end) 16%, transparent);
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
  border-color: var(--work-status-pending-border);
  background: var(--work-status-pending-bg);
  color: var(--work-status-pending-color);
}

.project-card__status--active {
  border-color: var(--work-status-active-border);
  background: var(--work-status-active-bg);
  color: var(--work-status-active-color);
}

.project-card__status--done {
  border-color: var(--work-status-done-border);
  background: var(--work-status-done-bg);
  color: var(--work-status-done-color);
}

.project-card__status--delayed {
  border-color: var(--work-status-delayed-border);
  background: var(--work-status-delayed-bg);
  color: var(--work-status-delayed-color);
}

.project-card__status--unknown {
  border-color: var(--work-status-unknown-border);
  background: var(--work-status-unknown-bg);
  color: var(--work-status-unknown-color);
}

.project-card__description {
  margin: 0;
  line-height: 1.56;
  color: color-mix(in srgb, var(--text-main) 72%, var(--text-soft));
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

.project-card__meta-item {
  position: relative;
  min-width: 0;
  min-height: 68px;
  display: grid;
  align-content: start;
  gap: 2px;
  padding: 10px 12px;
  border: 1px solid color-mix(in srgb, var(--border-soft) 90%, transparent);
  border-radius: 14px;
  background: var(--dialog-control-bg);
  box-shadow: none;
  overflow: hidden;
}

.project-card__meta-item::after {
  content: attr(data-tooltip);
  position: absolute;
  top: 10px;
  right: 10px;
  padding: 5px 8px;
  border: 1px solid var(--dialog-control-border);
  border-radius: 999px;
  background: var(--dialog-control-bg-strong);
  color: var(--text-main);
  font-size: 0.7rem;
  font-weight: 700;
  line-height: 1;
  letter-spacing: 0.05em;
  white-space: nowrap;
  box-shadow: var(--dialog-control-shadow);
  backdrop-filter: var(--backdrop-blur);
  opacity: 0;
  pointer-events: none;
  transform: translateY(6px);
  transition:
    opacity 180ms ease-out,
    transform 180ms ease-out;
}

.project-card__meta-item:hover::after {
  opacity: 1;
  transform: translateY(0);
}

.project-card__meta dt {
  display: flex;
  align-items: center;
  margin: 0;
  min-height: 28px;
  color: var(--text-soft);
}

.project-card__meta dd {
  margin: 2px 0 0;
  padding-left: 2px;
  min-width: 0;
  font-size: 0.98rem;
  font-weight: 700;
  line-height: 1.24;
  color: var(--text-main);
}

.project-card__meta-icon {
  width: 28px;
  height: 28px;
  display: inline-grid;
  place-items: center;
  border: 0;
  border-radius: 0;
  background: none;
  color: color-mix(in srgb, var(--text-main) 74%, var(--accent-end) 26%);
  box-shadow: none;
}

.project-card__meta-icon svg {
  width: 15px;
  height: 15px;
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
  gap: 1px;
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
  align-items: center;
}

.project-card__footer :deep(.project-actions) {
  width: 100%;
  margin-top: 0;
}

@media (max-width: 720px) {
  .project-grid {
    grid-template-columns: minmax(0, 1fr);
  }

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
