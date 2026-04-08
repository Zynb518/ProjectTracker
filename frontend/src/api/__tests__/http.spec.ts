import type { AxiosError } from 'axios'
import { describe, expect, it } from 'vitest'

import { BusinessError, unwrapResponse } from '@/api/http'

describe('unwrapResponse', () => {
  it('maps api error payloads from axios rejections into business errors', async () => {
    const request = Promise.reject({
      isAxiosError: true,
      response: {
        data: {
          code: 40003,
          message: '未登录或登录态失效',
          data: null,
        },
      },
    } satisfies Partial<AxiosError>)

    await expect(unwrapResponse(request)).rejects.toMatchObject<Partial<BusinessError>>({
      code: 40003,
      message: '未登录或登录态失效',
    })
  })
})
