<script setup lang="ts">
import type { ProjectDetail } from '@/types/project'

defineProps<{
  project: ProjectDetail
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
</script>

<template>
  <section class="project-hero">
    <div>
      <p class="project-hero__eyebrow">Project Workspace</p>
      <h2>{{ project.name }}</h2>
      <p class="project-hero__description">
        {{ project.description || '当前项目未填写描述。' }}
      </p>
    </div>

    <div class="project-hero__meta">
      <div>
        <span>状态</span>
        <strong>{{ statusLabel(project.status) }}</strong>
      </div>
      <div>
        <span>负责人</span>
        <strong>{{ project.owner_real_name }}</strong>
      </div>
      <div>
        <span>周期</span>
        <strong>{{ project.planned_start_date }} - {{ project.planned_end_date }}</strong>
      </div>
    </div>
  </section>
</template>

<style scoped>
.project-hero {
  display: flex;
  align-items: start;
  justify-content: space-between;
  gap: 20px;
  padding: 24px 26px;
  border: 1px solid var(--border-soft);
  border-radius: 20px;
  background:
    linear-gradient(135deg, rgba(10, 102, 255, 0.22), rgba(0, 194, 255, 0.1)),
    color-mix(in srgb, var(--glass-bg) 94%, transparent);
  color: var(--text-main);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
}

.project-hero h2,
.project-hero p {
  margin: 0;
}

.project-hero__eyebrow {
  margin-bottom: 10px;
  font-size: 0.78rem;
  letter-spacing: 0.18em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.project-hero__description {
  margin-top: 12px;
  max-width: 520px;
  color: var(--text-soft);
}

.project-hero__meta {
  display: grid;
  gap: 10px;
}

.project-hero__meta div {
  display: grid;
  gap: 6px;
  padding: 14px 16px;
  min-width: 220px;
  border: 1px solid var(--border-soft);
  border-radius: 16px;
  background: color-mix(in srgb, var(--panel-bg) 86%, transparent);
}

.project-hero__meta span {
  font-size: 0.76rem;
  letter-spacing: 0.12em;
  text-transform: uppercase;
  color: var(--text-soft);
}

@media (max-width: 900px) {
  .project-hero {
    flex-direction: column;
  }

  .project-hero__meta {
    width: 100%;
    grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
  }
}
</style>
