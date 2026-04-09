<script setup lang="ts">
import { computed, reactive, watch } from 'vue'

type UserFormValue = {
  username: string
  password: string
  real_name: string
  system_role: number
  email: string
  phone: string
}

const props = defineProps<{
  modelValue: boolean
  mode: 'create' | 'edit'
  initialValue: UserFormValue
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  submit: [payload: UserFormValue]
}>()

const form = reactive<UserFormValue>({
  username: '',
  password: '',
  real_name: '',
  system_role: 3,
  email: '',
  phone: '',
})

watch(
  () => props.initialValue,
  (value) => {
    form.username = value.username
    form.password = value.password
    form.real_name = value.real_name
    form.system_role = value.system_role
    form.email = value.email
    form.phone = value.phone
  },
  { immediate: true, deep: true },
)

const heading = computed(() => (props.mode === 'create' ? '新建用户' : '编辑用户'))
const submitLabel = computed(() => (props.mode === 'create' ? '创建用户' : '保存修改'))

function close() {
  emit('update:modelValue', false)
}

function submit() {
  emit('submit', {
    username: form.username.trim(),
    password: form.password,
    real_name: form.real_name.trim(),
    system_role: Number(form.system_role),
    email: form.email.trim(),
    phone: form.phone.trim(),
  })
}
</script>

<template>
  <Transition appear name="user-dialog-fade">
    <div v-if="modelValue" class="user-dialog__backdrop">
      <div aria-hidden="true" class="user-dialog__glow" />

      <section class="user-dialog">
        <header class="user-dialog__header">
          <div>
            <p class="user-dialog__eyebrow">User Console</p>
            <h2>{{ heading }}</h2>
          </div>

          <button type="button" @click="close">关闭</button>
        </header>

        <div class="user-dialog__grid">
          <label>
            <span>登录用户名</span>
            <input
              v-model="form.username"
              :disabled="mode === 'edit'"
              autocomplete="off"
              type="text"
            />
          </label>

          <label v-if="mode === 'create'">
            <span>初始密码</span>
            <input v-model="form.password" autocomplete="new-password" type="password" />
          </label>

          <label>
            <span>真实姓名</span>
            <input v-model="form.real_name" autocomplete="off" type="text" />
          </label>

          <label>
            <span>系统角色</span>
            <select v-model.number="form.system_role">
              <option :value="1">管理员</option>
              <option :value="2">项目经理</option>
              <option :value="3">普通员工</option>
            </select>
          </label>

          <label>
            <span>邮箱</span>
            <input v-model="form.email" autocomplete="off" type="email" />
          </label>

          <label>
            <span>手机号</span>
            <input v-model="form.phone" autocomplete="off" type="tel" />
          </label>
        </div>

        <footer class="user-dialog__footer">
          <button type="button" @click="close">取消</button>
          <button class="primary" type="button" @click="submit">
            {{ submitLabel }}
          </button>
        </footer>
      </section>
    </div>
  </Transition>
</template>

<style scoped>
.user-dialog-fade-enter-active,
.user-dialog-fade-leave-active {
  transition:
    opacity 280ms ease-out,
    backdrop-filter 280ms ease-out;
}

.user-dialog-fade-enter-active .user-dialog,
.user-dialog-fade-leave-active .user-dialog {
  transition:
    opacity 320ms ease-out,
    transform 320ms cubic-bezier(0.16, 1, 0.3, 1),
    box-shadow 320ms ease-out;
}

.user-dialog-fade-enter-active .user-dialog__glow,
.user-dialog-fade-leave-active .user-dialog__glow {
  transition:
    opacity 340ms ease-out,
    transform 360ms cubic-bezier(0.16, 1, 0.3, 1);
}

.user-dialog-fade-enter-from,
.user-dialog-fade-leave-to {
  opacity: 0;
  backdrop-filter: blur(0px);
}

.user-dialog-fade-enter-from .user-dialog,
.user-dialog-fade-leave-to .user-dialog {
  opacity: 0;
  transform: translateY(18px) scale(0.96);
}

.user-dialog-fade-enter-from .user-dialog__glow,
.user-dialog-fade-leave-to .user-dialog__glow {
  opacity: 0;
  transform: translate(-50%, -50%) scale(0.78);
}

.user-dialog__backdrop {
  position: fixed;
  inset: 0;
  z-index: 40;
  display: grid;
  place-items: center;
  padding: 24px;
  background:
    radial-gradient(circle at 18% 20%, rgba(10, 102, 255, 0.12), transparent 24%),
    radial-gradient(circle at 84% 78%, rgba(0, 194, 255, 0.16), transparent 28%),
    rgba(10, 14, 23, 0.42);
  backdrop-filter: blur(16px);
}

.user-dialog__glow {
  position: absolute;
  left: 78%;
  top: 22%;
  width: min(48vw, 420px);
  height: min(48vw, 420px);
  border-radius: 999px;
  background: radial-gradient(circle, rgba(0, 194, 255, 0.24), transparent 68%);
  filter: blur(18px);
  transform: translate(-50%, -50%);
  pointer-events: none;
}

.user-dialog {
  position: relative;
  width: min(100%, 680px);
  display: grid;
  gap: 18px;
  padding: 24px;
  border: 1px solid var(--border-soft);
  border-radius: 22px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--glass-bg-strong) 94%, transparent), var(--glass-bg-strong)),
    radial-gradient(circle at top right, rgba(0, 194, 255, 0.12), transparent 34%);
  box-shadow: var(--shadow-glass);
  backdrop-filter: var(--backdrop-blur);
}

.user-dialog::before {
  content: '';
  position: absolute;
  inset: 0;
  border-radius: inherit;
  border: 1px solid rgba(255, 255, 255, 0.06);
  pointer-events: none;
}

.user-dialog__header,
.user-dialog__footer {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 16px;
}

.user-dialog__header h2,
.user-dialog__header p {
  margin: 0;
}

.user-dialog__eyebrow {
  margin-bottom: 8px;
  font-size: 0.78rem;
  letter-spacing: 0.16em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.user-dialog__header button,
.user-dialog__footer button {
  border: 1px solid var(--border-soft);
  border-radius: 12px;
  padding: 10px 14px;
  color: var(--text-main);
  background: color-mix(in srgb, var(--glass-bg) 88%, transparent);
  cursor: pointer;
}

.user-dialog__footer .primary {
  color: var(--text-inverse);
  border-color: transparent;
  background: var(--gradient-primary);
  box-shadow: 0 10px 22px rgba(10, 102, 255, 0.24);
}

.user-dialog__grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 16px;
}

.user-dialog__grid label {
  display: grid;
  gap: 8px;
  color: var(--text-soft);
  font-size: 0.94rem;
}

.user-dialog__grid label span {
  font-weight: 600;
  color: var(--text-main);
}

.user-dialog__grid input,
.user-dialog__grid select {
  width: 100%;
  border: 1px solid var(--border-soft);
  border-radius: 12px;
  padding: 12px 14px;
  color: var(--text-main);
  background: color-mix(in srgb, var(--panel-bg) 88%, transparent);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.04);
}

.user-dialog__grid input:disabled {
  opacity: 0.72;
  cursor: not-allowed;
}

@media (max-width: 720px) {
  .user-dialog__grid {
    grid-template-columns: 1fr;
  }

  .user-dialog__header,
  .user-dialog__footer {
    align-items: stretch;
    flex-direction: column;
  }

  .user-dialog__header button,
  .user-dialog__footer button {
    width: 100%;
  }
}
</style>
