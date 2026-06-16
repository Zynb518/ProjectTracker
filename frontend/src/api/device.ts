import { http, unwrapResponse } from './http'
import type {
  DeviceListResponse,
  DeviceUsageLogListResponse,
  CreateDeviceRequest,
  CreateDeviceUsageLogRequest
} from '@/types/device'

export const deviceApi = {
  // 获取设备列表
  getDevices(page = 1, pageSize = 10) {
    return unwrapResponse<DeviceListResponse>(http.get('/api/devices', {
      params: { page, page_size: pageSize }
    }))
  },

  // 创建设备
  createDevice(data: CreateDeviceRequest) {
    return unwrapResponse<{ id: number }>(http.post('/api/devices', data))
  },

  // 删除设备
  deleteDevice(id: number) {
    return unwrapResponse<null>(http.delete(`/api/devices/${id}`))
  },

  // 获取设备使用记录
  getDeviceLogs(deviceId: number, page = 1, pageSize = 10) {
    return unwrapResponse<DeviceUsageLogListResponse>(http.get(`/api/devices/${deviceId}/logs`, {
      params: { page, page_size: pageSize }
    }))
  },

  // 提交设备使用记录
  createDeviceLog(deviceId: number, data: CreateDeviceUsageLogRequest) {
    return unwrapResponse<{ id: number }>(http.post(`/api/devices/${deviceId}/logs`, data))
  }
}
