<script setup lang="ts">
defineProps<{
  memberCount: number
  nodeCount: number
  completedNodeCount: number
  subTaskCount: number
  completedSubTaskCount: number
}>()
</script>

<template>
  <section class="workspace-stats">
    <article>
      <span>成员</span>
      <strong>{{ memberCount }}</strong>
    </article>
    <article>
      <span>阶段节点</span>
      <strong>{{ completedNodeCount }} / {{ nodeCount }}</strong>
    </article>
    <article>
      <span>子任务</span>
      <strong>{{ completedSubTaskCount }} / {{ subTaskCount }}</strong>
    </article>
  </section>
</template>

<style scoped>
.workspace-stats {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 14px;
}

article {
  display: grid;
  gap: 10px;
  padding: 18px;
  border: 1px solid var(--border-soft);
  border-radius: 16px;
  background: var(--glass-bg);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
}

span {
  font-size: 0.76rem;
  letter-spacing: 0.12em;
  text-transform: uppercase;
  color: var(--text-soft);
}

strong {
  font-size: 1.5rem;
}

@media (max-width: 900px) {
  .workspace-stats {
    grid-template-columns: 1fr;
  }
}
</style>
