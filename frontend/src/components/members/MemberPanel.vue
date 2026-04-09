<script setup lang="ts">
import type { ProjectMember } from '@/types/member'

withDefaults(defineProps<{
  canManage: boolean
  fixedHeight?: boolean
  members: ProjectMember[]
}>(), {
  fixedHeight: false,
})

defineEmits<{
  add: []
  remove: [userId: number]
}>()
</script>

<template>
  <section
    :class="['member-panel', { 'member-panel--fixed': fixedHeight }]"
    data-testid="member-panel"
  >
    <header>
      <div>
        <p>项目成员</p>
        <span>{{ members.length }} 人</span>
      </div>
      <button
        v-if="canManage"
        data-testid="add-member"
        type="button"
        @click="$emit('add')"
      >
        添加成员
      </button>
    </header>

    <div class="member-panel__list" data-testid="member-panel-list">
      <article v-for="member in members" :key="member.user_id" class="member-panel__item">
        <div>
          <strong>{{ member.real_name }}</strong>
          <p>{{ member.username }}</p>
        </div>
        <div class="member-panel__actions">
          <span>{{ member.is_owner ? '负责人' : '成员' }}</span>
          <button
            v-if="canManage && !member.is_owner"
            :data-testid="`remove-member-${member.user_id}`"
            type="button"
            @click="$emit('remove', member.user_id)"
          >
            移除
          </button>
        </div>
      </article>
    </div>
  </section>
</template>

<style scoped>
.member-panel {
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
  gap: 12px;
  min-height: 0;
  padding: 18px;
  border: 1px solid var(--border-soft);
  border-radius: 18px;
  background: var(--glass-bg);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
}

.member-panel--fixed {
  height: 720px;
  min-height: 720px;
}

header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

header p,
header span {
  margin: 0;
}

header p {
  font-weight: 700;
}

header div {
  display: grid;
  gap: 4px;
}

.member-panel__list {
  min-height: 0;
  display: grid;
  align-content: start;
  gap: 12px;
  overflow-y: auto;
  padding-right: 6px;
}

.member-panel__item {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  padding: 12px 14px;
  border: 1px solid var(--border-soft);
  border-radius: 14px;
  background: color-mix(in srgb, var(--panel-bg) 88%, transparent);
}

.member-panel__actions {
  display: grid;
  justify-items: end;
  gap: 8px;
}

.member-panel__item strong,
.member-panel__item p {
  margin: 0;
}

button {
  border: 1px solid var(--border-soft);
  border-radius: 10px;
  padding: 8px 12px;
  background: color-mix(in srgb, var(--accent-start) 12%, transparent);
  color: var(--text-main);
  cursor: pointer;
}

.member-panel__item p,
.member-panel__item span {
  color: var(--text-soft);
}

@media (max-width: 1180px) {
  .member-panel--fixed {
    height: auto;
    min-height: 0;
  }
}
</style>
