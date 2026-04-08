import { computed, ref } from 'vue'
import { defineStore } from 'pinia'

import { BusinessError } from '@/api/http'
import { getCurrentUser, login as loginRequest, logout as logoutRequest } from '@/api/auth'
import type { LoginPayload, UserProfile } from '@/types/auth'

export const useAuthStore = defineStore('auth', () => {
  const currentUser = ref<UserProfile | null>(null)
  const isReady = ref(false)
  const isLoading = ref(false)

  const isAuthenticated = computed(() => currentUser.value !== null)

  async function initializeSession() {
    isLoading.value = true

    try {
      currentUser.value = await getCurrentUser()
    } catch (error) {
      if (!(error instanceof BusinessError) || error.code !== 40003) {
        throw error
      }

      currentUser.value = null
    } finally {
      isReady.value = true
      isLoading.value = false
    }
  }

  async function login(payload: LoginPayload) {
    isLoading.value = true

    try {
      currentUser.value = await loginRequest(payload)
      isReady.value = true
    } finally {
      isLoading.value = false
    }
  }

  async function logout() {
    isLoading.value = true

    try {
      await logoutRequest()
    } finally {
      currentUser.value = null
      isReady.value = true
      isLoading.value = false
    }
  }

  return {
    currentUser,
    isAuthenticated,
    isLoading,
    isReady,
    initializeSession,
    login,
    logout,
  }
})
