<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { useRouter } from 'vue-router'

import ThemeToggle from '@/components/ThemeToggle.vue'
import cat3Image from '@/assets/login/cat3.png'
import skyImage from '@/assets/login/sky.png'
import { getErrorMessage } from '@/api/http'
import { useNotificationStore } from '@/stores/notifications'
import { useAuthStore } from '@/stores/auth'
import { useThemeStore } from '@/stores/theme'

const router = useRouter()
const authStore = useAuthStore()
const notificationStore = useNotificationStore()
const themeStore = useThemeStore()

const username = ref('')
const password = ref('')
const isDark = computed(() => themeStore.mode === 'dark')
const activeBackgroundImage = computed(() => `url(${isDark.value ? cat3Image : skyImage})`)

onMounted(() => {
  if (typeof Image === 'undefined') {
    return
  }

  for (const src of [skyImage, cat3Image]) {
    const image = new Image()
    image.decoding = 'async'
    image.src = src
  }
})

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
    <div
      aria-hidden="true"
      class="login-view__background"
      data-testid="login-background"
      :style="{ backgroundImage: activeBackgroundImage }"
    />
    <div
      aria-hidden="true"
      :class="['login-view__scrim', { 'login-view__scrim--dark': isDark }]"
    />

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
  padding: 32px 24px;
  position: relative;
  isolation: isolate;
}

.login-view__background,
.login-view__scrim {
  position: absolute;
  inset: 0;
  pointer-events: none;
  z-index: 0;
}

.login-view__background {
  background-position: center;
  background-repeat: no-repeat;
  background-size: cover;
  background-color: color-mix(in srgb, var(--panel-bg-soft) 68%, #ffffff 32%);
}

.login-view__scrim {
  background:
    radial-gradient(circle at 50% 18%, color-mix(in srgb, var(--accent-start) 8%, transparent), transparent 26%),
    linear-gradient(180deg, rgba(255, 255, 255, 0.04), rgba(228, 237, 247, 0.2));
}

.login-view__scrim--dark {
  background:
    radial-gradient(circle at 50% 18%, color-mix(in srgb, var(--accent-end) 12%, transparent), transparent 28%),
    linear-gradient(180deg, rgba(7, 10, 20, 0.1), rgba(7, 10, 20, 0.34));
}

.login-view__toolbar {
  width: min(100%, 420px);
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
  letter-spacing: 0.2em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.login-card {
  width: min(100%, 420px);
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
  transform: translateY(-2px);
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
