<script setup lang="ts">
import { useRouter } from 'vue-router'

import ThemeToggle from '@/components/ThemeToggle.vue'
import { getErrorMessage } from '@/api/http'
import { useAuthStore } from '@/stores/auth'
import { useNotificationStore } from '@/stores/notifications'

const router = useRouter()
const authStore = useAuthStore()
const notificationStore = useNotificationStore()

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
    <aside class="app-shell__sidebar">
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
      </nav>

      <div class="app-shell__sidebar-foot">
        <p class="app-shell__foot-label">视觉模式</p>
        <ThemeToggle />
      </div>
    </aside>

    <div class="app-shell__content">
      <header class="app-shell__header">
        <div class="app-shell__header-copy">
          <p class="app-shell__header-label">当前登录</p>
          <strong>{{ authStore.currentUser?.real_name ?? '未登录' }}</strong>
          <span>数据已同步到当前工作区</span>
        </div>

        <div class="app-shell__toolbar">
          <ThemeToggle class="app-shell__toolbar-theme" />
          <button class="app-shell__logout" type="button" @click="handleLogout">退出登录</button>
        </div>
      </header>

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
  background: var(--sidebar-bg);
  box-shadow: var(--shadow-panel);
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
  box-shadow: 0 12px 24px rgba(10, 102, 255, 0.24);
}

.app-shell__brand-mark::after {
  content: '';
  position: absolute;
  inset: 1px;
  border-radius: 13px;
  border: 1px solid rgba(255, 255, 255, 0.24);
}

.app-shell__brand-mark span {
  position: absolute;
  inset: 11px;
  border-radius: 10px;
  border: 1.5px solid rgba(255, 255, 255, 0.84);
}

.app-shell__brand h1 {
  font-size: 1.75rem;
}

.app-shell__eyebrow {
  margin-bottom: 8px;
  letter-spacing: 0.16em;
  text-transform: uppercase;
  color: var(--text-soft);
  font-size: 0.75rem;
}

.app-shell__brand-copy {
  margin-top: 10px;
  color: var(--text-soft);
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
  color: var(--text-soft);
  background: rgba(255, 255, 255, 0.04);
  border: 1px solid transparent;
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
  background: rgba(255, 255, 255, 0.08);
}

.app-shell__link-icon svg {
  width: 18px;
  height: 18px;
}

.app-shell__sidebar-foot {
  display: grid;
  gap: 12px;
  margin-top: auto;
  padding-top: 16px;
  border-top: 1px solid var(--border-soft);
}

.app-shell__foot-label {
  margin: 0;
  font-size: 0.78rem;
  letter-spacing: 0.12em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.app-shell__content {
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
  gap: 18px;
}

.app-shell__header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
  padding: 18px 22px;
  border: 1px solid var(--border-soft);
  border-radius: 22px;
  background: var(--glass-bg);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
}

.app-shell__header-copy {
  display: grid;
  gap: 4px;
}

.app-shell__header-copy strong {
  font-size: 1.2rem;
}

.app-shell__header-copy span {
  color: var(--text-soft);
}

.app-shell__header-label {
  margin: 0 0 6px;
  font-size: 0.8rem;
  letter-spacing: 0.14em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.app-shell__toolbar {
  display: inline-flex;
  align-items: center;
  gap: 12px;
}

.app-shell__toolbar-theme {
  display: none;
}

.app-shell__logout {
  border: 1px solid var(--border-soft);
  border-radius: 10px;
  padding: 10px 14px;
  background: var(--glass-bg-strong);
  color: var(--text-main);
  cursor: pointer;
}

.app-shell__logout:hover {
  transform: translateY(-2px);
  border-color: var(--accent-line);
  box-shadow: var(--shadow-panel-hover);
}

.app-shell__workspace {
  min-width: 0;
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
    grid-template-columns: minmax(0, 1fr) auto auto;
    align-items: center;
  }

  .app-shell__sidebar-foot {
    margin-top: 0;
    padding-top: 0;
    border-top: none;
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

  .app-shell__header {
    flex-direction: column;
    align-items: stretch;
  }

  .app-shell__toolbar {
    justify-content: space-between;
  }

  .app-shell__toolbar-theme {
    display: inline-flex;
  }

  .app-shell__sidebar-foot .theme-toggle {
    display: none;
  }
}
</style>
