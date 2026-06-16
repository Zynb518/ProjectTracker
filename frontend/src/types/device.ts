export interface Device {
  id: number
  name: string
  description?: string
  created_at: string
  updated_at: string
}

export interface DeviceUsageLog {
  id: number
  device_id: number
  user_id: number
  operator_name: string
  purpose: string
  created_at: string
}

export interface DeviceListResponse {
  list: Device[]
  total: number
  page: number
  page_size: number
}

export interface DeviceUsageLogListResponse {
  list: DeviceUsageLog[]
  total: number
  page: number
  page_size: number
}

export interface CreateDeviceRequest {
  name: string
  description?: string
}

export interface CreateDeviceUsageLogRequest {
  purpose: string
}
