<script setup lang="ts">
import { useRouter } from 'vue-router'

import ThemeToggle from '@/components/ThemeToggle.vue'
import { getErrorMessage } from '@/api/http'
import { useAuthStore } from '@/stores/auth'
import { useNotificationStore } from '@/stores/notifications'
import { getSystemRoleLabel } from '@/utils/display'

const router = useRouter()
const authStore = useAuthStore()
const notificationStore = useNotificationStore()

function userInitial(name?: string | null) {
  return name?.trim().charAt(0) || '访'
}

async function handleLogout() {
  try {
    await authStore.logout()
    await router.push('/login')
  } catch (error) {
    notificationStore.notifyError(getErrorMessage(error, '退出登录失败'))
  }
}
</script>

<template>
  <div class="app-shell">
    <aside v-smooth-wheel class="app-shell__sidebar smooth-scroll-surface">
      <div class="app-shell__brand">
        <div class="app-shell__brand-mark" aria-hidden="true">
          <span />
        </div>
        <div>
          <p class="app-shell__eyebrow">Project Tracker</p>
          <h1>项目台</h1>
          <p class="app-shell__brand-copy">科技感进度工作台，统一项目、节点与任务视图。</p>
        </div>
      </div>

      <nav class="app-shell__nav">
        <RouterLink class="app-shell__link" to="/projects">
          <span class="app-shell__link-icon" aria-hidden="true">
            <svg viewBox="0 0 24 24">
              <path
                d="M4 6.5h16M4 12h16M4 17.5h10"
                fill="none"
                stroke="currentColor"
                stroke-linecap="round"
                stroke-width="1.5"
              />
            </svg>
          </span>
          <span>项目列表</span>
        </RouterLink>
        <RouterLink class="app-shell__link" to="/my-tasks">
          <span class="app-shell__link-icon" aria-hidden="true">
            <svg viewBox="0 0 24 24">
              <path
                d="M8 7h11M8 12h11M8 17h11M4 7h.01M4 12h.01M4 17h.01"
                fill="none"
                stroke="currentColor"
                stroke-linecap="round"
                stroke-width="1.5"
              />
            </svg>
          </span>
          <span>我的任务</span>
        </RouterLink>
        <RouterLink
          v-if="authStore.currentUser?.system_role === 1"
          class="app-shell__link"
          to="/users"
        >
          <span class="app-shell__link-icon" aria-hidden="true">
            <svg viewBox="0 0 24 24">
              <path
                d="M15.5 18.5h4 M17.5 16.5v4 M4 19a4.5 4.5 0 0 1 9 0 M8.5 11.5a3 3 0 1 0 0-6 3 3 0 0 0 0 6Z M17 10.5a2.5 2.5 0 1 0 0-5 2.5 2.5 0 0 0 0 5Z"
                fill="none"
                stroke="currentColor"
                stroke-linecap="round"
                stroke-linejoin="round"
                stroke-width="1.5"
              />
            </svg>
          </span>
          <span>用户管理</span>
        </RouterLink>
      </nav>

      <div class="app-shell__sidebar-foot">
        <section class="app-shell__sidebar-account" data-testid="sidebar-account">
          <p class="app-shell__foot-label">当前会话</p>

          <div class="app-shell__account-card">
            <div class="app-shell__account-avatar" aria-hidden="true">
              {{ userInitial(authStore.currentUser?.real_name) }}
            </div>

            <div class="app-shell__account-copy">
              <strong>{{ authStore.currentUser?.real_name ?? '未登录' }}</strong>
              <span>{{ getSystemRoleLabel(authStore.currentUser?.system_role) }}</span>
            </div>
          </div>

          <p class="app-shell__account-note">数据已同步到当前工作区，当前会话状态正常。</p>

          <div class="app-shell__account-actions">
            <ThemeToggle />
            <button class="app-shell__logout" type="button" @click="handleLogout">退出登录</button>
          </div>
        </section>
      </div>
    </aside>

    <div class="app-shell__content">
      <main class="app-shell__workspace">
        <RouterView />
      </main>
    </div>
  </div>
</template>

<style scoped>
.app-shell {
  min-height: 100vh;
  display: grid;
  grid-template-columns: 288px minmax(0, 1fr);
  gap: 24px;
  padding: 20px;
}

.app-shell__sidebar {
  position: sticky;
  top: 20px;
  align-self: start;
  height: calc(100vh - 40px);
  max-height: calc(100vh - 40px);
  overflow-y: auto;
  display: grid;
  align-content: start;
  gap: 24px;
  padding: 24px 20px;
  border: 1px solid var(--border-soft);
  border-radius: 24px;
  background: linear-gradient(180deg, color-mix(in srgb, var(--sidebar-bg) 96%, transparent), color-mix(in srgb, var(--panel-bg-soft) 84%, transparent));
  box-shadow: var(--shadow-glass);
  backdrop-filter: var(--backdrop-blur);
}

.app-shell__brand {
  display: grid;
  grid-template-columns: auto minmax(0, 1fr);
  gap: 14px;
  align-items: start;
}

.app-shell__brand h1,
.app-shell__brand p {
  margin: 0;
}

.app-shell__brand-mark {
  position: relative;
  width: 46px;
  height: 46px;
  border-radius: 14px;
  background: var(--gradient-primary);
  box-shadow:
    0 12px 24px color-mix(in srgb, var(--accent-start) 24%, transparent),
    0 0 0 1px color-mix(in srgb, var(--accent-end) 18%, transparent);
}

.app-shell__brand-mark::after {
  content: '';
  position: absolute;
  inset: 1px;
  border-radius: 13px;
  border: 1px solid color-mix(in srgb, var(--text-inverse) 24%, transparent);
}

.app-shell__brand-mark span {
  position: absolute;
  inset: 11px;
  border-radius: 10px;
  border: 1.5px solid color-mix(in srgb, var(--text-inverse) 82%, transparent);
  box-shadow: 0 0 16px color-mix(in srgb, var(--accent-start) 16%, transparent);
}

.app-shell__brand h1 {
  font-size: 1.88rem;
  line-height: 1.06;
  color: var(--text-main);
}

.app-shell__eyebrow {
  margin-bottom: 8px;
  letter-spacing: 0.16em;
  text-transform: uppercase;
  color: color-mix(in srgb, var(--text-main) 66%, var(--text-soft));
  font-size: 0.82rem;
}

.app-shell__brand-copy {
  margin-top: 10px;
  color: color-mix(in srgb, var(--text-main) 72%, var(--text-soft));
  line-height: 1.62;
}

.app-shell__nav {
  display: grid;
  gap: 10px;
}

.app-shell__link {
  display: grid;
  grid-template-columns: auto minmax(0, 1fr);
  align-items: center;
  gap: 12px;
  padding: 14px 16px;
  border-radius: 16px;
  color: color-mix(in srgb, var(--text-main) 78%, var(--text-soft));
  background: linear-gradient(180deg, color-mix(in srgb, var(--panel-bg-soft) 92%, transparent), color-mix(in srgb, var(--glass-bg) 84%, transparent));
  border: 1px solid color-mix(in srgb, var(--border-soft) 86%, transparent);
  box-shadow: inset 0 1px 0 color-mix(in srgb, var(--text-inverse) 18%, transparent);
}

.app-shell__link:hover,
.router-link-active.app-shell__link {
  transform: translateY(-2px);
  color: var(--text-main);
  border-color: var(--accent-line);
  background: var(--gradient-primary-soft);
  box-shadow: var(--shadow-panel-hover);
}

.app-shell__link-icon {
  display: inline-grid;
  place-items: center;
  width: 34px;
  height: 34px;
  border-radius: 12px;
  background: color-mix(in srgb, var(--panel-bg) 88%, transparent);
  box-shadow:
    inset 0 0 0 1px color-mix(in srgb, var(--border-soft) 88%, transparent),
    0 10px 18px color-mix(in srgb, var(--accent-start) 5%, transparent);
}

.app-shell__link-icon svg {
  width: 18px;
  height: 18px;
}

.app-shell__sidebar-foot {
  margin-top: auto;
  padding-top: 16px;
  border-top: 1px solid var(--border-soft);
}

.app-shell__foot-label {
  margin: 0;
  font-size: 0.82rem;
  letter-spacing: 0.12em;
  text-transform: uppercase;
  color: color-mix(in srgb, var(--text-main) 66%, var(--text-soft));
}

.app-shell__sidebar-account {
  display: grid;
  gap: 14px;
}

.app-shell__account-card {
  display: grid;
  grid-template-columns: auto minmax(0, 1fr);
  gap: 12px;
  align-items: center;
  padding: 14px;
  border: 1px solid color-mix(in srgb, var(--border-soft) 88%, rgba(255, 255, 255, 0.08));
  border-radius: 18px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--glass-bg-strong) 96%, transparent), var(--glass-bg-strong)),
    radial-gradient(circle at top right, color-mix(in srgb, var(--accent-start) 18%, transparent), transparent 46%),
    radial-gradient(circle at 0% 100%, color-mix(in srgb, var(--accent-tertiary) 14%, transparent), transparent 40%);
  box-shadow:
    inset 0 1px 0 color-mix(in srgb, var(--text-inverse) 8%, transparent),
    0 18px 30px rgba(3, 10, 24, 0.12);
}

.app-shell__account-avatar {
  width: 44px;
  height: 44px;
  display: grid;
  place-items: center;
  border-radius: 14px;
  background: var(--gradient-primary);
  color: var(--text-inverse);
  font-size: 1rem;
  font-weight: 700;
  box-shadow:
    0 12px 24px color-mix(in srgb, var(--accent-start) 22%, transparent),
    0 0 0 1px color-mix(in srgb, var(--accent-end) 16%, transparent);
}

.app-shell__account-copy {
  min-width: 0;
  display: grid;
  gap: 6px;
}

.app-shell__account-copy strong {
  font-size: 1.06rem;
  line-height: 1.2;
}

.app-shell__account-copy span {
  justify-self: start;
  padding: 6px 10px;
  border: 1px solid color-mix(in srgb, var(--accent-start) 26%, var(--accent-tertiary) 14%);
  border-radius: 999px;
  background: var(--gradient-primary-soft);
  color: var(--text-main);
  font-size: 0.8rem;
  font-weight: 600;
  letter-spacing: 0.05em;
}

.app-shell__account-note {
  margin: 0;
  color: color-mix(in srgb, var(--text-main) 74%, var(--text-soft));
  line-height: 1.62;
}

.app-shell__account-actions {
  display: grid;
  gap: 12px;
}

.app-shell__content {
  min-width: 0;
}

.app-shell__logout {
  width: 100%;
  border: 1px solid var(--border-soft);
  border-radius: 14px;
  padding: 12px 14px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--glass-bg-strong) 94%, transparent), var(--glass-bg-strong)),
    radial-gradient(circle at right top, color-mix(in srgb, var(--accent-warning) 18%, transparent), transparent 52%),
    radial-gradient(circle at left center, color-mix(in srgb, var(--accent-tertiary) 10%, transparent), transparent 44%);
  color: color-mix(in srgb, var(--text-main) 92%, var(--text-soft));
  font: inherit;
  font-weight: 600;
  cursor: pointer;
  transition:
    transform 200ms ease-out,
    border-color 200ms ease-out,
    box-shadow 200ms ease-out,
    background 200ms ease-out;
}

.app-shell__logout:hover {
  transform: translateY(-2px);
  border-color: color-mix(in srgb, var(--accent-warning) 36%, transparent);
  box-shadow:
    0 14px 30px rgba(3, 10, 24, 0.18),
    0 0 0 1px color-mix(in srgb, var(--accent-warning) 14%, transparent);
}

.app-shell__workspace {
  min-width: 0;
}

:global(html.dark) .app-shell__sidebar {
  box-shadow:
    var(--shadow-glass),
    inset 0 1px 0 rgba(224, 224, 255, 0.05),
    0 0 34px rgba(157, 0, 255, 0.14);
}

:global(html.dark) .app-shell__brand-copy,
:global(html.dark) .app-shell__account-note {
  color: color-mix(in srgb, var(--text-main) 82%, var(--text-soft));
}

:global(html.dark) .app-shell__link {
  text-shadow: 0 0 10px rgba(0, 240, 255, 0.05);
}

:global(html.dark) .app-shell__link:hover,
:global(html.dark) .router-link-active.app-shell__link {
  box-shadow:
    var(--shadow-panel-hover),
    inset 0 1px 0 rgba(224, 224, 255, 0.06);
}

:global(html.dark) .app-shell__link-icon {
  color: color-mix(in srgb, var(--text-main) 88%, var(--accent-start) 12%);
}

:global(html.dark) .app-shell__account-card,
:global(html.dark) .app-shell__logout {
  box-shadow:
    var(--shadow-panel),
    inset 0 1px 0 rgba(224, 224, 255, 0.05);
}

@media (max-width: 1100px) {
  .app-shell {
    grid-template-columns: 1fr;
  }

  .app-shell__sidebar {
    position: static;
    top: auto;
    height: auto;
    max-height: none;
    overflow-y: visible;
  }

  .app-shell__sidebar-foot {
    margin-top: 0;
  }

  .app-shell__content {
    min-width: 0;
  }
}

@media (max-width: 800px) {
  .app-shell {
    gap: 16px;
    padding: 14px;
  }

  .app-shell__sidebar {
    grid-template-columns: 1fr;
  }

  .app-shell__content {
    min-width: 0;
  }
}
</style>
