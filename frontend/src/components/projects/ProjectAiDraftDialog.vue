<script setup lang="ts">
import { computed, onBeforeUnmount, reactive, ref, watch } from 'vue'
import { useRouter } from 'vue-router'

import { getErrorMessage } from '@/api/http'
import { createProjectNode } from '@/api/nodes'
import { generateProjectDraft } from '@/api/projectAi'
import { createProject } from '@/api/projects'
import { createSubtask } from '@/api/subtasks'
import { useNotificationStore } from '@/stores/notifications'
import type { ProjectNodePayload } from '@/types/node'
import type { AiProjectDraft, AiProjectDraftNode, AiProjectDraftSubtask } from '@/types/projectAi'
import type { ProjectFormPayload } from '@/types/project'
import type { SubtaskPayload } from '@/types/subtask'

type DialogMotionOrigin = {
  x: number
  y: number
  translateX: number
  translateY: number
}

type EditableSubtask = Omit<AiProjectDraftSubtask, never> & {
  local_id: string
}

type EditableNode = Omit<AiProjectDraftNode, 'subtasks'> & {
  local_id: string
  subtasks: EditableSubtask[]
}

type EditableDraft = {
  project: ProjectFormPayload
  nodes: EditableNode[]
}

type SelectedEntity =
  | { kind: 'project' }
  | { kind: 'node'; nodeId: string }
  | { kind: 'subtask'; nodeId: string; subtaskId: string }

const props = defineProps<{
  modelValue: boolean
  motionOrigin?: DialogMotionOrigin | null
}>()

const emit = defineEmits<{
  'update:modelValue': [value: boolean]
}>()

const router = useRouter()
const notificationStore = useNotificationStore()

const prompt = ref('')
const draft = ref<EditableDraft | null>(null)
const selected = ref<SelectedEntity>({ kind: 'project' })
const isGenerating = ref(false)
const isSubmitting = ref(false)
const localError = ref<string | null>(null)
const submitState = reactive({
  current: 0,
  total: 0,
  label: '',
})

let latestGenerateRequestId = 0
let localIdSeed = 0
let htmlOverflow = ''
let bodyOverflow = ''

const backdropStyle = computed(() => {
  if (!props.motionOrigin) {
    return undefined
  }

  return {
    '--project-ai-origin-x': `${props.motionOrigin.x}px`,
    '--project-ai-origin-y': `${props.motionOrigin.y}px`,
    '--project-ai-translate-x': `${props.motionOrigin.translateX}px`,
    '--project-ai-translate-y': `${props.motionOrigin.translateY}px`,
  }
})

const selectedNode = computed(() => {
  const currentDraft = draft.value
  const currentSelected = selected.value

  if (currentDraft === null || currentSelected.kind === 'project') {
    return null
  }

  return currentDraft.nodes.find((node) => node.local_id === currentSelected.nodeId) ?? null
})

const selectedSubtask = computed(() => {
  const currentSelected = selected.value
  const currentNode = selectedNode.value

  if (currentSelected.kind !== 'subtask' || currentNode === null) {
    return null
  }

  return currentNode.subtasks.find((subtask) => subtask.local_id === currentSelected.subtaskId) ?? null
})

const submitPercent = computed(() => {
  if (submitState.total <= 0) {
    return 0
  }

  return Math.round((submitState.current / submitState.total) * 100)
})

function createLocalId(prefix: string) {
  localIdSeed += 1
  return `${prefix}-${localIdSeed}`
}

function createEditableSubtask(source?: Partial<AiProjectDraftSubtask>): EditableSubtask {
  return {
    local_id: createLocalId('subtask'),
    name: source?.name ?? '新的子任务',
    description: source?.description ?? '',
    priority: source?.priority ?? 2,
    planned_start_date: source?.planned_start_date ?? '',
    planned_end_date: source?.planned_end_date ?? '',
  }
}

function createEditableNode(source?: Partial<AiProjectDraftNode>): EditableNode {
  return {
    local_id: createLocalId('node'),
    name: source?.name ?? '新的阶段',
    description: source?.description ?? '',
    planned_start_date: source?.planned_start_date ?? '',
    planned_end_date: source?.planned_end_date ?? '',
    subtasks: (source?.subtasks ?? []).map((subtask) => createEditableSubtask(subtask)),
  }
}

function hydrateDraft(source: AiProjectDraft): EditableDraft {
  return {
    project: {
      name: source.project.name,
      description: source.project.description,
      planned_start_date: source.project.planned_start_date,
      planned_end_date: source.project.planned_end_date,
    },
    nodes: source.nodes.map((node) => createEditableNode(node)),
  }
}

function resetState() {
  prompt.value = ''
  draft.value = null
  selected.value = { kind: 'project' }
  isGenerating.value = false
  isSubmitting.value = false
  localError.value = null
  submitState.current = 0
  submitState.total = 0
  submitState.label = ''
}

function lockBodyScroll() {
  if (typeof document === 'undefined') {
    return
  }

  htmlOverflow = document.documentElement.style.overflow
  bodyOverflow = document.body.style.overflow
  document.documentElement.style.overflow = 'hidden'
  document.body.style.overflow = 'hidden'
}

function restoreBodyScroll() {
  if (typeof document === 'undefined') {
    return
  }

  document.documentElement.style.overflow = htmlOverflow
  document.body.style.overflow = bodyOverflow
}

function close() {
  if (isSubmitting.value) {
    return
  }

  emit('update:modelValue', false)
}

function isValidDateRange(startDate: string, endDate: string) {
  return startDate <= endDate
}

function validateDraft(currentDraft: EditableDraft) {
  if (!currentDraft.project.name.trim()) {
    return '项目名称不能为空'
  }

  if (!currentDraft.project.planned_start_date || !currentDraft.project.planned_end_date) {
    return '项目计划日期不能为空'
  }

  if (!isValidDateRange(currentDraft.project.planned_start_date, currentDraft.project.planned_end_date)) {
    return '项目开始日期不能晚于结束日期'
  }

  if (currentDraft.nodes.length === 0) {
    return '至少需要一个阶段'
  }

  for (const node of currentDraft.nodes) {
    if (!node.name.trim()) {
      return '阶段名称不能为空'
    }

    if (!node.planned_start_date || !node.planned_end_date) {
      return `阶段「${node.name || '未命名阶段'}」计划日期不能为空`
    }

    if (!isValidDateRange(node.planned_start_date, node.planned_end_date)) {
      return `阶段「${node.name}」开始日期不能晚于结束日期`
    }

    if (
      node.planned_start_date < currentDraft.project.planned_start_date
      || node.planned_end_date > currentDraft.project.planned_end_date
    ) {
      return `阶段「${node.name}」必须落在项目周期内`
    }

    for (const subtask of node.subtasks) {
      if (!subtask.name.trim()) {
        return `阶段「${node.name}」下存在未命名子任务`
      }

      if (!subtask.planned_start_date || !subtask.planned_end_date) {
        return `子任务「${subtask.name || '未命名子任务'}」计划日期不能为空`
      }

      if (!isValidDateRange(subtask.planned_start_date, subtask.planned_end_date)) {
        return `子任务「${subtask.name}」开始日期不能晚于结束日期`
      }

      if (
        subtask.planned_start_date < node.planned_start_date
        || subtask.planned_end_date > node.planned_end_date
      ) {
        return `子任务「${subtask.name}」必须落在所属阶段周期内`
      }
    }
  }

  return null
}

async function handleGenerateDraft() {
  if (isGenerating.value || isSubmitting.value) {
    return
  }

  const normalizedPrompt = prompt.value.trim()
  if (!normalizedPrompt) {
    localError.value = '请先输入项目需求提示词'
    return
  }

  if (draft.value !== null) {
    const confirmed = window.confirm('重新生成会覆盖当前全部草稿，确定继续吗？')
    if (!confirmed) {
      return
    }
  }

  const requestId = ++latestGenerateRequestId
  localError.value = null
  isGenerating.value = true

  try {
    const generatedDraft = await generateProjectDraft(normalizedPrompt)
    if (requestId !== latestGenerateRequestId) {
      return
    }

    draft.value = hydrateDraft(generatedDraft)
    selected.value = { kind: 'project' }
  } catch (error) {
    if (requestId !== latestGenerateRequestId) {
      return
    }

    localError.value = getErrorMessage(error, 'AI 草稿生成失败')
    notificationStore.notifyError(localError.value)
  } finally {
    if (requestId === latestGenerateRequestId) {
      isGenerating.value = false
    }
  }
}

function selectProject() {
  selected.value = { kind: 'project' }
}

function selectNode(nodeId: string) {
  selected.value = {
    kind: 'node',
    nodeId,
  }
}

function selectSubtask(nodeId: string, subtaskId: string) {
  selected.value = {
    kind: 'subtask',
    nodeId,
    subtaskId,
  }
}

function addNode() {
  if (draft.value === null) {
    return
  }

  const node = createEditableNode({
    planned_start_date: draft.value.project.planned_start_date,
    planned_end_date: draft.value.project.planned_end_date,
  })

  draft.value.nodes.push(node)
  selectNode(node.local_id)
}

function addSubtask(nodeId: string) {
  if (draft.value === null) {
    return
  }

  const node = draft.value.nodes.find((item) => item.local_id === nodeId)
  if (!node) {
    return
  }

  const subtask = createEditableSubtask({
    planned_start_date: node.planned_start_date,
    planned_end_date: node.planned_end_date,
  })

  node.subtasks.push(subtask)
  selectSubtask(node.local_id, subtask.local_id)
}

function removeSelectedNode() {
  const currentDraft = draft.value
  const currentSelected = selected.value

  if (currentDraft === null || currentSelected.kind !== 'node') {
    return
  }

  currentDraft.nodes = currentDraft.nodes.filter((node) => node.local_id !== currentSelected.nodeId)
  selectProject()
}

function removeSelectedSubtask() {
  const currentDraft = draft.value
  const currentSelected = selected.value

  if (currentDraft === null || currentSelected.kind !== 'subtask') {
    return
  }

  const node = currentDraft.nodes.find((item) => item.local_id === currentSelected.nodeId)
  if (!node) {
    return
  }

  const remainingSubtasks: EditableSubtask[] = node.subtasks.filter(
    (subtask): subtask is EditableSubtask => subtask.local_id !== currentSelected.subtaskId,
  )
  node.subtasks = remainingSubtasks
  selectNode(node.local_id)
}

function toNodePayload(node: EditableNode): ProjectNodePayload {
  return {
    name: node.name.trim(),
    description: node.description.trim(),
    planned_start_date: node.planned_start_date,
    planned_end_date: node.planned_end_date,
  }
}

function toSubtaskPayload(subtask: EditableSubtask): SubtaskPayload {
  return {
    name: subtask.name.trim(),
    description: subtask.description.trim(),
    priority: subtask.priority ?? 2,
    planned_start_date: subtask.planned_start_date,
    planned_end_date: subtask.planned_end_date,
  }
}

async function submitDraft() {
  if (draft.value === null || isGenerating.value || isSubmitting.value) {
    return
  }

  const validationMessage = validateDraft(draft.value)
  if (validationMessage) {
    localError.value = validationMessage
    return
  }

  localError.value = null
  isSubmitting.value = true
  submitState.current = 0
  submitState.total = 1 + draft.value.nodes.length + draft.value.nodes.reduce((total, node) => total + node.subtasks.length, 0)
  submitState.label = '创建项目...'

  let createdProjectId: number | null = null

  try {
    const createdProject = await createProject({
      ...draft.value.project,
      name: draft.value.project.name.trim(),
      description: draft.value.project.description.trim(),
    })
    createdProjectId = createdProject.id
    submitState.current += 1

    for (const node of draft.value.nodes) {
      submitState.label = `创建阶段：${node.name}`
      const createdNode = await createProjectNode(createdProjectId, toNodePayload(node))
      submitState.current += 1

      for (const subtask of node.subtasks) {
        submitState.label = `创建子任务：${subtask.name}`
        await createSubtask(createdProjectId, createdNode.id, toSubtaskPayload(subtask))
        submitState.current += 1
      }
    }

    emit('update:modelValue', false)
    await router.push(`/projects/${createdProjectId}`)
  } catch (error) {
    const message = getErrorMessage(error, 'AI 草稿提交失败')

    if (createdProjectId !== null) {
      emit('update:modelValue', false)
      notificationStore.notifyError(`${message}，项目已部分创建，正在进入详情页继续处理。`, 4600)
      await router.push(`/projects/${createdProjectId}`)
      return
    }

    localError.value = message
    notificationStore.notifyError(message)
  } finally {
    isSubmitting.value = false
  }
}

watch(
  () => props.modelValue,
  (value) => {
    if (value) {
      resetState()
      lockBodyScroll()
      return
    }

    restoreBodyScroll()
  },
  { immediate: true },
)

onBeforeUnmount(() => {
  restoreBodyScroll()
})
</script>

<template>
  <Transition appear name="project-ai-dialog">
    <div v-if="modelValue" :style="backdropStyle" class="project-ai-dialog__backdrop">
      <div aria-hidden="true" class="project-ai-dialog__glow" />

      <section class="project-ai-dialog">
        <header class="project-ai-dialog__header">
          <div>
            <h2>AI 创建项目</h2>
          </div>

          <button type="button" class="project-ai-dialog__close" @click="close">关闭</button>
        </header>

        <div class="project-ai-dialog__status">
          <div v-if="localError" class="project-ai-dialog__banner project-ai-dialog__banner--error">
            {{ localError }}
          </div>

          <div v-if="isSubmitting" class="project-ai-dialog__banner">
            <div class="project-ai-dialog__progress-head">
              <strong>{{ submitState.label }}</strong>
              <span>{{ submitState.current }} / {{ submitState.total }}</span>
            </div>
            <div class="project-ai-dialog__progress-track">
              <span :style="{ width: `${submitPercent}%` }" class="project-ai-dialog__progress-fill" />
            </div>
          </div>
        </div>

        <div class="project-ai-dialog__body">
          <section class="project-ai-dialog__panel project-ai-dialog__panel--prompt">
            <div class="project-ai-dialog__panel-head">
              <p class="project-ai-dialog__panel-label">提示词区</p>
              <h3>AI 生成面板</h3>
            </div>

            <div class="project-ai-dialog__panel-content project-ai-dialog__panel-content--prompt">
              <label class="project-ai-dialog__field project-ai-dialog__field--fill">
                <span>项目需求提示词</span>
                <textarea
                  v-model="prompt"
                  aria-label="AI 项目提示词"
                  class="project-ai-dialog__textarea"
                  placeholder="例如：做一个内部流程平台项目，包含需求梳理、开发联调、测试上线三个阶段。"
                  rows="10"
                ></textarea>
              </label>
            </div>

            <button
              type="button"
              class="project-ai-dialog__primary"
              :disabled="isGenerating || isSubmitting"
              @click="handleGenerateDraft"
            >
              {{ isGenerating ? 'AI 生成中...' : 'AI 生成草稿' }}
            </button>
          </section>

          <section class="project-ai-dialog__panel project-ai-dialog__panel--tree">
            <div class="project-ai-dialog__panel-head project-ai-dialog__panel-head--split">
              <div>
                <p class="project-ai-dialog__panel-label">结构树</p>
                <h3>项目草稿树</h3>
              </div>

              <button
                type="button"
                class="project-ai-dialog__ghost"
                :disabled="draft === null || isSubmitting"
                @click="addNode"
              >
                新增阶段
              </button>
            </div>

            <div class="project-ai-dialog__panel-content project-ai-dialog__panel-content--tree">
              <div v-if="draft" class="project-ai-dialog__tree">
                <button
                  type="button"
                  :class="['project-ai-dialog__tree-project', { 'is-selected': selected.kind === 'project' }]"
                  @click="selectProject"
                >
                  <strong>{{ draft.project.name || '未命名项目' }}</strong>
                  <span>{{ draft.project.planned_start_date || '未设置' }} ~ {{ draft.project.planned_end_date || '未设置' }}</span>
                </button>

                <div v-smooth-wheel class="project-ai-dialog__tree-list smooth-scroll-surface">
                  <article
                    v-for="node in draft.nodes"
                    :key="node.local_id"
                    :data-testid="`project-ai-draft-node-${node.local_id}`"
                    :class="[
                      'project-ai-dialog__tree-node',
                      {
                        'is-selected': selected.kind === 'node' && selected.nodeId === node.local_id,
                      },
                    ]"
                  >
                    <button type="button" class="project-ai-dialog__tree-node-button" @click="selectNode(node.local_id)">
                      <strong>{{ node.name || '未命名阶段' }}</strong>
                      <span>{{ node.planned_start_date || '未设置' }} ~ {{ node.planned_end_date || '未设置' }}</span>
                    </button>

                    <div class="project-ai-dialog__tree-subtasks">
                      <button
                        v-for="subtask in node.subtasks"
                        :key="subtask.local_id"
                        type="button"
                        :class="[
                          'project-ai-dialog__tree-subtask',
                          {
                            'is-selected': selected.kind === 'subtask'
                              && selected.nodeId === node.local_id
                              && selected.subtaskId === subtask.local_id,
                          },
                        ]"
                        @click="selectSubtask(node.local_id, subtask.local_id)"
                      >
                        {{ subtask.name || '未命名子任务' }}
                      </button>

                      <button
                        type="button"
                        class="project-ai-dialog__tree-subtask project-ai-dialog__tree-subtask--add"
                        :disabled="isSubmitting"
                        @click="addSubtask(node.local_id)"
                      >
                        + 新增子任务
                      </button>
                    </div>
                  </article>
                </div>
              </div>

              <div v-else class="project-ai-dialog__empty">
                <strong>还没有 AI 草稿</strong>
                <p>先在左侧输入提示词并生成，再到中间调整项目结构。</p>
              </div>
            </div>
          </section>

          <section class="project-ai-dialog__panel project-ai-dialog__panel--editor">
            <div class="project-ai-dialog__panel-head">
              <p class="project-ai-dialog__panel-label">属性编辑</p>
              <h3 v-if="selected.kind === 'project'">当前选中：项目</h3>
              <h3 v-else-if="selected.kind === 'node'">当前选中：阶段</h3>
              <h3 v-else>当前选中：子任务</h3>
            </div>

            <div class="project-ai-dialog__panel-content">
              <template v-if="draft && selected.kind === 'project'">
                <label class="project-ai-dialog__field">
                  <span>项目名称</span>
                  <input v-model="draft.project.name" type="text">
                </label>

                <label class="project-ai-dialog__field">
                  <span>项目描述</span>
                  <textarea v-model="draft.project.description" rows="5"></textarea>
                </label>

                <div class="project-ai-dialog__field-grid">
                  <label class="project-ai-dialog__field">
                    <span>计划开始</span>
                    <input v-model="draft.project.planned_start_date" type="date">
                  </label>

                  <label class="project-ai-dialog__field">
                    <span>计划结束</span>
                    <input v-model="draft.project.planned_end_date" type="date">
                  </label>
                </div>
              </template>

              <template v-else-if="selectedNode && selected.kind === 'node'">
                <label class="project-ai-dialog__field">
                  <span>阶段名称</span>
                  <input v-model="selectedNode.name" type="text">
                </label>

                <label class="project-ai-dialog__field">
                  <span>阶段描述</span>
                  <textarea v-model="selectedNode.description" rows="5"></textarea>
                </label>

                <div class="project-ai-dialog__field-grid">
                  <label class="project-ai-dialog__field">
                    <span>计划开始</span>
                    <input v-model="selectedNode.planned_start_date" type="date">
                  </label>

                  <label class="project-ai-dialog__field">
                    <span>计划结束</span>
                    <input v-model="selectedNode.planned_end_date" type="date">
                  </label>
                </div>

                <div class="project-ai-dialog__inline-actions">
                  <button type="button" class="project-ai-dialog__ghost" @click="addSubtask(selectedNode.local_id)">
                    新增子任务
                  </button>
                  <button type="button" class="project-ai-dialog__danger" @click="removeSelectedNode">
                    删除当前阶段
                  </button>
                </div>
              </template>

              <template v-else-if="selectedSubtask && selected.kind === 'subtask'">
                <label class="project-ai-dialog__field">
                  <span>子任务名称</span>
                  <input v-model="selectedSubtask.name" type="text">
                </label>

                <label class="project-ai-dialog__field">
                  <span>子任务描述</span>
                  <textarea v-model="selectedSubtask.description" rows="5"></textarea>
                </label>

                <label class="project-ai-dialog__field">
                  <span>优先级</span>
                  <select v-model.number="selectedSubtask.priority">
                    <option :value="1">低</option>
                    <option :value="2">中</option>
                    <option :value="3">高</option>
                  </select>
                </label>

                <div class="project-ai-dialog__field-grid">
                  <label class="project-ai-dialog__field">
                    <span>计划开始</span>
                    <input v-model="selectedSubtask.planned_start_date" type="date">
                  </label>

                  <label class="project-ai-dialog__field">
                    <span>计划结束</span>
                    <input v-model="selectedSubtask.planned_end_date" type="date">
                  </label>
                </div>

                <div class="project-ai-dialog__inline-actions">
                  <button type="button" class="project-ai-dialog__danger" @click="removeSelectedSubtask">
                    删除当前子任务
                  </button>
                </div>
              </template>

              <div v-else class="project-ai-dialog__empty">
                <strong>没有可编辑项</strong>
                <p>生成草稿后，从中间结构树选择项目、阶段或子任务进行编辑。</p>
              </div>
            </div>
          </section>
        </div>

        <footer class="project-ai-dialog__footer">
          <div class="project-ai-dialog__footer-actions">
            <button
              type="button"
              class="project-ai-dialog__primary"
              :disabled="draft === null || isGenerating || isSubmitting"
              @click="submitDraft"
            >
              {{ isSubmitting ? '提交中...' : '最终提交' }}
            </button>
          </div>
        </footer>
      </section>
    </div>
  </Transition>
</template>

<style scoped>
.project-ai-dialog-enter-active,
.project-ai-dialog-leave-active {
  transition:
    opacity 260ms ease-out,
    backdrop-filter 260ms ease-out;
}

.project-ai-dialog-enter-active .project-ai-dialog,
.project-ai-dialog-leave-active .project-ai-dialog {
  transition:
    opacity 300ms cubic-bezier(0.16, 1, 0.3, 1),
    transform 340ms cubic-bezier(0.16, 1, 0.3, 1);
}

.project-ai-dialog-enter-from,
.project-ai-dialog-leave-to {
  opacity: 0;
  backdrop-filter: blur(0px);
}

.project-ai-dialog-enter-from .project-ai-dialog,
.project-ai-dialog-leave-to .project-ai-dialog {
  opacity: 0;
  transform:
    perspective(1200px)
    translate3d(var(--project-ai-translate-x), var(--project-ai-translate-y), 0)
    scale(0.96)
    rotateX(4deg);
}

.project-ai-dialog__backdrop {
  --project-ai-origin-x: 50vw;
  --project-ai-origin-y: 18vh;
  --project-ai-translate-x: 0px;
  --project-ai-translate-y: 24px;
  position: fixed;
  inset: 0;
  z-index: 120;
  display: grid;
  padding: 18px;
  background:
    radial-gradient(circle at 18% 20%, rgba(0, 182, 255, 0.12), transparent 24%),
    radial-gradient(circle at 82% 74%, rgba(10, 102, 255, 0.14), transparent 24%),
    rgba(4, 9, 18, 0.78);
  backdrop-filter: blur(14px);
}

.project-ai-dialog__backdrop::before {
  content: '';
  position: absolute;
  inset: 0;
  background:
    linear-gradient(rgba(104, 137, 191, 0.08) 1px, transparent 1px),
    linear-gradient(90deg, rgba(104, 137, 191, 0.08) 1px, transparent 1px);
  background-size: 26px 26px;
  pointer-events: none;
}

.project-ai-dialog__glow {
  position: absolute;
  left: var(--project-ai-origin-x);
  top: var(--project-ai-origin-y);
  width: min(56vw, 560px);
  height: min(56vw, 560px);
  border-radius: 999px;
  background: radial-gradient(circle, rgba(0, 176, 255, 0.24) 0%, rgba(10, 102, 255, 0.16) 42%, transparent 74%);
  filter: blur(24px);
  transform: translate(-50%, -50%);
  pointer-events: none;
}

.project-ai-dialog {
  position: relative;
  display: grid;
  grid-template-rows: auto auto minmax(0, 1fr);
  gap: 12px;
  width: min(100%, 1480px);
  height: min(880px, calc(100vh - 36px));
  margin: 0 auto;
  padding: 24px;
  border: 1px solid var(--dialog-surface-border);
  border-radius: 26px;
  background: var(--meta-surface-bg), var(--meta-surface-glow), var(--card-sheen);
  box-shadow:
    var(--meta-surface-shadow),
    0 28px 56px rgba(3, 10, 24, 0.34);
  overflow: hidden;
  transform-origin: var(--project-ai-origin-x) var(--project-ai-origin-y);
}

.project-ai-dialog__header,
.project-ai-dialog__footer {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 18px;
}

.project-ai-dialog__panel-label,
.project-ai-dialog__field span {
  margin: 0;
  font-size: 0.76rem;
  letter-spacing: 0.16em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.project-ai-dialog__header h2,
.project-ai-dialog__panel-head h3 {
  margin: 0;
}

.project-ai-dialog__header h2 {
  font-size: clamp(1.72rem, 2.8vw, 2.3rem);
}

.project-ai-dialog__close,
.project-ai-dialog__ghost,
.project-ai-dialog__danger,
.project-ai-dialog__primary {
  font: inherit;
  font-weight: 700;
  border-radius: 14px;
  cursor: pointer;
  transition:
    transform 180ms ease-out,
    border-color 180ms ease-out,
    box-shadow 180ms ease-out,
    background 180ms ease-out,
    color 180ms ease-out,
    opacity 180ms ease-out;
}

.project-ai-dialog__close,
.project-ai-dialog__ghost,
.project-ai-dialog__danger {
  border: 1px solid var(--dialog-control-border);
  padding: 11px 16px;
  background: var(--dialog-control-bg);
  box-shadow: var(--dialog-control-shadow);
  color: var(--text-main);
}

.project-ai-dialog__primary {
  border: 0;
  padding: 12px 18px;
  background: linear-gradient(135deg, color-mix(in srgb, var(--accent-start) 84%, #173668), color-mix(in srgb, var(--accent-end) 80%, #08182f));
  color: #f7fbff;
  box-shadow: 0 16px 28px rgba(10, 102, 255, 0.22);
}

.project-ai-dialog__danger {
  border-color: color-mix(in srgb, var(--work-status-delayed-border) 86%, transparent);
  background: color-mix(in srgb, var(--work-status-delayed-bg) 80%, var(--dialog-control-bg));
  color: var(--work-status-delayed-color);
}

.project-ai-dialog__close:hover,
.project-ai-dialog__ghost:hover,
.project-ai-dialog__danger:hover,
.project-ai-dialog__primary:hover {
  transform: translateY(-1px);
}

.project-ai-dialog__close:disabled,
.project-ai-dialog__ghost:disabled,
.project-ai-dialog__danger:disabled,
.project-ai-dialog__primary:disabled {
  opacity: 0.56;
  cursor: not-allowed;
  transform: none;
}

.project-ai-dialog__banner {
  display: grid;
  gap: 10px;
  padding: 13px 16px;
  border: 1px solid color-mix(in srgb, var(--accent-line) 24%, transparent);
  border-radius: 16px;
  background: linear-gradient(180deg, color-mix(in srgb, var(--panel-bg) 94%, var(--app-bg)), color-mix(in srgb, var(--accent-end) 10%, var(--panel-bg)));
  color: var(--text-main);
}

.project-ai-dialog__banner--error {
  border-color: var(--work-status-delayed-border);
  background: var(--work-status-delayed-bg);
  color: var(--work-status-delayed-color);
}

.project-ai-dialog__status {
  display: grid;
  gap: 12px;
  min-height: 0;
}

.project-ai-dialog__progress-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.project-ai-dialog__progress-track {
  height: 8px;
  overflow: hidden;
  border-radius: 999px;
  background: color-mix(in srgb, var(--accent-line) 16%, transparent);
}

.project-ai-dialog__progress-fill {
  display: block;
  height: 100%;
  border-radius: inherit;
  background: linear-gradient(90deg, var(--accent-start), var(--accent-end));
}

.project-ai-dialog__body {
  min-height: 0;
  display: grid;
  grid-template-columns: 360px minmax(0, 1fr) 392px;
  gap: 16px;
  overflow: hidden;
  align-items: stretch;
  padding-bottom: 72px;
}

.project-ai-dialog__panel {
  height: 100%;
  min-height: 0;
  display: grid;
  gap: 14px;
  padding: 18px;
  border: 1px solid var(--meta-surface-border);
  border-radius: 20px;
  background: color-mix(in srgb, var(--panel-bg) 92%, transparent), var(--card-sheen);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.04);
}

.project-ai-dialog__panel--prompt {
  grid-template-rows: auto minmax(0, 1fr) auto;
}

.project-ai-dialog__panel--tree {
  grid-template-rows: auto minmax(0, 1fr);
}

.project-ai-dialog__panel--editor {
  grid-template-rows: auto minmax(0, 1fr);
}

.project-ai-dialog__panel-head {
  display: grid;
  gap: 6px;
}

.project-ai-dialog__panel-head--split {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 12px;
}

.project-ai-dialog__panel-head h3 {
  font-size: 1.06rem;
}

.project-ai-dialog__panel-content {
  min-height: 0;
  overflow: auto;
  display: grid;
  align-content: start;
  gap: 14px;
  padding-right: 4px;
}

.project-ai-dialog__panel-content--prompt {
  overflow: hidden;
}

.project-ai-dialog__panel-content--tree {
  overflow: hidden;
  padding-right: 0;
}

.project-ai-dialog__field {
  display: grid;
  gap: 8px;
}

.project-ai-dialog__field--fill {
  min-height: 0;
  height: 100%;
}

.project-ai-dialog__field span {
  font-weight: 700;
}

.project-ai-dialog__field input,
.project-ai-dialog__field textarea,
.project-ai-dialog__field select,
.project-ai-dialog__textarea {
  width: 100%;
  box-sizing: border-box;
  border: 1px solid var(--dialog-control-border);
  border-radius: 14px;
  padding: 12px 14px;
  background: var(--dialog-control-bg);
  box-shadow: var(--dialog-control-shadow);
  color: var(--text-main);
  font: inherit;
}

.project-ai-dialog__field textarea,
.project-ai-dialog__textarea {
  resize: none;
  min-height: 118px;
}

.project-ai-dialog__textarea {
  min-height: 100%;
  height: 100%;
}

.project-ai-dialog__field input:focus,
.project-ai-dialog__field textarea:focus,
.project-ai-dialog__field select:focus,
.project-ai-dialog__textarea:focus {
  outline: none;
  border-color: var(--accent-line);
  box-shadow:
    var(--dialog-control-shadow),
    0 0 0 4px color-mix(in srgb, var(--accent-end) 10%, transparent);
}

.project-ai-dialog__field-grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 12px;
}

.project-ai-dialog__inline-actions,
.project-ai-dialog__footer-actions {
  display: flex;
  align-items: center;
  justify-content: flex-end;
  gap: 10px;
  flex-wrap: wrap;
}

.project-ai-dialog__tree {
  min-height: 0;
  height: 100%;
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
  gap: 12px;
  contain: layout paint;
}

.project-ai-dialog__tree-project {
  display: grid;
  gap: 6px;
  width: 100%;
  padding: 14px 16px;
  border: 1px solid transparent;
  border-radius: 18px;
  background: linear-gradient(135deg, color-mix(in srgb, var(--accent-start) 74%, #173668), color-mix(in srgb, var(--accent-end) 70%, #08182f));
  color: #f7fbff;
  text-align: left;
  cursor: pointer;
  box-shadow: 0 16px 28px rgba(9, 46, 114, 0.2);
}

.project-ai-dialog__tree-project span,
.project-ai-dialog__tree-node-button span {
  color: inherit;
  font-size: 0.8rem;
}

.project-ai-dialog__tree-project.is-selected,
.project-ai-dialog__tree-subtask.is-selected {
  box-shadow:
    0 0 0 1px color-mix(in srgb, var(--accent-end) 36%, #ffffff),
    0 14px 24px rgba(4, 13, 28, 0.18);
}

.project-ai-dialog__tree-list {
  min-height: 0;
  overflow: auto;
  display: grid;
  align-content: start;
  gap: 12px;
  padding-right: 8px;
  overscroll-behavior: contain;
  scrollbar-gutter: stable both-edges;
  contain: layout paint;
}

.project-ai-dialog__tree-node {
  box-sizing: border-box;
  display: grid;
  gap: 10px;
  padding: 0;
  border: 0;
  border-radius: 0;
  background: transparent;
  box-shadow: none;
  contain: layout paint;
  content-visibility: auto;
  contain-intrinsic-size: 232px;
}

.project-ai-dialog__tree-node-button {
  box-sizing: border-box;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 14px;
  width: 100%;
  padding: 12px 14px;
  border: 1px solid color-mix(in srgb, var(--accent-line) 16%, transparent);
  border-radius: 18px;
  background: var(--dialog-control-bg), var(--card-sheen);
  box-shadow: var(--dialog-control-shadow);
  color: var(--text-main);
  text-align: left;
  cursor: pointer;
  min-height: 46px;
}

.project-ai-dialog__tree-node.is-selected .project-ai-dialog__tree-node-button {
  box-shadow:
    0 0 0 1px color-mix(in srgb, var(--accent-end) 36%, #ffffff),
    0 14px 24px rgba(4, 13, 28, 0.18);
}

.project-ai-dialog__tree-node-button strong,
.project-ai-dialog__tree-node-button span,
.project-ai-dialog__tree-project strong,
.project-ai-dialog__tree-project span,
.project-ai-dialog__tree-subtask {
  display: block;
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.project-ai-dialog__tree-node-button strong {
  flex: 1 1 auto;
}

.project-ai-dialog__tree-node-button span {
  flex: none;
}

.project-ai-dialog__tree-subtasks {
  display: grid;
  gap: 8px;
  padding-left: 14px;
  background: transparent;
  border: 0;
  box-shadow: none;
  contain: layout paint;
}

.project-ai-dialog__tree-subtask {
  box-sizing: border-box;
  display: flex;
  align-items: center;
  width: 100%;
  padding: 10px 12px;
  border: 1px solid color-mix(in srgb, var(--accent-line) 12%, transparent);
  border-radius: 14px;
  background: color-mix(in srgb, var(--dialog-control-bg) 94%, transparent);
  color: var(--text-soft);
  text-align: left;
  cursor: pointer;
  min-height: 40px;
  contain: layout paint;
  content-visibility: auto;
  contain-intrinsic-size: 40px;
}

.project-ai-dialog__tree-subtask--add {
  border-style: dashed;
  color: color-mix(in srgb, var(--accent-end) 72%, var(--text-soft));
}

.project-ai-dialog__empty {
  align-self: center;
  display: grid;
  gap: 8px;
  padding: 20px;
  border: 1px dashed color-mix(in srgb, var(--accent-line) 22%, transparent);
  border-radius: 18px;
  background: color-mix(in srgb, var(--dialog-control-bg) 76%, transparent);
  text-align: center;
  color: var(--text-soft);
}

.project-ai-dialog__footer {
  position: absolute;
  right: 24px;
  bottom: 24px;
  z-index: 121;
  align-items: center;
  justify-content: flex-end;
  padding-top: 0;
}

.project-ai-dialog__footer-actions {
  flex-wrap: nowrap;
}

.project-ai-dialog__footer .project-ai-dialog__primary {
  padding: 10px 16px;
  min-width: 132px;
  box-shadow: 0 12px 22px rgba(10, 102, 255, 0.18);
}

@media (max-width: 1280px) {
  .project-ai-dialog__body {
    grid-template-columns: 1fr;
    overflow: auto;
    padding-right: 4px;
  }

  .project-ai-dialog {
    height: min(100%, calc(100vh - 30px));
  }
}

@media (max-width: 760px) {
  .project-ai-dialog__backdrop {
    padding: 10px;
  }

  .project-ai-dialog {
    padding: 16px;
    border-radius: 20px;
  }

  .project-ai-dialog__header,
  .project-ai-dialog__footer {
    display: grid;
  }

  .project-ai-dialog__footer {
    right: 16px;
    bottom: 16px;
  }

  .project-ai-dialog__field-grid {
    grid-template-columns: 1fr;
  }
}
</style>
