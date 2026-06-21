<script setup lang="ts">
import type { ProjectMember } from '@/types/member'

defineProps<{
  canManage: boolean
  canTransferOwner: boolean
  members: ProjectMember[]
  modelValue: boolean
}>()

const emit = defineEmits<{
  add: []
  remove: [userId: number]
  'transfer-owner': []
  'update:modelValue': [value: boolean]
}>()
</script>

<template>
  <Transition appear name="member-drawer-slide">
    <div
      v-if="modelValue"
      class="member-drawer__backdrop"
      @click.self="emit('update:modelValue', false)"
    >
      <aside class="member-drawer">
        <header class="member-drawer__header">
          <div class="member-drawer__header-copy">
            <p>工单协作人</p>
            <h2>{{ members.length }} 人</h2>
          </div>
          <div class="member-drawer__header-actions">
            <button
              v-if="canManage"
              class="member-drawer__add-btn"
              type="button"
              @click="emit('add')"
            >+ 添加协作人</button>
            <button
              class="member-drawer__close-btn"
              type="button"
              @click="emit('update:modelValue', false)"
            >关闭</button>
          </div>
        </header>

        <div v-if="members.length === 0" class="member-drawer__empty">
          暂无工单协作人
        </div>

        <article
          v-for="member in members"
          :key="member.user_id"
          class="member-drawer__item"
        >
          <div class="member-drawer__item-info">
            <strong class="member-drawer__item-name">{{ member.real_name }}</strong>
            <span class="member-drawer__item-username">@{{ member.username }}</span>
            <span
              :class="[
                'member-drawer__item-badge',
                { 'member-drawer__item-badge--owner': member.is_owner },
              ]"
            >
              {{ member.is_owner ? '负责人' : '成员' }}
            </span>
          </div>
          <div class="member-drawer__item-actions">
            <button
              v-if="member.is_owner && canTransferOwner"
              class="member-drawer__action-btn member-drawer__action-btn--transfer"
              type="button"
              :data-testid="'transfer-owner-' + member.user_id"
              @click="emit('transfer-owner')"
            >转交</button>
            <button
              v-if="!member.is_owner && canManage"
              class="member-drawer__action-btn member-drawer__action-btn--remove"
              type="button"
              :data-testid="'remove-member-' + member.user_id"
              @click="emit('remove', member.user_id)"
            >移除</button>
          </div>
        </article>
      </aside>
    </div>
  </Transition>
</template>

<style scoped>
.member-drawer-slide-enter-active,
.member-drawer-slide-leave-active {
  transition: opacity 240ms ease-out;
}

.member-drawer-slide-enter-active .member-drawer,
.member-drawer-slide-leave-active .member-drawer {
  transition:
    transform 320ms cubic-bezier(0.16, 1, 0.3, 1),
    opacity 240ms ease-out;
}

.member-drawer-slide-enter-from,
.member-drawer-slide-leave-to {
  opacity: 0;
}

.member-drawer-slide-enter-from .member-drawer,
.member-drawer-slide-leave-to .member-drawer {
  opacity: 0;
  transform: translateX(24px);
}

.member-drawer__backdrop {
  position: fixed;
  inset: 0;
  z-index: 42;
  display: flex;
  justify-content: flex-end;
  padding-left: 40px;
  background: linear-gradient(
    90deg,
    rgba(5, 9, 18, 0),
    color-mix(in srgb, var(--overlay-backdrop) 72%, transparent) 24%,
    var(--overlay-backdrop)
  );
  backdrop-filter: blur(6px);
}

.member-drawer {
  height: 100%;
  width: min(100%, 420px);
  display: grid;
  align-content: start;
  gap: 12px;
  padding: 24px;
  border-left: 1px solid var(--dialog-surface-border);
  background: var(--dialog-surface-glow), var(--drawer-surface-bg);
  box-shadow: -24px 0 48px rgba(0, 0, 0, 0.22);
  backdrop-filter: var(--backdrop-blur);
}

.member-drawer__header {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 12px;
}

.member-drawer__header-copy {
  display: grid;
  gap: 4px;
}

.member-drawer__header-copy p {
  margin: 0;
  color: var(--text-soft);
  font-size: 0.76rem;
  letter-spacing: 0.12em;
  text-transform: uppercase;
}

.member-drawer__header-copy h2 {
  margin: 0;
  font-size: 1.1rem;
  color: var(--text-main);
}

.member-drawer__header-actions {
  display: inline-flex;
  align-items: center;
  gap: 8px;
  flex-shrink: 0;
}

.member-drawer__add-btn,
.member-drawer__close-btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  height: 36px;
  padding: 0 14px;
  border: 1px solid var(--dialog-control-border);
  border-radius: 10px;
  font-size: 0.8rem;
  font-weight: 700;
  cursor: pointer;
  white-space: nowrap;
  transition: transform 120ms ease-out, border-color 120ms ease-out, box-shadow 120ms ease-out;
}

.member-drawer__add-btn {
  background: linear-gradient(135deg, var(--accent-start), var(--accent-end));
  border-color: transparent;
  color: #f8fbff;
  box-shadow: 0 4px 12px color-mix(in srgb, var(--accent-end) 18%, transparent);
}

.member-drawer__add-btn:hover {
  transform: translateY(-1px);
  box-shadow: 0 8px 18px color-mix(in srgb, var(--accent-end) 24%, transparent);
}

.member-drawer__close-btn {
  background: var(--dialog-control-bg-strong);
  color: var(--text-soft);
}

.member-drawer__close-btn:hover {
  border-color: color-mix(in srgb, var(--accent-line) 42%, var(--dialog-control-border));
  color: var(--text-main);
}

.member-drawer__empty {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 120px;
  color: var(--text-soft);
  font-size: 0.88rem;
}

.member-drawer__item {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  padding: 12px 14px;
  border: 1px solid var(--dialog-control-border);
  border-radius: 14px;
  background: var(--drawer-item-bg);
  box-shadow: var(--dialog-control-shadow);
  transition: border-color 160ms ease-out;
}

.member-drawer__item:hover {
  border-color: color-mix(in srgb, var(--accent-line) 28%, var(--dialog-control-border));
}

.member-drawer__item-info {
  display: flex;
  align-items: center;
  gap: 8px;
  min-width: 0;
  overflow: hidden;
}

.member-drawer__item-name {
  font-size: 0.9rem;
  color: var(--text-main);
  white-space: nowrap;
}

.member-drawer__item-username {
  color: var(--text-soft);
  font-size: 0.78rem;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.member-drawer__item-badge {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-width: 48px;
  height: 22px;
  padding: 0 8px;
  border-radius: 999px;
  background: color-mix(in srgb, var(--accent-end) 10%, transparent);
  color: var(--text-soft);
  font-size: 0.7rem;
  font-weight: 700;
  white-space: nowrap;
  flex-shrink: 0;
}

.member-drawer__item-badge--owner {
  background: color-mix(in srgb, var(--accent-warning) 16%, transparent);
  color: var(--accent-warning);
}

.member-drawer__item-actions {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  flex-shrink: 0;
}

.member-drawer__action-btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  height: 28px;
  padding: 0 10px;
  border: 1px solid var(--dialog-control-border);
  border-radius: 8px;
  background: var(--dialog-control-bg);
  color: var(--text-soft);
  font-size: 0.74rem;
  font-weight: 700;
  cursor: pointer;
  white-space: nowrap;
  transition: background 120ms ease-out, color 120ms ease-out, border-color 120ms ease-out;
}

.member-drawer__action-btn--transfer:hover {
  background: color-mix(in srgb, var(--accent-warning) 12%, transparent);
  color: var(--accent-warning);
  border-color: color-mix(in srgb, var(--accent-warning) 20%, transparent);
}

.member-drawer__action-btn--remove:hover {
  background: color-mix(in srgb, #ff6b7a 12%, transparent);
  color: #ff6b7a;
  border-color: color-mix(in srgb, #ff6b7a 20%, transparent);
}
</style>
