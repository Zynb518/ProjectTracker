<script setup lang="ts">
import { computed, onMounted, reactive, ref } from 'vue'

import { getErrorMessage } from '@/api/http'
import { createUser, getUserDetail, listUsers, updateUser, updateUserStatus } from '@/api/users'
import UserFormDialog from '@/components/users/UserFormDialog.vue'
import { useNotificationStore } from '@/stores/notifications'
import type {
  CreateUserPayload,
  UpdateUserPayload,
  UserListItem,
} from '@/types/user'
import {
  formatDisplayDateTime,
  getAccountStatusLabel,
  getAccountStatusTone,
  getSystemRoleLabel,
  getSystemRoleTone,
} from '@/utils/display'
import { getTotalPages, getVisiblePages } from '@/utils/pagination'

type UserFormValue = {
  username: string
  password: string
  real_name: string
  system_role: number
  email: string
  phone: string
}

const notificationStore = useNotificationStore()

const filters = reactive({
  keyword: '',
  systemRole: '',
  status: '',
})

const users = ref<UserListItem[]>([])
const isLoading = ref(false)
const hasLoadError = ref(false)
const dialogOpen = ref(false)
const dialogMode = ref<'create' | 'edit'>('create')
const editingUserId = ref<number | null>(null)
const formValue = ref<UserFormValue>(defaultFormValue())
const pagination = reactive({
  page: 1,
  pageSize: 10,
  total: 0,
})

const totalPages = computed(() => getTotalPages(pagination.total, pagination.pageSize))
const visiblePages = computed(() => getVisiblePages(pagination.page, totalPages.value))

const enabledCount = computed(() => users.value.filter((user) => user.status === 1).length)

function defaultFormValue(): UserFormValue {
  return {
    username: '',
    password: '',
    real_name: '',
    system_role: 3,
    email: '',
    phone: '',
  }
}

function buildQuery(page = pagination.page) {
  const keyword = filters.keyword.trim()

  return {
    keyword: keyword || undefined,
    system_role: filters.systemRole ? Number(filters.systemRole) : undefined,
    status: filters.status ? Number(filters.status) : undefined,
    page,
    page_size: pagination.pageSize,
  }
}

async function loadUsers(page = pagination.page) {
  isLoading.value = true
  hasLoadError.value = false

  try {
    const response = await listUsers(buildQuery(page))
    users.value = response.list
    pagination.total = response.total
    pagination.page = response.page
    pagination.pageSize = response.page_size
  } catch (error) {
    hasLoadError.value = true
    notificationStore.notifyError(getErrorMessage(error, '用户列表加载失败'))
  } finally {
    isLoading.value = false
  }
}

async function applyFilters() {
  pagination.page = 1
  await loadUsers(1)
}

async function changePage(page: number) {
  if (page === pagination.page || page < 1 || page > totalPages.value) {
    return
  }

  await loadUsers(page)
}

function openCreateDialog() {
  dialogMode.value = 'create'
  editingUserId.value = null
  formValue.value = defaultFormValue()
  dialogOpen.value = true
}

async function openEditDialog(userId: number) {
  dialogMode.value = 'edit'

  try {
    const detail = await getUserDetail(userId)
    editingUserId.value = userId
    formValue.value = {
      username: detail.username,
      password: '',
      real_name: detail.real_name,
      system_role: detail.system_role,
      email: detail.email,
      phone: detail.phone,
    }
    dialogOpen.value = true
  } catch (error) {
    notificationStore.notifyError(getErrorMessage(error, '用户详情加载失败'))
  }
}

function normalizeUserMutationPayload(payload: UserFormValue): CreateUserPayload | UpdateUserPayload {
  return {
    username: payload.username,
    password: payload.password,
    real_name: payload.real_name,
    system_role: payload.system_role,
    email: payload.email,
    phone: payload.phone,
  }
}

async function submitUser(payload: UserFormValue) {
  try {
    const normalizedPayload = normalizeUserMutationPayload(payload)

    if (dialogMode.value === 'create') {
      await createUser(normalizedPayload as CreateUserPayload)
    } else if (editingUserId.value !== null) {
      const updatePayload: UpdateUserPayload = {
        real_name: normalizedPayload.real_name,
        system_role: normalizedPayload.system_role,
        email: normalizedPayload.email,
        phone: normalizedPayload.phone,
      }

      await updateUser(editingUserId.value, updatePayload)
    }

    dialogOpen.value = false
    await loadUsers(pagination.page)
  } catch (error) {
    notificationStore.notifyError(getErrorMessage(error, '用户保存失败'))
  }
}

async function toggleUserStatus(user: UserListItem) {
  const nextStatus = (user.status === 1 ? 2 : 1) as 1 | 2

  try {
    await updateUserStatus(user.id, { status: nextStatus })
    await loadUsers(pagination.page)
  } catch (error) {
    notificationStore.notifyError(getErrorMessage(error, '用户状态更新失败'))
  }
}

function actionLabel(user: UserListItem) {
  return user.status === 1 ? `禁用用户 ${user.real_name}` : `启用用户 ${user.real_name}`
}

function handleRoleChange(value: string) {
  filters.systemRole = value
  void applyFilters()
}

function handleStatusChange(value: string) {
  filters.status = value
  void applyFilters()
}

onMounted(loadUsers)
</script>

<template>
  <section class="user-management">
    <section class="user-management__hero">
      <div class="user-management__hero-copy">
        <p class="user-management__eyebrow">Admin Control</p>
        <h2>用户管理</h2>
        <p class="user-management__description">
          统一查看系统账号、系统角色与启停状态，直接使用 API 设计中的用户管理接口完成维护。
        </p>
      </div>

      <div class="user-management__hero-stats">
        <article class="user-management__stat-card">
          <span>当前页账号</span>
          <strong>{{ users.length }}</strong>
        </article>
        <article class="user-management__stat-card">
          <span>启用中</span>
          <strong>{{ enabledCount }}</strong>
        </article>
        <article class="user-management__stat-card">
          <span>总账号数</span>
          <strong>{{ pagination.total }}</strong>
        </article>
      </div>

      <div class="user-management__filters">
        <label class="user-management__field user-management__field--search">
          <span>搜索用户</span>
          <input
            :value="filters.keyword"
            aria-label="搜索用户"
            placeholder="用户名 / 真实姓名"
            type="search"
            @input="filters.keyword = ($event.target as HTMLInputElement).value"
            @keydown.enter.prevent="applyFilters"
          />
        </label>

        <label class="user-management__field">
          <span>角色筛选</span>
          <select
            :value="filters.systemRole"
            data-testid="user-management-role"
            @change="handleRoleChange(($event.target as HTMLSelectElement).value)"
          >
            <option value="">全部角色</option>
            <option value="1">管理员</option>
            <option value="2">项目经理</option>
            <option value="3">普通员工</option>
          </select>
        </label>

        <label class="user-management__field">
          <span>账号状态</span>
          <select
            :value="filters.status"
            data-testid="user-management-status"
            @change="handleStatusChange(($event.target as HTMLSelectElement).value)"
          >
            <option value="">全部状态</option>
            <option value="1">启用</option>
            <option value="2">禁用</option>
          </select>
        </label>

        <button class="user-management__create" type="button" @click="openCreateDialog">
          <span aria-hidden="true">+</span>
          <span>新建用户</span>
        </button>
      </div>
    </section>

    <p v-if="isLoading && users.length === 0" class="user-management__state loading-panel">
      用户列表加载中...
    </p>
    <p v-else-if="!isLoading && !hasLoadError && users.length === 0" class="user-management__state">
      当前没有匹配的用户。
    </p>

    <p
      v-if="isLoading && users.length > 0"
      aria-live="polite"
      class="user-management__refreshing"
    >
      用户列表刷新中...
    </p>

    <section
      v-if="users.length > 0"
      v-smooth-wheel="{ axis: 'horizontal' }"
      class="user-management__table-card smooth-scroll-surface"
    >
      <table class="user-management__table">
        <thead>
          <tr>
            <th>用户</th>
            <th>系统角色</th>
            <th>联系方式</th>
            <th>状态</th>
            <th>更新时间</th>
            <th>操作</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="user in users" :key="user.id">
            <td>
              <div class="user-management__identity">
                <strong>{{ user.real_name }}</strong>
                <span>@{{ user.username }}</span>
              </div>
            </td>

            <td>
              <span
                :class="['user-management__pill', `user-management__pill--${getSystemRoleTone(user.system_role)}`]"
              >
                {{ getSystemRoleLabel(user.system_role) }}
              </span>
            </td>

            <td>
              <div class="user-management__contact">
                <span>{{ user.email || '未填写邮箱' }}</span>
                <span>{{ user.phone || '未填写手机号' }}</span>
              </div>
            </td>

            <td>
              <span
                :class="['user-management__pill', `user-management__pill--${getAccountStatusTone(user.status)}`]"
              >
                {{ getAccountStatusLabel(user.status) }}
              </span>
            </td>

            <td class="user-management__timestamp">
              {{ formatDisplayDateTime(user.updated_at) }}
            </td>

            <td>
              <div class="user-management__actions">
                <button
                  :aria-label="`编辑用户 ${user.real_name}`"
                  :title="`编辑用户 ${user.real_name}`"
                  class="user-management__icon-button"
                  type="button"
                  @click="openEditDialog(user.id)"
                >
                  <svg viewBox="0 0 24 24">
                    <path
                      d="M4 20h4.5L19 9.5 14.5 5 4 15.5V20Z M12.5 7l4.5 4.5"
                      fill="none"
                      stroke="currentColor"
                      stroke-linecap="round"
                      stroke-linejoin="round"
                      stroke-width="1.6"
                    />
                  </svg>
                </button>

                <button
                  :aria-label="actionLabel(user)"
                  :title="actionLabel(user)"
                  class="user-management__icon-button"
                  type="button"
                  @click="toggleUserStatus(user)"
                >
                  <svg v-if="user.status === 1" viewBox="0 0 24 24">
                    <path
                      d="M9.5 9.5 14.5 14.5 M14.5 9.5 9.5 14.5 M21 12A9 9 0 1 1 3 12a9 9 0 0 1 18 0Z"
                      fill="none"
                      stroke="currentColor"
                      stroke-linecap="round"
                      stroke-linejoin="round"
                      stroke-width="1.6"
                    />
                  </svg>
                  <svg v-else viewBox="0 0 24 24">
                    <path
                      d="m8.5 12 2.5 2.5 4.5-5 M21 12A9 9 0 1 1 3 12a9 9 0 0 1 18 0Z"
                      fill="none"
                      stroke="currentColor"
                      stroke-linecap="round"
                      stroke-linejoin="round"
                      stroke-width="1.6"
                    />
                  </svg>
                </button>
              </div>
            </td>
          </tr>
        </tbody>
      </table>
    </section>

    <section v-if="pagination.total > 0" class="user-management__pagination">
      <p class="user-management__pagination-summary">共 {{ pagination.total }} 个账号</p>

      <div class="user-management__pagination-controls">
        <button
          :disabled="isLoading || pagination.page === 1"
          type="button"
          @click="changePage(1)"
        >
          首页
        </button>

        <button
          :disabled="isLoading || pagination.page === 1"
          type="button"
          @click="changePage(pagination.page - 1)"
        >
          上一页
        </button>

        <button
          v-for="page in visiblePages"
          :key="page"
          :class="{ 'is-active': page === pagination.page }"
          :disabled="isLoading"
          type="button"
          @click="changePage(page)"
        >
          {{ page }}
        </button>

        <button
          :disabled="isLoading || pagination.page === totalPages"
          type="button"
          @click="changePage(pagination.page + 1)"
        >
          下一页
        </button>

        <button
          :disabled="isLoading || pagination.page === totalPages"
          type="button"
          @click="changePage(totalPages)"
        >
          尾页
        </button>
      </div>
    </section>

    <UserFormDialog
      v-model="dialogOpen"
      :initial-value="formValue"
      :mode="dialogMode"
      @submit="submitUser"
    />
  </section>
</template>

<style scoped>
.user-management {
  display: grid;
  gap: 18px;
  padding: 0 8px 8px;
}

.user-management__hero,
.user-management__table-card,
.user-management__pagination,
.user-management__state,
.user-management__refreshing {
  border: 1px solid var(--border-soft);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
}

.user-management__hero {
  position: relative;
  overflow: hidden;
  display: grid;
  grid-template-columns: minmax(0, 1.2fr) minmax(240px, 0.8fr);
  gap: 18px;
  padding: 24px 26px;
  border-radius: 24px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--glass-bg) 94%, transparent), var(--glass-bg)),
    radial-gradient(circle at top right, rgba(0, 194, 255, 0.16), transparent 26%);
}

.user-management__hero::before {
  content: '';
  position: absolute;
  inset: 0;
  background:
    linear-gradient(var(--grid-line) 1px, transparent 1px),
    linear-gradient(90deg, var(--grid-line) 1px, transparent 1px);
  background-size: 24px 24px;
  opacity: 0.2;
  pointer-events: none;
}

.user-management__hero-copy,
.user-management__hero-stats,
.user-management__filters {
  position: relative;
  z-index: 1;
}

.user-management__eyebrow,
.user-management__description {
  margin: 0;
}

.user-management__eyebrow {
  margin-bottom: 10px;
  font-size: 0.8rem;
  letter-spacing: 0.18em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.user-management__hero h2 {
  font-size: 2rem;
}

.user-management__description {
  max-width: 520px;
  margin-top: 12px;
  color: var(--text-soft);
}

.user-management__hero-stats {
  display: grid;
  grid-template-columns: repeat(3, minmax(0, 1fr));
  gap: 12px;
}

.user-management__stat-card {
  display: grid;
  gap: 6px;
  padding: 16px 18px;
  border: 1px solid var(--border-soft);
  border-radius: 18px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--panel-bg) 88%, transparent), color-mix(in srgb, var(--panel-bg) 74%, transparent)),
    radial-gradient(circle at 100% 0%, rgba(0, 194, 255, 0.14), transparent 64%);
}

.user-management__stat-card span {
  color: var(--text-soft);
  font-size: 0.86rem;
}

.user-management__stat-card strong {
  font-size: 1.55rem;
}

.user-management__filters {
  grid-column: 1 / -1;
  display: grid;
  grid-template-columns: minmax(0, 1.4fr) repeat(2, minmax(180px, 0.8fr)) auto;
  gap: 14px;
  align-items: end;
}

.user-management__field {
  display: grid;
  gap: 8px;
}

.user-management__field span {
  color: var(--text-soft);
  font-size: 0.84rem;
  font-weight: 600;
}

.user-management__field input,
.user-management__field select {
  width: 100%;
  border: 1px solid var(--border-soft);
  border-radius: 14px;
  padding: 12px 14px;
  color: var(--text-main);
  background: color-mix(in srgb, var(--panel-bg) 86%, transparent);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.05);
}

.user-management__create {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 10px;
  min-height: 48px;
  padding: 0 18px;
  border: 0;
  border-radius: 14px;
  color: var(--text-inverse);
  background: var(--gradient-primary);
  box-shadow: 0 12px 28px rgba(10, 102, 255, 0.22);
  cursor: pointer;
}

.user-management__create span:first-child {
  font-size: 1.25rem;
  line-height: 1;
}

.user-management__state,
.user-management__refreshing {
  margin: 0;
  padding: 16px 18px;
  border-radius: 18px;
  color: var(--text-soft);
  background: var(--glass-bg);
}

.user-management__table-card {
  overflow: hidden;
  border-radius: 22px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--panel-bg) 90%, transparent), color-mix(in srgb, var(--panel-bg) 76%, transparent)),
    radial-gradient(circle at top right, rgba(10, 102, 255, 0.12), transparent 24%);
}

.user-management__table {
  width: 100%;
  border-collapse: collapse;
}

.user-management__table th,
.user-management__table td {
  padding: 16px 18px;
  text-align: left;
  border-bottom: 1px solid var(--border-soft);
}

.user-management__table thead th {
  font-size: 0.82rem;
  letter-spacing: 0.12em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.user-management__table tbody tr:hover {
  background: color-mix(in srgb, var(--accent-start) 6%, transparent);
}

.user-management__identity,
.user-management__contact {
  display: grid;
  gap: 4px;
}

.user-management__identity strong {
  font-size: 1rem;
}

.user-management__identity span,
.user-management__contact span,
.user-management__timestamp {
  color: var(--text-soft);
}

.user-management__pill {
  display: inline-flex;
  align-items: center;
  min-height: 32px;
  padding: 0 12px;
  border-radius: 999px;
  border: 1px solid transparent;
  font-size: 0.84rem;
  font-weight: 700;
}

.user-management__pill--admin {
  color: var(--accent-start);
  border-color: color-mix(in srgb, var(--accent-start) 22%, transparent);
  background: color-mix(in srgb, var(--accent-start) 10%, transparent);
}

.user-management__pill--manager {
  color: var(--accent-warning);
  border-color: color-mix(in srgb, var(--accent-warning) 22%, transparent);
  background: color-mix(in srgb, var(--accent-warning) 10%, transparent);
}

.user-management__pill--member {
  color: var(--text-soft);
  border-color: var(--border-soft);
  background: color-mix(in srgb, var(--panel-bg) 62%, transparent);
}

.user-management__pill--enabled {
  color: var(--accent-success);
  border-color: color-mix(in srgb, var(--accent-success) 24%, transparent);
  background: color-mix(in srgb, var(--accent-success) 10%, transparent);
}

.user-management__pill--disabled {
  color: var(--accent-danger);
  border-color: color-mix(in srgb, var(--accent-danger) 24%, transparent);
  background: color-mix(in srgb, var(--accent-danger) 10%, transparent);
}

.user-management__actions {
  display: inline-flex;
  align-items: center;
  gap: 10px;
}

.user-management__icon-button {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 36px;
  height: 36px;
  border: 1px solid var(--border-soft);
  border-radius: 12px;
  color: var(--text-main);
  background: color-mix(in srgb, var(--panel-bg) 82%, transparent);
  cursor: pointer;
}

.user-management__icon-button:hover {
  transform: translateY(-2px);
  border-color: var(--accent-line);
  box-shadow: var(--shadow-panel-hover);
}

.user-management__icon-button svg {
  width: 18px;
  height: 18px;
}

.user-management__pagination {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 18px;
  padding: 16px 18px;
  border-radius: 18px;
  background: color-mix(in srgb, var(--glass-bg) 90%, transparent);
}

.user-management__pagination-summary {
  margin: 0;
  color: var(--text-soft);
}

.user-management__pagination-controls {
  display: inline-flex;
  flex-wrap: wrap;
  justify-content: flex-end;
  gap: 10px;
}

.user-management__pagination-controls button {
  min-width: 42px;
  padding: 10px 14px;
  border: 1px solid var(--border-soft);
  border-radius: 12px;
  color: var(--text-main);
  background: color-mix(in srgb, var(--panel-bg) 78%, transparent);
  cursor: pointer;
}

.user-management__pagination-controls button.is-active {
  color: var(--text-inverse);
  border-color: transparent;
  background: var(--gradient-primary);
}

.user-management__pagination-controls button:disabled {
  opacity: 0.52;
  cursor: not-allowed;
}

@media (max-width: 1080px) {
  .user-management__hero {
    grid-template-columns: 1fr;
  }

  .user-management__hero-stats {
    grid-template-columns: repeat(3, minmax(0, 1fr));
  }

  .user-management__filters {
    grid-template-columns: 1fr 1fr;
  }
}

@media (max-width: 900px) {
  .user-management__table-card {
    overflow-x: auto;
  }
}

@media (max-width: 680px) {
  .user-management__hero-stats,
  .user-management__filters {
    grid-template-columns: 1fr;
  }

  .user-management__pagination {
    align-items: stretch;
    flex-direction: column;
  }

  .user-management__pagination-controls {
    justify-content: flex-start;
  }
}
</style>
