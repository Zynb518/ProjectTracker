<script setup lang="ts">
defineProps<{
  keyword: string
  status: string
}>()

const emit = defineEmits<{
  'update:keyword': [value: string]
  'update:status': [value: string]
  submit: []
  create: []
}>()

function updateKeyword(event: Event) {
  emit('update:keyword', (event.target as HTMLInputElement).value)
}

function submitKeyword() {
  emit('submit')
}

function updateStatus(event: Event) {
  emit('update:status', (event.target as HTMLSelectElement).value)
  emit('submit')
}
</script>

<template>
  <section class="project-filters">
    <label class="project-filters__field">
      <span>项目关键字</span>
      <input
        :value="keyword"
        class="project-filters__input"
        placeholder="按项目名称搜索"
        type="text"
        @input="updateKeyword"
        @keydown.enter.prevent="submitKeyword"
      />
    </label>

    <label class="project-filters__field">
      <span>项目状态</span>
      <select
        :value="status"
        data-testid="projects-status"
        class="project-filters__input"
        @change="updateStatus"
      >
        <option value="">全部状态</option>
        <option value="1">未开始</option>
        <option value="2">进行中</option>
        <option value="3">已完成</option>
        <option value="4">已延期</option>
      </select>
    </label>

    <div class="project-filters__actions">
      <button type="button" class="primary" @click="emit('create')">新建项目</button>
    </div>
  </section>
</template>

<style scoped>
.project-filters {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr)) auto;
  gap: 14px;
  padding: 18px;
  border: 1px solid var(--border-soft);
  border-radius: 18px;
  background: var(--glass-bg);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
}

.project-filters__field {
  display: grid;
  gap: 8px;
}

.project-filters__field span {
  font-size: 0.76rem;
  font-weight: 600;
  letter-spacing: 0.08em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.project-filters__input {
  min-height: 46px;
  border: 1px solid var(--border-soft);
  border-radius: 8px;
  padding: 0 14px;
  background: color-mix(in srgb, var(--panel-bg) 92%, transparent);
  color: var(--text-main);
  font: inherit;
}

.project-filters__input::placeholder {
  color: var(--text-muted);
}

.project-filters__input:focus {
  border-color: var(--accent-line);
  box-shadow: 0 0 0 4px rgba(10, 102, 255, 0.12);
}

.project-filters__actions {
  display: flex;
  align-items: end;
  gap: 10px;
}

button {
  min-height: 46px;
  border: 1px solid var(--border-soft);
  border-radius: 10px;
  padding: 12px 16px;
  background: color-mix(in srgb, var(--panel-bg) 86%, transparent);
  color: var(--text-main);
  cursor: pointer;
}

button:hover {
  transform: translateY(-2px);
  border-color: var(--accent-line);
  box-shadow: var(--shadow-panel-hover);
}

.primary {
  border: none;
  background: var(--gradient-primary);
  color: var(--text-inverse);
  box-shadow: 0 16px 28px rgba(10, 102, 255, 0.2);
}

@media (max-width: 820px) {
  .project-filters {
    grid-template-columns: 1fr;
  }

  .project-filters__actions {
    align-items: stretch;
  }
}
</style>
