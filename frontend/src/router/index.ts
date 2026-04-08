import type { Pinia } from 'pinia'
import { createRouter, createWebHistory } from 'vue-router'

import AppShell from '@/layouts/AppShell.vue'
import { useAuthStore } from '@/stores/auth'
import { appPinia } from '@/stores/pinia'
import ProjectDetailView from '@/views/ProjectDetailView.vue'
import LoginView from '@/views/LoginView.vue'
import MyTasksView from '@/views/MyTasksView.vue'
import ProjectListView from '@/views/ProjectListView.vue'

export function createAppRouter(pinia: Pinia = appPinia) {
  const router = createRouter({
    history: createWebHistory(),
    routes: [
      {
        path: '/login',
        name: 'login',
        component: LoginView,
      },
      {
        path: '/',
        component: AppShell,
        meta: {
          requiresAuth: true,
        },
        children: [
          {
            path: '',
            redirect: '/projects',
          },
          {
            path: 'projects',
            name: 'projects',
            component: ProjectListView,
          },
          {
            path: 'projects/:projectId',
            name: 'project-detail',
            component: ProjectDetailView,
          },
          {
            path: 'my-tasks',
            name: 'my-tasks',
            component: MyTasksView,
          },
        ],
      },
    ],
  })

  router.beforeEach(async (to) => {
    const authStore = useAuthStore(pinia)

    if (!authStore.isReady) {
      try {
        await authStore.initializeSession()
      } catch (error) {
        if (to.path !== '/login') {
          throw error
        }
      }
    }

    if (to.meta.requiresAuth && !authStore.currentUser) {
      return '/login'
    }

    if (to.path === '/login' && authStore.currentUser) {
      return '/projects'
    }

    return true
  })

  return router
}

export default createAppRouter(appPinia)
