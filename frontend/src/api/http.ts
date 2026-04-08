import axios from 'axios'

import type { ApiResponse } from '@/types/api'

export class BusinessError extends Error {
  code: number

  constructor(code: number, message: string) {
    super(message)
    this.code = code
  }
}

export function getErrorMessage(error: unknown, fallback = '请求失败，请稍后重试'): string {
  if (error instanceof BusinessError) {
    return error.message
  }

  if (error instanceof Error && error.message) {
    return error.message
  }

  if (typeof error === 'string' && error.trim()) {
    return error
  }

  return fallback
}

export const http = axios.create({
  baseURL: '/',
  withCredentials: true,
  headers: {
    'Content-Type': 'application/json',
  },
})

function isApiResponse<T>(payload: unknown): payload is ApiResponse<T> {
  return (
    typeof payload === 'object' &&
    payload !== null &&
    'code' in payload &&
    typeof payload.code === 'number' &&
    'message' in payload &&
    typeof payload.message === 'string' &&
    'data' in payload
  )
}

function unwrapPayload<T>(payload: ApiResponse<T>): T {
  if (payload.code !== 0) {
    throw new BusinessError(payload.code, payload.message)
  }

  return payload.data
}

export async function unwrapResponse<T>(request: Promise<{ data: ApiResponse<T> }>): Promise<T> {
  try {
    const response = await request
    return unwrapPayload(response.data)
  } catch (error) {
    if (axios.isAxiosError(error) && isApiResponse<T>(error.response?.data)) {
      return unwrapPayload(error.response.data)
    }

    throw error
  }
}
