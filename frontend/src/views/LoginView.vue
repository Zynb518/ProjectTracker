<script setup lang="ts">
import { ref } from 'vue'
import { useRouter } from 'vue-router'

import ThemeToggle from '@/components/ThemeToggle.vue'
import { getErrorMessage } from '@/api/http'
import { useNotificationStore } from '@/stores/notifications'
import { useAuthStore } from '@/stores/auth'

const router = useRouter()
const authStore = useAuthStore()
const notificationStore = useNotificationStore()

const username = ref('')
const password = ref('')

async function submitLogin() {
  try {
    await authStore.login({
      username: username.value,
      password: password.value,
    })
    await router.push('/projects')
  } catch (error) {
    notificationStore.notifyError(getErrorMessage(error, '登录失败，请稍后重试'))
  }
}
</script>

<template>
  <section class="login-view">
    <header class="login-view__toolbar">
      <p class="login-view__brand">Project Tracker</p>
      <ThemeToggle />
    </header>

    <div class="login-shell">
      <section class="login-card login-card--hero">
        <p class="login-eyebrow">Control Surface</p>
        <h1>进入项目台</h1>
        <p class="login-copy">科技感、通透、轻量化的项目控制面板，统一查看项目、节点、成员与任务状态。</p>

        <div class="login-highlights">
          <article>
            <strong>双主题</strong>
            <span>浅色与深色模式平滑切换</span>
          </article>
          <article>
            <strong>高密度</strong>
            <span>保持信息聚合而不牺牲层次</span>
          </article>
          <article>
            <strong>实时调度</strong>
            <span>从项目到子任务的统一工作流</span>
          </article>
        </div>
      </section>

      <section class="login-card login-card--form">
        <p class="login-card__caption">账号登录</p>
        <form class="login-form" @submit.prevent="submitLogin">
          <label class="login-label" for="username">用户名</label>
          <input
            id="username"
            v-model="username"
            data-testid="login-username"
            class="login-input"
            autocomplete="username"
            name="username"
            placeholder="输入用户名"
            type="text"
          />

          <label class="login-label" for="password">密码</label>
          <input
            id="password"
            v-model="password"
            data-testid="login-password"
            class="login-input"
            autocomplete="current-password"
            name="password"
            placeholder="输入登录密码"
            type="password"
          />
          <button class="login-button" type="submit">进入项目台</button>
        </form>
      </section>
    </div>
  </section>
</template>

<style scoped>
.login-view {
  min-height: 100vh;
  display: grid;
  align-content: center;
  gap: 20px;
  padding: 28px;
}

.login-view__toolbar {
  width: min(100%, 1120px);
  margin: 0 auto;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
}

.login-view__brand {
  margin: 0;
  font-size: 0.82rem;
  font-weight: 600;
  letter-spacing: 0.18em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.login-shell {
  width: min(100%, 1120px);
  margin: 0 auto;
  display: grid;
  grid-template-columns: minmax(0, 1.1fr) minmax(360px, 420px);
  gap: 20px;
}

.login-card {
  position: relative;
  overflow: hidden;
  display: grid;
  gap: 16px;
  padding: 32px;
  border: 1px solid var(--border-soft);
  border-radius: 24px;
  background: var(--glass-bg);
  box-shadow: var(--shadow-glass);
  backdrop-filter: var(--backdrop-blur);
}

.login-card::after {
  content: '';
  position: absolute;
  inset: auto -10% -35% 38%;
  height: 180px;
  background: radial-gradient(circle, rgba(0, 194, 255, 0.18), transparent 65%);
  filter: blur(12px);
  pointer-events: none;
}

.login-card--hero {
  align-content: start;
  min-height: 520px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--glass-bg) 86%, transparent), var(--glass-bg)),
    radial-gradient(circle at top right, rgba(0, 194, 255, 0.14), transparent 28%);
}

.login-card--form {
  align-content: center;
}

.login-eyebrow {
  margin: 0;
  font-size: 0.8rem;
  letter-spacing: 0.22em;
  text-transform: uppercase;
  color: var(--text-soft);
}

h1 {
  margin: 0;
  font-size: clamp(2rem, 4vw, 3rem);
}

.login-copy {
  margin: 0;
  max-width: 44ch;
  color: var(--text-soft);
}

.login-highlights {
  display: grid;
  gap: 12px;
  margin-top: 12px;
}

.login-highlights article {
  display: grid;
  gap: 6px;
  padding: 14px 16px;
  border: 1px solid var(--border-soft);
  border-radius: 18px;
  background: var(--panel-bg-soft);
}

.login-highlights strong {
  font-size: 1rem;
}

.login-highlights span,
.login-card__caption {
  color: var(--text-soft);
}

.login-card__caption {
  margin: 0;
  font-size: 0.8rem;
  font-weight: 600;
  letter-spacing: 0.16em;
  text-transform: uppercase;
}

.login-form {
  display: grid;
  gap: 14px;
}

.login-label {
  font-size: 0.82rem;
  font-weight: 600;
  letter-spacing: 0.08em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.login-input {
  min-height: 48px;
  border: 1px solid var(--border-soft);
  border-radius: 8px;
  padding: 0 14px;
  background: color-mix(in srgb, var(--panel-bg) 92%, transparent);
  color: var(--text-main);
  font: inherit;
}

.login-input::placeholder {
  color: var(--text-muted);
}

.login-input:focus {
  border-color: var(--accent-line);
  box-shadow: 0 0 0 4px rgba(10, 102, 255, 0.12);
}

.login-button {
  min-height: 48px;
  margin-top: 8px;
  border: none;
  border-radius: 10px;
  padding: 12px 18px;
  background: var(--gradient-primary);
  color: var(--text-inverse);
  font: inherit;
  font-weight: 600;
  cursor: pointer;
  box-shadow: 0 16px 30px rgba(10, 102, 255, 0.22);
}

.login-button:hover {
  transform: translateY(-2px);
  box-shadow: 0 20px 34px rgba(10, 102, 255, 0.28);
}

@media (max-width: 920px) {
  .login-shell {
    grid-template-columns: 1fr;
  }

  .login-card--hero {
    min-height: auto;
  }
}
</style>
