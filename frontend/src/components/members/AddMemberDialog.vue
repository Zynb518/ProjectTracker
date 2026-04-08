<script setup lang="ts">
import { ref, watch } from 'vue'

import { getErrorMessage } from '@/api/http'
import { listMemberCandidates } from '@/api/members'
import { useNotificationStore } from '@/stores/notifications'
import type { MemberCandidate } from '@/types/member'

const props = defineProps<{
  modelValue: boolean
  projectId: number
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
  select: [userId: number]
}>()

const keyword = ref('')
const candidates = ref<MemberCandidate[]>([])
const isLoading = ref(false)
const notificationStore = useNotificationStore()

async function loadCandidates() {
  isLoading.value = true

  try {
    const response = await listMemberCandidates(props.projectId, {
      keyword: keyword.value,
      page: 1,
      page_size: 20,
    })
    candidates.value = response.list
  } catch (error) {
    candidates.value = []
    notificationStore.notifyError(getErrorMessage(error, '候选成员加载失败'))
  } finally {
    isLoading.value = false
  }
}

watch(
  () => props.modelValue,
  (value) => {
    if (value) {
      void loadCandidates()
    }
  },
  { immediate: true },
)
</script>

<template>
  <div v-if="modelValue" class="member-dialog__backdrop">
    <section class="member-dialog">
      <header>
        <div>
          <p>成员候选</p>
          <h2>添加项目成员</h2>
        </div>
        <button type="button" @click="$emit('update:modelValue', false)">关闭</button>
      </header>

      <div class="member-dialog__search">
        <input v-model="keyword" placeholder="搜索用户名或姓名" type="text" />
        <button type="button" @click="loadCandidates">搜索</button>
      </div>

      <p v-if="isLoading" class="member-dialog__state">候选成员加载中...</p>
      <div v-else class="member-dialog__list">
        <button
          v-for="candidate in candidates"
          :key="candidate.id"
          type="button"
          @click="$emit('select', candidate.id)"
        >
          <strong>{{ candidate.real_name }}</strong>
          <span>{{ candidate.username }}</span>
        </button>
      </div>
    </section>
  </div>
</template>

<style scoped>
.member-dialog__backdrop {
  position: fixed;
  inset: 0;
  display: grid;
  place-items: center;
  padding: 24px;
  background: rgba(10, 14, 23, 0.38);
  backdrop-filter: blur(16px);
}

.member-dialog {
  width: min(100%, 560px);
  display: grid;
  gap: 16px;
  padding: 24px;
  border: 1px solid var(--border-soft);
  border-radius: 16px;
  background: var(--glass-bg-strong);
  box-shadow: var(--shadow-glass);
  backdrop-filter: var(--backdrop-blur);
}

header,
.member-dialog__search {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

header h2,
header p {
  margin: 0;
}

.member-dialog__search input {
  flex: 1;
}

input,
button {
  border-radius: 8px;
  padding: 10px 14px;
  font: inherit;
}

input {
  border: 1px solid var(--border-soft);
  background: color-mix(in srgb, var(--panel-bg) 92%, transparent);
  color: var(--text-main);
}

button {
  border: 1px solid var(--border-soft);
  background: color-mix(in srgb, var(--panel-bg) 88%, transparent);
  color: var(--text-main);
}

.member-dialog__list {
  display: grid;
  gap: 10px;
}

.member-dialog__list button {
  display: flex;
  align-items: center;
  justify-content: space-between;
  background: color-mix(in srgb, var(--panel-bg) 88%, transparent);
  cursor: pointer;
}

.member-dialog__state {
  margin: 0;
  color: var(--text-soft);
}
</style>
