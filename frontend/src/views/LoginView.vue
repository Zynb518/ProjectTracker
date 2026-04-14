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
    <div aria-hidden="true" class="login-view__background" data-testid="login-background" />

    <header class="login-view__toolbar">
      <p class="login-view__brand">Project Tracker</p>
      <ThemeToggle />
    </header>

    <section class="login-card">
      <div class="login-card__header">
        <p class="login-card__caption">账号登录</p>
        <h1>进入项目台</h1>
      </div>

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
  </section>
</template>

<style scoped>
.login-view {
  min-height: 100vh;
  display: grid;
  justify-items: center;
  align-content: center;
  gap: 24px;
  padding: clamp(32px, 7vw, 56px) 24px;
  position: relative;
  isolation: isolate;
}

.login-view__background {
  position: absolute;
  inset: 0;
  pointer-events: none;
  z-index: 0;
  background: var(--panel-bg);
}

.login-view__toolbar {
  width: min(100%, 440px);
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  position: relative;
  z-index: 1;
}

.login-view__brand {
  margin: 0;
  font-size: 0.8rem;
  font-weight: 700;
  letter-spacing: 0.24em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.login-card {
  width: min(100%, 440px);
  position: relative;
  overflow: hidden;
  display: grid;
  gap: 22px;
  padding: 30px 28px 28px;
  border: 1px solid color-mix(in srgb, var(--border-soft) 96%, transparent);
  border-radius: 26px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--glass-bg-strong) 92%, #ffffff 8%), color-mix(in srgb, var(--glass-bg) 94%, transparent)),
    radial-gradient(circle at top right, color-mix(in srgb, var(--accent-end) 12%, transparent), transparent 34%);
  box-shadow:
    0 22px 44px color-mix(in srgb, #0f172a 12%, transparent),
    inset 0 1px 0 color-mix(in srgb, #ffffff 34%, transparent);
  backdrop-filter: var(--backdrop-blur);
  z-index: 1;
}

.login-card::before {
  content: '';
  position: absolute;
  inset: 0;
  background:
    linear-gradient(148deg, rgba(255, 255, 255, 0.34), rgba(255, 255, 255, 0) 36%),
    radial-gradient(circle at 82% 18%, rgba(102, 212, 255, 0.18), transparent 22%);
  pointer-events: none;
}

.login-card::after {
  content: '';
  position: absolute;
  inset: auto -18% -32% 42%;
  height: 160px;
  background: radial-gradient(circle, color-mix(in srgb, var(--accent-end) 18%, transparent), transparent 64%);
  filter: blur(14px);
  pointer-events: none;
}

.login-card__header {
  display: grid;
  gap: 10px;
}

h1 {
  margin: 0;
  font-size: clamp(1.8rem, 4vw, 2.3rem);
  line-height: 1.05;
}

.login-card__caption {
  margin: 0;
  font-size: 0.76rem;
  font-weight: 700;
  letter-spacing: 0.18em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.login-form {
  display: grid;
  gap: 14px;
  padding-top: 2px;
}

.login-label {
  font-size: 0.78rem;
  font-weight: 700;
  letter-spacing: 0.1em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.login-input {
  min-height: 50px;
  border: 1px solid color-mix(in srgb, var(--border-soft) 96%, transparent);
  border-radius: 14px;
  padding: 0 16px;
  background: color-mix(in srgb, var(--panel-bg) 90%, #ffffff 10%);
  color: var(--text-main);
  font: inherit;
  box-shadow: inset 0 1px 0 color-mix(in srgb, #ffffff 40%, transparent);
  transition:
    border-color 180ms ease-out,
    box-shadow 180ms ease-out,
    background-color 180ms ease-out;
}

.login-input::placeholder {
  color: color-mix(in srgb, var(--text-muted) 92%, transparent);
}

.login-input:focus {
  border-color: color-mix(in srgb, var(--accent-line) 62%, transparent);
  background: color-mix(in srgb, var(--glass-bg-strong) 96%, #ffffff 4%);
  box-shadow:
    inset 0 1px 0 color-mix(in srgb, #ffffff 52%, transparent),
    0 0 0 4px color-mix(in srgb, var(--accent-start) 8%, transparent);
}

.login-button {
  min-height: 52px;
  margin-top: 10px;
  border: none;
  border-radius: 14px;
  padding: 12px 18px;
  background: var(--gradient-primary);
  color: var(--text-inverse);
  font: inherit;
  font-weight: 700;
  letter-spacing: 0.03em;
  cursor: pointer;
  box-shadow:
    inset 0 1px 0 color-mix(in srgb, #ffffff 18%, transparent),
    0 16px 30px color-mix(in srgb, var(--accent-start) 22%, transparent);
  transition:
    transform 180ms ease-out,
    box-shadow 180ms ease-out,
    filter 180ms ease-out;
}

.login-button:hover {
  box-shadow:
    inset 0 1px 0 color-mix(in srgb, #ffffff 22%, transparent),
    0 22px 36px color-mix(in srgb, var(--accent-start) 26%, transparent);
  filter: saturate(1.03);
}

@media (max-width: 560px) {
  .login-view {
    padding: 24px 16px;
    gap: 18px;
  }

  .login-card {
    padding: 24px 20px 20px;
    border-radius: 22px;
  }
}
</style>

<style>
html.light .login-view,
html:not(.dark) .login-view {
  gap: 28px;
}

html.light .login-view__background,
html:not(.dark) .login-view__background {
  background-color: #091321;
  background-image:
    linear-gradient(180deg, rgba(10, 19, 39, 0.96) 0%, rgba(28, 50, 86, 0.9) 32%, rgba(87, 120, 160, 0.66) 68%, rgba(232, 242, 255, 0.3) 100%),
    radial-gradient(circle at 50% 100%, rgba(255, 212, 163, 0.46), transparent 34%),
    linear-gradient(127deg, rgba(132, 228, 255, 0) 30%, rgba(132, 228, 255, 0.28) 58%, rgba(214, 170, 255, 0.16) 74%, rgba(214, 170, 255, 0) 88%),
    radial-gradient(circle at 20% 18%, rgba(143, 190, 255, 0.18), transparent 22%),
    linear-gradient(180deg, rgba(4, 10, 24, 0.72), rgba(4, 10, 24, 0.18));
}

html.light .login-view__toolbar,
html:not(.dark) .login-view__toolbar {
  padding-inline: 4px;
}

html.light .login-view__brand,
html:not(.dark) .login-view__brand {
  color: rgba(227, 239, 255, 0.88);
  text-shadow: 0 10px 28px rgba(6, 12, 28, 0.26);
}

html.light .login-card,
html:not(.dark) .login-card {
  border: 1px solid rgba(176, 211, 246, 0.34);
  border-radius: 30px;
  background:
    linear-gradient(180deg, rgba(12, 24, 43, 0.34), rgba(16, 34, 60, 0.22)),
    linear-gradient(146deg, rgba(255, 255, 255, 0.18), rgba(255, 255, 255, 0) 42%),
    radial-gradient(circle at 86% 14%, rgba(116, 214, 255, 0.16), transparent 24%);
  box-shadow:
    0 30px 60px rgba(7, 17, 34, 0.24),
    0 14px 28px rgba(76, 110, 154, 0.18),
    inset 0 1px 0 rgba(255, 255, 255, 0.58);
  backdrop-filter: blur(18px) saturate(1.15);
}

html.light .login-card::before,
html:not(.dark) .login-card::before {
  background:
    linear-gradient(145deg, rgba(255, 255, 255, 0.22), rgba(255, 255, 255, 0) 38%),
    linear-gradient(122deg, rgba(104, 212, 255, 0), rgba(104, 212, 255, 0.16) 58%, rgba(209, 168, 255, 0.1) 76%, rgba(209, 168, 255, 0) 90%);
}

html.light .login-card::after,
html:not(.dark) .login-card::after {
  inset: auto -10% -20% 34%;
  height: 190px;
  background: radial-gradient(circle, rgba(255, 218, 170, 0.28), rgba(92, 200, 255, 0.08) 42%, transparent 70%);
  filter: blur(20px);
}

html.light .login-card__caption,
html:not(.dark) .login-card__caption {
  color: rgba(213, 228, 247, 0.74);
}

html.light .login-card h1,
html:not(.dark) .login-card h1 {
  color: #f2f7ff;
  letter-spacing: -0.03em;
  text-shadow: 0 10px 28px rgba(6, 12, 28, 0.24);
}

html.light .login-label,
html:not(.dark) .login-label {
  color: rgba(208, 224, 245, 0.74);
}

html.light .login-input,
html:not(.dark) .login-input {
  border-color: rgba(122, 160, 204, 0.28);
  background:
    linear-gradient(180deg, rgba(9, 18, 35, 0.46), rgba(18, 34, 57, 0.3)),
    linear-gradient(140deg, rgba(255, 255, 255, 0.12), rgba(255, 255, 255, 0) 50%);
  color: #eef5ff;
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.22),
    0 10px 18px rgba(6, 17, 34, 0.16);
}

html.light .login-input::placeholder,
html:not(.dark) .login-input::placeholder {
  color: rgba(199, 214, 233, 0.5);
}

html.light .login-input:focus,
html:not(.dark) .login-input:focus {
  border-color: rgba(111, 210, 255, 0.5);
  background:
    linear-gradient(180deg, rgba(11, 24, 44, 0.56), rgba(18, 38, 66, 0.42)),
    linear-gradient(140deg, rgba(255, 255, 255, 0.16), rgba(255, 255, 255, 0) 48%);
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.28),
    0 0 0 4px rgba(103, 204, 255, 0.12),
    0 16px 30px rgba(10, 23, 44, 0.24);
}

html.light .login-button,
html:not(.dark) .login-button {
  background: linear-gradient(135deg, #234a82 0%, #3f76bd 34%, #63d9ff 100%);
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.28),
    0 18px 34px rgba(34, 82, 140, 0.34),
    0 0 24px rgba(99, 217, 255, 0.14);
}

html.light .login-button:hover,
html:not(.dark) .login-button:hover {
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.32),
    0 22px 40px rgba(34, 82, 140, 0.42),
    0 0 28px rgba(99, 217, 255, 0.18);
  filter: saturate(1.08) brightness(1.02);
}
</style>
