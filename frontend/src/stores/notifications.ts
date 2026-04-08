import { ref } from 'vue'
import { defineStore } from 'pinia'

export interface AppNotice {
  id: number
  kind: 'error'
  message: string
}

export const useNotificationStore = defineStore('notifications', () => {
  const notices = ref<AppNotice[]>([])
  let nextId = 0

  function dismiss(id: number) {
    notices.value = notices.value.filter((notice) => notice.id !== id)
  }

  function notifyError(message: string, duration = 3200) {
    const id = ++nextId

    notices.value = [
      {
        id,
        kind: 'error',
        message,
      },
    ]

    window.setTimeout(() => {
      dismiss(id)
    }, duration)
  }

  return {
    notices,
    dismiss,
    notifyError,
  }
})
