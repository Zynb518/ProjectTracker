<script setup lang="ts">
import { computed, onMounted, reactive, ref } from 'vue'

import { getErrorMessage } from '@/api/http'
import { deviceApi } from '@/api/device'
import { useNotificationStore } from '@/stores/notifications'
import type { Device, DeviceUsageLog } from '@/types/device'
import { formatDisplayDateTime } from '@/utils/display'
import { getTotalPages, getVisiblePages } from '@/utils/pagination'

const notificationStore = useNotificationStore()

const devices = ref<Device[]>([])
const isLoading = ref(false)
const hasLoadError = ref(false)

const pagination = reactive({
  page: 1,
  pageSize: 9,
  total: 0,
})

const totalPages = computed(() => getTotalPages(pagination.total, pagination.pageSize))
const visiblePages = computed(() => getVisiblePages(pagination.page, totalPages.value))

// 弹窗状态
const createDialogOpen = ref(false)
const createForm = reactive({ name: '', description: '' })

const logDialogOpen = ref(false)
const currentDeviceId = ref<number | null>(null)
const logForm = reactive({ purpose: '' })

const historyDialogOpen = ref(false)
const currentDeviceName = ref('')
const deviceLogs = ref<DeviceUsageLog[]>([])
const logPagination = reactive({ page: 1, pageSize: 10, total: 0 })
const isLogsLoading = ref(false)

async function loadDevices(page = pagination.page) {
  isLoading.value = true
  hasLoadError.value = false

  try {
    const response = await deviceApi.getDevices(page, pagination.pageSize)
    devices.value = response.list
    pagination.total = response.total
    pagination.page = response.page
    pagination.pageSize = response.page_size
  } catch (error) {
    hasLoadError.value = true
    notificationStore.notifyError(getErrorMessage(error, '设备列表加载失败'))
  } finally {
    isLoading.value = false
  }
}

async function changePage(page: number) {
  if (page === pagination.page || page < 1 || page > totalPages.value) {
    return
  }
  await loadDevices(page)
}

function openCreateDialog() {
  createForm.name = ''
  createForm.description = ''
  createDialogOpen.value = true
}

async function submitCreateDevice() {
  if (!createForm.name.trim()) {
    notificationStore.notifyError('设备名称不能为空')
    return
  }
  try {
    await deviceApi.createDevice({ name: createForm.name, description: createForm.description })
    createDialogOpen.value = false
    await loadDevices(1)
  } catch (error) {
    notificationStore.notifyError(getErrorMessage(error, '新建设备失败'))
  }
}

async function deleteDeviceAction(id: number) {
  if (!confirm('确定要删除该设备吗？删除后相关使用记录将一并删除。')) return
  try {
    await deviceApi.deleteDevice(id)
    await loadDevices(pagination.page)
  } catch (error) {
    notificationStore.notifyError(getErrorMessage(error, '删除设备失败'))
  }
}

function openLogDialog(id: number) {
  currentDeviceId.value = id
  logForm.purpose = ''
  logDialogOpen.value = true
}

async function submitDeviceLog() {
  if (!currentDeviceId.value || !logForm.purpose.trim()) {
    notificationStore.notifyError('使用目的不能为空')
    return
  }
  try {
    await deviceApi.createDeviceLog(currentDeviceId.value, { purpose: logForm.purpose })
    logDialogOpen.value = false
    alert('登记成功')
  } catch (error) {
    notificationStore.notifyError(getErrorMessage(error, '登记失败'))
  }
}

async function loadDeviceLogs(deviceId: number, page = 1) {
  isLogsLoading.value = true
  try {
    const response = await deviceApi.getDeviceLogs(deviceId, page, logPagination.pageSize)
    deviceLogs.value = response.list
    logPagination.total = response.total
    logPagination.page = response.page
  } catch (error) {
    notificationStore.notifyError(getErrorMessage(error, '加载设备记录失败'))
  } finally {
    isLogsLoading.value = false
  }
}

function openHistoryDialog(device: Device) {
  currentDeviceId.value = device.id
  currentDeviceName.value = device.name
  logPagination.page = 1
  historyDialogOpen.value = true
  void loadDeviceLogs(device.id, 1)
}

async function changeLogPage(page: number) {
  const totalLogsPages = getTotalPages(logPagination.total, logPagination.pageSize)
  if (!currentDeviceId.value || page === logPagination.page || page < 1 || page > totalLogsPages) return
  await loadDeviceLogs(currentDeviceId.value, page)
}

onMounted(loadDevices)
</script>

<template>
  <section class="device-management">
    <section class="device-management__hero">
      <div class="device-management__hero-copy">
        <p class="device-management__eyebrow">Device Management</p>
        <h2>设备列表</h2>
        <p class="device-management__description">
          管理团队的公用设备与使用记录流水。
        </p>
      </div>

      <div class="device-management__hero-actions">
        <button class="device-management__create" type="button" @click="openCreateDialog">
          <span>+</span>
          <span>新增设备</span>
        </button>
      </div>
    </section>

    <div v-if="isLoading" class="device-management__loading">
      <div class="spinner" />
      <p>正在加载设备数据...</p>
    </div>

    <div v-else-if="hasLoadError" class="device-management__error">
      <p>设备列表加载失败，请检查网络或后端服务</p>
      <button class="button" @click="loadDevices(pagination.page)">重试</button>
    </div>

    <div v-else-if="devices.length === 0" class="device-management__empty">
      <svg viewBox="0 0 24 24" width="48" height="48" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
        <rect x="3" y="3" width="18" height="18" rx="2" ry="2"></rect>
        <line x1="9" y1="3" x2="9" y2="21"></line>
      </svg>
      <p>暂无设备</p>
    </div>

    <div v-else class="device-management__grid">
      <article v-for="device in devices" :key="device.id" class="device-card">
        <div class="device-card__header">
          <h3 class="device-card__title">{{ device.name }}</h3>
          <p class="device-card__date">创建于 {{ formatDisplayDateTime(device.created_at) }}</p>
        </div>
        <div class="device-card__body">
          <p class="device-card__desc">{{ device.description || '暂无描述' }}</p>
        </div>
        <div class="device-card__actions">
          <button class="device-card__action" type="button" @click="openLogDialog(device.id)">使用登记</button>
          <button class="device-card__action" type="button" @click="openHistoryDialog(device)">查看记录</button>
          <button class="device-card__action device-card__delete" type="button" @click="deleteDeviceAction(device.id)">删除</button>
        </div>
      </article>
    </div>

    <nav v-if="totalPages > 1" class="pagination" aria-label="分页导航">
      <button class="pagination__arrow" :disabled="pagination.page <= 1" @click="changePage(pagination.page - 1)">上一页</button>
      <div class="pagination__pages">
        <button v-for="p in visiblePages" :key="p" class="pagination__page" :aria-current="p === pagination.page ? 'page' : undefined" @click="changePage(p)">
          {{ p }}
        </button>
      </div>
      <button class="pagination__arrow" :disabled="pagination.page >= totalPages" @click="changePage(pagination.page + 1)">下一页</button>
    </nav>

    <!-- 新增设备弹窗 -->
    <dialog v-if="createDialogOpen" class="modal" open>
      <div class="modal__backdrop" @click="createDialogOpen = false"></div>
      <div class="modal__content">
        <div class="modal__header">
          <h3>新增设备</h3>
          <button class="modal__close" @click="createDialogOpen = false">×</button>
        </div>
        <div class="modal__body">
          <div class="form-group">
            <label>设备名称</label>
            <input v-model="createForm.name" type="text" placeholder="输入设备名称" class="input" />
          </div>
          <div class="form-group">
            <label>设备描述</label>
            <textarea v-model="createForm.description" placeholder="输入设备描述（选填）" class="input textarea"></textarea>
          </div>
        </div>
        <div class="modal__footer">
          <button class="button" data-variant="outline" @click="createDialogOpen = false">取消</button>
          <button class="button" data-variant="primary" @click="submitCreateDevice">保存</button>
        </div>
      </div>
    </dialog>

    <!-- 登记使用弹窗 -->
    <dialog v-if="logDialogOpen" class="modal" open>
      <div class="modal__backdrop" @click="logDialogOpen = false"></div>
      <div class="modal__content">
        <div class="modal__header">
          <h3>设备使用登记</h3>
          <button class="modal__close" @click="logDialogOpen = false">×</button>
        </div>
        <div class="modal__body">
          <div class="form-group">
            <label>使用目的</label>
            <textarea v-model="logForm.purpose" placeholder="请简述您使用该设备的目的..." class="input textarea"></textarea>
          </div>
        </div>
        <div class="modal__footer">
          <button class="button" data-variant="outline" @click="logDialogOpen = false">取消</button>
          <button class="button" data-variant="primary" @click="submitDeviceLog">提交登记</button>
        </div>
      </div>
    </dialog>

    <!-- 查看历史记录弹窗 -->
    <dialog v-if="historyDialogOpen" class="modal modal--large" open>
      <div class="modal__backdrop" @click="historyDialogOpen = false"></div>
      <div class="modal__content">
        <div class="modal__header">
          <h3>[{{ currentDeviceName }}] 使用记录</h3>
          <button class="modal__close" @click="historyDialogOpen = false">×</button>
        </div>
        <div class="modal__body">
          <div v-if="isLogsLoading" class="modal__loading">加载中...</div>
          <div v-else-if="deviceLogs.length === 0" class="modal__empty">暂无使用记录</div>
          <table v-else class="table">
            <colgroup>
              <col style="width: 15%; min-width: 120px;" />
              <col style="width: 60%;" />
              <col style="width: 25%; min-width: 180px;" />
            </colgroup>
            <thead>
              <tr>
                <th style="white-space: nowrap;">操作人</th>
                <th>使用目的</th>
                <th style="white-space: nowrap;">登记时间</th>
              </tr>
            </thead>
            <tbody>
              <tr v-for="log in deviceLogs" :key="log.id">
                <td style="white-space: nowrap;">{{ log.operator_name || '未知' }}</td>
                <td>{{ log.purpose }}</td>
                <td style="white-space: nowrap;">{{ formatDisplayDateTime(log.created_at) }}</td>
              </tr>
            </tbody>
          </table>

          <nav v-if="getTotalPages(logPagination.total, logPagination.pageSize) > 1" class="pagination" style="margin-top: 1rem;">
            <button class="pagination__arrow" :disabled="logPagination.page <= 1" @click="changeLogPage(logPagination.page - 1)">上一页</button>
            <span style="padding: 0 1rem;">第 {{ logPagination.page }} 页 / 共 {{ getTotalPages(logPagination.total, logPagination.pageSize) }} 页</span>
            <button class="pagination__arrow" :disabled="logPagination.page >= getTotalPages(logPagination.total, logPagination.pageSize)" @click="changeLogPage(logPagination.page + 1)">下一页</button>
          </nav>
        </div>
      </div>
    </dialog>
  </section>
</template>

<style scoped>
.device-management {
  display: grid;
  gap: 18px;
  padding: 0 8px 8px;
}

.device-management__hero,
.device-management__loading,
.device-management__error,
.device-management__empty,
.pagination {
  border: 1px solid var(--border-soft);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
}

.device-management__hero {
  position: relative;
  overflow: hidden;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 18px;
  padding: 24px 26px;
  border-radius: 24px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--glass-bg) 94%, transparent), var(--glass-bg)),
    radial-gradient(circle at top right, rgba(0, 194, 255, 0.16), transparent 26%);
}

.device-management__hero::before {
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

.device-management__hero-copy,
.device-management__hero-actions {
  position: relative;
  z-index: 1;
}

.device-management__eyebrow {
  margin-bottom: 10px;
  font-size: 0.8rem;
  font-weight: 600;
  letter-spacing: 0.18em;
  text-transform: uppercase;
  color: var(--text-soft);
  margin-top: 0;
}

.device-management__hero-copy h2 {
  font-size: 2rem;
  margin: 0;
  color: var(--text-heading);
}

.device-management__description {
  max-width: 520px;
  margin-top: 12px;
  color: var(--text-soft);
}

.device-management__create {
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

.device-management__create span:first-child {
  font-size: 1.25rem;
  line-height: 1;
}

.device-management__create span:last-child {
  font-size: 0.94rem;
  font-weight: 600;
}

.device-management__create:hover {
  filter: brightness(1.1);
  transform: translateY(-1px);
}

.device-management__grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(320px, 1fr));
  gap: 24px;
}

.device-card {
  display: flex;
  flex-direction: column;
  padding: 24px;
  border: 1px solid var(--border-soft);
  border-radius: 20px;
  background: var(--panel-bg);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
  transition: all 0.2s cubic-bezier(0.4, 0, 0.2, 1);
}

.device-card:hover {
  transform: translateY(-4px);
  box-shadow: var(--shadow-panel-hover);
  border-color: var(--border-base);
}

.device-card__title {
  font-size: 1.125rem;
  font-weight: 600;
  color: var(--text-heading);
  margin: 0 0 4px;
}

.device-card__date {
  font-size: 0.75rem;
  color: var(--text-muted);
}

.device-card__body {
  margin: 16px 0;
  flex: 1;
}

.device-card__desc {
  font-size: 0.875rem;
  color: var(--text-body);
  line-height: 1.5;
  display: -webkit-box;
  -webkit-line-clamp: 3;
  -webkit-box-orient: vertical;
  overflow: hidden;
}

.device-card__actions {
  display: flex;
  gap: 8px;
  margin-top: auto;
  border-top: 1px solid color-mix(in srgb, var(--border-soft) 92%, transparent);
  padding-top: 16px;
}

.device-card__action {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  padding: 8px 12px;
  border: 1px solid var(--border-soft);
  border-radius: 10px;
  color: var(--text-main);
  background: color-mix(in srgb, var(--panel-bg) 82%, transparent);
  font-size: 0.84rem;
  font-weight: 500;
  cursor: pointer;
  transition: all 0.2s;
}

.device-card__action:hover {
  transform: translateY(-2px);
  border-color: var(--accent-line);
  box-shadow: var(--shadow-panel-hover);
}

.device-card__delete {
  margin-left: auto;
  color: var(--accent-danger);
  border-color: color-mix(in srgb, var(--accent-danger) 24%, transparent);
  background: color-mix(in srgb, var(--accent-danger) 10%, transparent);
}

.device-card__delete:hover {
  border-color: var(--accent-danger);
  background: color-mix(in srgb, var(--accent-danger) 15%, transparent);
}

/* Modals */
.modal {
  position: fixed;
  inset: 0;
  z-index: 100;
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 24px;
  border: none;
  background: transparent;
}

.modal__backdrop {
  position: fixed;
  inset: 0;
  background:
    radial-gradient(circle at 18% 20%, rgba(10, 102, 255, 0.12), transparent 24%),
    radial-gradient(circle at 84% 78%, rgba(0, 194, 255, 0.16), transparent 28%),
    var(--overlay-backdrop);
  backdrop-filter: blur(16px);
}

.modal__backdrop::before {
  content: '';
  position: absolute;
  inset: 0;
  background:
    linear-gradient(var(--overlay-grid-line) 1px, transparent 1px),
    linear-gradient(90deg, var(--overlay-grid-line) 1px, transparent 1px);
  background-size: 28px 28px;
  opacity: 0.18;
  pointer-events: none;
}

.modal__content {
  position: relative;
  background: var(--panel-bg);
  border: 1px solid var(--border-soft);
  border-radius: 24px;
  width: 90vw;
  max-width: 1200px;
  aspect-ratio: 16 / 9;
  box-shadow: 0 24px 48px rgba(0, 0, 0, 0.2);
  overflow: hidden;
  display: flex;
  flex-direction: column;
  backdrop-filter: var(--backdrop-blur);
}
.modal--large .modal__content {
  max-width: 1400px;
}

.modal__header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 24px 32px;
  border-bottom: 1px solid color-mix(in srgb, var(--border-soft) 92%, transparent);
}

.modal__header h3 {
  margin: 0;
  font-size: 1.25rem;
  font-weight: 700;
  color: var(--text-heading);
}

.modal__close {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 32px;
  height: 32px;
  border-radius: 50%;
  background: transparent;
  border: 1px solid transparent;
  font-size: 1.5rem;
  color: var(--text-muted);
  cursor: pointer;
  transition: all 0.2s;
}

.modal__close:hover {
  background: color-mix(in srgb, var(--panel-bg) 80%, transparent);
  border-color: var(--border-soft);
  color: var(--text-heading);
}

.modal__body {
  padding: 32px;
  flex: 1;
  overflow-y: auto;
}

.modal__footer {
  display: flex;
  justify-content: flex-end;
  gap: 16px;
  padding: 24px 32px;
  border-top: 1px solid color-mix(in srgb, var(--border-soft) 92%, transparent);
  background: color-mix(in srgb, var(--panel-bg) 95%, transparent);
}

.form-group {
  margin-bottom: 24px;
}
.form-group label {
  display: block;
  margin-bottom: 8px;
  font-size: 0.9rem;
  font-weight: 600;
  color: var(--text-heading);
}
.input {
  width: 100%;
  padding: 14px 16px;
  border: 1px solid var(--border-soft);
  border-radius: 12px;
  background: color-mix(in srgb, var(--panel-bg) 86%, transparent);
  color: var(--text-main);
  font-size: 0.95rem;
  transition: all 0.2s;
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.05);
}
.input:focus {
  outline: none;
  border-color: var(--brand-primary);
  box-shadow: 0 0 0 3px color-mix(in srgb, var(--brand-primary) 15%, transparent);
  background: var(--bg-surface);
}
.textarea {
  resize: vertical;
  min-height: 120px;
}

.modal__footer .button {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  min-height: 44px;
  padding: 0 28px;
  border-radius: 22px;
  font-size: 0.95rem;
  font-weight: 600;
  transition: all 0.2s cubic-bezier(0.4, 0, 0.2, 1);
  cursor: pointer;
}

.modal__footer .button[data-variant="outline"] {
  border: 1px solid var(--border-soft);
  background: color-mix(in srgb, var(--panel-bg) 88%, transparent);
  color: var(--text-main);
}

.modal__footer .button[data-variant="outline"]:hover {
  transform: translateY(-2px);
  border-color: var(--accent-line);
  box-shadow: var(--shadow-panel-hover);
}

.modal__footer .button[data-variant="primary"] {
  border: 0;
  color: var(--text-inverse);
  background: var(--gradient-primary);
  box-shadow: 0 8px 16px rgba(10, 102, 255, 0.16);
}

.modal__footer .button[data-variant="primary"]:hover {
  transform: translateY(-2px);
  filter: brightness(1.1);
  box-shadow: 0 12px 24px rgba(10, 102, 255, 0.24);
}

/* Table */
.table {
  width: 100%;
  border-collapse: collapse;
}
.table th, .table td {
  padding: 12px;
  text-align: left;
  border-bottom: 1px solid var(--border-soft);
}
.table th {
  font-size: 0.875rem;
  font-weight: 600;
  color: var(--text-muted);
}
.table td {
  font-size: 0.875rem;
  color: var(--text-body);
}

/* Pagination component copy */
.pagination {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 12px;
  margin-top: 16px;
  padding: 16px 18px;
  border-radius: 18px;
  background: color-mix(in srgb, var(--glass-bg) 90%, transparent);
}
.pagination__pages {
  display: flex;
  gap: 4px;
}
.pagination__page, .pagination__arrow {
  min-width: 40px;
  min-height: 40px;
  border: 1px solid var(--border-soft);
  border-radius: 10px;
  padding: 0 14px;
  background: color-mix(in srgb, var(--panel-bg) 88%, transparent);
  color: var(--text-main);
  cursor: pointer;
  transition: all 0.2s;
}
.pagination__page[aria-current="page"] {
  border-color: transparent;
  background: var(--gradient-primary);
  color: var(--text-inverse);
  box-shadow: 0 14px 24px rgba(10, 102, 255, 0.2);
}
.pagination__page:hover:not([aria-current="page"]), .pagination__arrow:hover:not(:disabled) {
  transform: translateY(-2px);
  border-color: var(--accent-line);
  box-shadow: var(--shadow-panel-hover);
}
.pagination__arrow:disabled {
  opacity: 0.48;
  cursor: not-allowed;
}

.device-management__loading, .device-management__error, .device-management__empty, .modal__loading, .modal__empty {
  text-align: center;
  padding: 48px;
  color: var(--text-muted);
  border-radius: 18px;
  background: var(--meta-surface-bg);
}
</style>
