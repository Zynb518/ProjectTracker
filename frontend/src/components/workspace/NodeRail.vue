<script setup lang="ts">
import { computed, onBeforeUnmount, onMounted, ref } from 'vue'
import type { CSSProperties } from 'vue'

import NodeActionsMenu from '@/components/workspace/NodeActionsMenu.vue'
import type { ProjectNode } from '@/types/node'

type NodeRailViewMode = 'compact' | 'full'

const props = withDefaults(defineProps<{
  canManage?: boolean
  defaultViewMode?: NodeRailViewMode
  embedded?: boolean
  nodes: ProjectNode[]
  selectedNodeId: number | null
}>(), {
  canManage: false,
  defaultViewMode: 'compact',
  embedded: false,
})

const emit = defineEmits<{
  complete: [nodeId: number]
  create: []
  edit: [nodeId: number]
  remove: [nodeId: number]
  reorder: [payload: { nodes: Array<{ node_id: number; sequence_no: number }> }]
  reopen: [nodeId: number]
  select: [nodeId: number]
  start: [nodeId: number]
}>()

const draggedNodeId = ref<number | null>(null)
const displayMode = ref<NodeRailViewMode>(props.defaultViewMode)
const hoveredAnchor = ref<HTMLElement | null>(null)
const hoveredNodeId = ref<number | null>(null)
const hoverCardPosition = ref<{ left: number; top: number } | null>(null)
const previewMovableNodeIds = ref<number[] | null>(null)
const dropTargetNodeId = ref<number | null>(null)

const HOVER_CARD_WIDTH = 320
const HOVER_CARD_HEIGHT = 224
const HOVER_CARD_GAP = 18
const HOVER_CARD_MARGIN = 20

const orderedNodes = computed(() =>
  [...props.nodes].sort((left, right) => left.sequence_no - right.sequence_no),
)

const movableNodeIds = computed(() =>
  orderedNodes.value
    .filter((node) => node.status !== 3)
    .map((node) => node.id),
)

const activeMovableNodeIds = computed(() => previewMovableNodeIds.value ?? movableNodeIds.value)

const displayNodes = computed(() => mergeNodesByMovableIds(activeMovableNodeIds.value))
const isCompactView = computed(() => displayMode.value === 'compact')

const hoveredNode = computed(() =>
  orderedNodes.value.find((node) => node.id === hoveredNodeId.value) ?? null,
)

const hoverCardStyle = computed<CSSProperties | undefined>(() => {
  if (hoverCardPosition.value === null) {
    return undefined
  }

  return {
    left: `${hoverCardPosition.value.left}px`,
    top: `${hoverCardPosition.value.top}px`,
    pointerEvents: 'none',
    position: 'fixed',
    zIndex: '240',
  }
})

function mergeNodesByMovableIds(nodeIds: number[]) {
  const nodeMap = new Map(orderedNodes.value.map((node) => [node.id, node]))
  let movableIndex = 0

  return orderedNodes.value.map((node) => {
    if (node.status === 3) {
      return node
    }

    const currentNode = nodeMap.get(nodeIds[movableIndex])
    movableIndex += 1
    return currentNode ?? node
  })
}

function statusLabel(status: number) {
  return (
    {
      1: '未开始',
      2: '进行中',
      3: '已完成',
      4: '已延期',
    }[status] ?? '未知状态'
  )
}

function statusTone(status: number) {
  return (
    {
      1: 'pending',
      2: 'active',
      3: 'done',
      4: 'delayed',
    }[status] ?? 'unknown'
  )
}

function setDisplayMode(mode: NodeRailViewMode) {
  displayMode.value = mode
}

function isInteractive(status: number) {
  return status !== 3
}

function isDraggable(node: ProjectNode) {
  return props.canManage && node.status !== 3
}

function progressPercent(node: ProjectNode) {
  if (node.sub_task_count === 0) {
    return 0
  }

  return Math.round((node.completed_sub_task_count / node.sub_task_count) * 100)
}

function handleSelect(node: ProjectNode) {
  if (!isInteractive(node.status)) {
    return
  }

  emit('select', node.id)
}

function handleCardKeydown(node: ProjectNode, event: KeyboardEvent) {
  if (event.key !== 'Enter' && event.key !== ' ') {
    return
  }

  event.preventDefault()
  handleSelect(node)
}

function clamp(value: number, min: number, max: number) {
  return Math.min(Math.max(value, min), max)
}

function updateHoverCardPosition() {
  if (hoveredAnchor.value === null || typeof window === 'undefined') {
    hoverCardPosition.value = null
    return
  }

  const rect = hoveredAnchor.value.getBoundingClientRect()
  const viewportWidth = window.innerWidth || document.documentElement.clientWidth
  const viewportHeight = window.innerHeight || document.documentElement.clientHeight

  hoverCardPosition.value = {
    left: clamp(
      rect.right + HOVER_CARD_GAP,
      HOVER_CARD_MARGIN,
      viewportWidth - HOVER_CARD_WIDTH - HOVER_CARD_MARGIN,
    ),
    top: clamp(
      rect.top - 6,
      HOVER_CARD_MARGIN,
      viewportHeight - HOVER_CARD_HEIGHT - HOVER_CARD_MARGIN,
    ),
  }
}

function showHover(nodeId: number, anchor: HTMLElement | null) {
  hoveredNodeId.value = nodeId
  hoveredAnchor.value = anchor
  updateHoverCardPosition()
}

function showHoverFromEvent(nodeId: number, event: FocusEvent | MouseEvent) {
  showHover(nodeId, event.currentTarget instanceof HTMLElement ? event.currentTarget : null)
}

function clearHover(nodeId?: number) {
  if (nodeId !== undefined && hoveredNodeId.value !== nodeId) {
    return
  }

  hoveredNodeId.value = null
  hoveredAnchor.value = null
  hoverCardPosition.value = null
}

function reorderPreview(overNodeId: number) {
  if (draggedNodeId.value === null || draggedNodeId.value === overNodeId) {
    return
  }

  const currentIds = [...activeMovableNodeIds.value]
  const sourceIndex = currentIds.indexOf(draggedNodeId.value)
  const targetIndex = currentIds.indexOf(overNodeId)

  if (sourceIndex === -1 || targetIndex === -1 || sourceIndex === targetIndex) {
    return
  }

  currentIds.splice(sourceIndex, 1)
  currentIds.splice(currentIds.indexOf(overNodeId), 0, draggedNodeId.value)
  previewMovableNodeIds.value = currentIds
}

function startDrag(node: ProjectNode, event: DragEvent) {
  if (!isDraggable(node)) {
    return
  }

  draggedNodeId.value = node.id
  dropTargetNodeId.value = node.id
  previewMovableNodeIds.value = [...movableNodeIds.value]
  clearHover()

  if (event.dataTransfer) {
    event.dataTransfer.effectAllowed = 'move'
    event.dataTransfer.setData('text/plain', String(node.id))
  }
}

function enterDropTarget(node: ProjectNode) {
  if (draggedNodeId.value === null || node.status === 3) {
    return
  }

  dropTargetNodeId.value = node.id
  reorderPreview(node.id)
}

function resetDragState() {
  draggedNodeId.value = null
  dropTargetNodeId.value = null
  previewMovableNodeIds.value = null
}

function commitReorder() {
  if (draggedNodeId.value === null || previewMovableNodeIds.value === null) {
    resetDragState()
    return
  }

  const reorderedNodeIds = displayNodes.value.map((node) => node.id)
  const originalNodeIds = orderedNodes.value.map((node) => node.id)

  if (reorderedNodeIds.every((nodeId, index) => nodeId === originalNodeIds[index])) {
    resetDragState()
    return
  }

  emit('reorder', {
    nodes: displayNodes.value.map((node, index) => ({
      node_id: node.id,
      sequence_no: index + 1,
    })),
  })

  resetDragState()
}

function cancelDrag() {
  resetDragState()
}

function handleViewportChange() {
  if (hoveredNodeId.value === null) {
    return
  }

  updateHoverCardPosition()
}

onMounted(() => {
  if (typeof window === 'undefined') {
    return
  }

  window.addEventListener('resize', handleViewportChange)
  window.addEventListener('scroll', handleViewportChange, true)
})

onBeforeUnmount(() => {
  if (typeof window === 'undefined') {
    return
  }

  window.removeEventListener('resize', handleViewportChange)
  window.removeEventListener('scroll', handleViewportChange, true)
})
</script>

<template>
  <section
    :class="['node-rail', { 'node-rail--embedded': embedded }]"
    @mouseleave="clearHover()"
  >
    <header class="node-rail__header">
      <div>
        <p>阶段节点</p>
        <span>
          {{ isCompactView ? '默认精简视图，悬浮查看详情，点击展开节点内容。' : '拖拽未完成阶段即可自动挤占排序。' }}
        </span>
      </div>
      <div class="node-rail__header-side">
        <div class="node-rail__view-switch" role="tablist" aria-label="阶段节点视图">
          <button
            data-testid="node-view-mode-compact"
            class="node-rail__view-button"
            :class="{ 'is-active': isCompactView }"
            :aria-pressed="isCompactView"
            type="button"
            @click="setDisplayMode('compact')"
          >
            <svg aria-hidden="true" class="node-rail__view-icon" viewBox="0 0 24 24">
              <path
                d="M5 7.5h14M5 12h14M5 16.5h14"
                fill="none"
                stroke="currentColor"
                stroke-linecap="round"
                stroke-width="1.7"
              />
            </svg>
            <span>精简</span>
          </button>
          <button
            data-testid="node-view-mode-full"
            class="node-rail__view-button"
            :class="{ 'is-active': !isCompactView }"
            :aria-pressed="!isCompactView"
            type="button"
            @click="setDisplayMode('full')"
          >
            <svg aria-hidden="true" class="node-rail__view-icon" viewBox="0 0 24 24">
              <path
                d="M5 6.5h14M5 12h14M5 17.5h7"
                fill="none"
                stroke="currentColor"
                stroke-linecap="round"
                stroke-width="1.7"
              />
              <path
                d="M16.5 16.25h2.25M19.875 16.25h.125M16.5 18.75h2.25M19.875 18.75h.125"
                fill="none"
                stroke="currentColor"
                stroke-linecap="round"
                stroke-width="1.7"
              />
            </svg>
            <span>完整</span>
          </button>
        </div>
        <strong>{{ props.nodes.length }} 个节点</strong>
        <button
          v-if="canManage"
          data-testid="create-node"
          aria-label="新建节点"
          class="node-rail__create"
          data-tooltip="新建节点"
          type="button"
          @click="$emit('create')"
        >
          <svg aria-hidden="true" class="node-rail__create-icon" viewBox="0 0 24 24">
            <path
              d="M12 5v14M5 12h14"
              fill="none"
              stroke="currentColor"
              stroke-linecap="round"
              stroke-width="2"
            />
          </svg>
        </button>
      </div>
    </header>

    <TransitionGroup
      tag="div"
      name="node-list"
      data-testid="node-rail-scroll"
      class="node-rail__timeline"
    >
      <article
        v-for="node in displayNodes"
        :key="node.id"
        :data-testid="`node-row-${node.id}`"
        class="node-rail__row"
        :class="[
          `node-rail__row--${statusTone(node.status)}`,
          {
            'is-active': node.id === selectedNodeId,
            'is-complete': node.status === 3,
            'is-dragging': draggedNodeId === node.id,
            'is-drop-target': dropTargetNodeId === node.id && draggedNodeId !== node.id,
          },
        ]"
      >
        <div class="node-rail__track" aria-hidden="true">
          <span class="node-rail__dot" />
        </div>

        <div class="node-rail__content">
          <article
            :data-testid="`node-item-${node.id}`"
            class="node-rail__item"
            :class="{
              'node-rail__item--compact': isCompactView,
              'is-interactive': isInteractive(node.status),
              'is-active': node.id === selectedNodeId,
            }"
            :draggable="isDraggable(node)"
            :role="isInteractive(node.status) ? 'button' : undefined"
            :tabindex="isInteractive(node.status) ? 0 : undefined"
            @blur="clearHover(node.id)"
            @dragend="cancelDrag"
            @dragenter.prevent="enterDropTarget(node)"
            @dragover.prevent
            @dragstart="startDrag(node, $event)"
            @drop.prevent="commitReorder"
            @click="handleSelect(node)"
            @focus="showHoverFromEvent(node.id, $event)"
            @keydown="handleCardKeydown(node, $event)"
            @mouseenter="showHoverFromEvent(node.id, $event)"
            @mouseleave="clearHover(node.id)"
          >
            <template v-if="isCompactView">
              <div class="node-rail__compact-head">
                <strong class="node-rail__name node-rail__name--compact">{{ node.name }}</strong>
                <span :class="['node-rail__status', `node-rail__status--${statusTone(node.status)}`]">
                  {{ statusLabel(node.status) }}
                </span>
              </div>
            </template>

            <template v-else>
              <div class="node-rail__item-top">
                <div class="node-rail__item-top-left">
                  <span class="node-rail__sequence">Phase {{ node.sequence_no }}</span>
                  <span v-if="isDraggable(node)" class="node-rail__drag-pill">
                    <svg aria-hidden="true" class="node-rail__drag-icon" viewBox="0 0 24 24">
                      <path
                        d="M9 7.5h.01M9 12h.01M9 16.5h.01M15 7.5h.01M15 12h.01M15 16.5h.01"
                        fill="none"
                        stroke="currentColor"
                        stroke-linecap="round"
                        stroke-width="2"
                      />
                    </svg>
                  </span>
                </div>
                <span :class="['node-rail__status', `node-rail__status--${statusTone(node.status)}`]">
                  {{ statusLabel(node.status) }}
                </span>
              </div>

              <strong class="node-rail__name">{{ node.name }}</strong>

              <div class="node-rail__progress">
                <span>{{ node.completed_sub_task_count }} / {{ node.sub_task_count }} 子任务完成</span>
                <span>{{ progressPercent(node) }}%</span>
              </div>

              <NodeActionsMenu
                v-if="canManage"
                :node="node"
                @complete="$emit('complete', node.id)"
                @edit="$emit('edit', node.id)"
                @remove="$emit('remove', node.id)"
                @reopen="$emit('reopen', node.id)"
                @start="$emit('start', node.id)"
              />
            </template>
          </article>
        </div>
      </article>
    </TransitionGroup>

    <Teleport to="body">
      <transition name="node-hover-card">
        <aside
          v-if="hoveredNode !== null && hoverCardStyle"
          data-testid="node-hover-card"
          class="node-rail__hover-card node-rail__hover-card--right"
          :style="hoverCardStyle"
        >
          <div class="node-rail__hover-head">
            <strong>{{ hoveredNode.name }}</strong>
            <span
              :class="[
                'node-rail__hover-status',
                `node-rail__hover-status--${statusTone(hoveredNode.status)}`,
              ]"
            >
              {{ statusLabel(hoveredNode.status) }}
            </span>
          </div>
          <p>{{ hoveredNode.description || '当前阶段暂无补充说明。' }}</p>
          <dl class="node-rail__hover-meta">
            <div>
              <dt>计划周期</dt>
              <dd>{{ hoveredNode.planned_start_date }} - {{ hoveredNode.planned_end_date }}</dd>
            </div>
            <div>
              <dt>子任务进度</dt>
              <dd>{{ hoveredNode.completed_sub_task_count }} / {{ hoveredNode.sub_task_count }}</dd>
            </div>
          </dl>
        </aside>
      </transition>
    </Teleport>
  </section>
</template>

<style scoped>
.node-rail {
  display: grid;
  grid-template-rows: auto minmax(0, 1fr);
  gap: 18px;
  min-height: 0;
  height: 100%;
  padding: 22px 20px;
  border: 1px solid var(--border-soft);
  border-radius: 24px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--glass-bg) 94%, transparent), var(--glass-bg)),
    radial-gradient(circle at top left, color-mix(in srgb, var(--accent-start) 16%, transparent), transparent 38%);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
}

.node-rail--embedded {
  gap: 16px;
  padding: 0;
  border: none;
  border-radius: 0;
  background: none;
  box-shadow: none;
  backdrop-filter: none;
}

.node-rail__header {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 16px;
}

.node-rail__header-side {
  display: grid;
  justify-items: end;
  gap: 10px;
}

.node-rail__view-switch {
  display: inline-grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 6px;
  padding: 4px;
  border: 1px solid var(--border-soft);
  border-radius: 16px;
  background: color-mix(in srgb, var(--panel-bg) 78%, transparent);
  box-shadow: inset 0 1px 0 color-mix(in srgb, var(--glass-bg-strong) 52%, transparent);
  backdrop-filter: var(--backdrop-blur);
}

.node-rail__view-button {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  min-height: 36px;
  border: none;
  border-radius: 12px;
  padding: 0 12px;
  background: transparent;
  color: var(--text-soft);
  font: inherit;
  font-size: 0.82rem;
  font-weight: 700;
  cursor: pointer;
  transition:
    transform 200ms ease-out,
    color 200ms ease-out,
    background 200ms ease-out,
    box-shadow 200ms ease-out;
}

.node-rail__view-button:hover {
  color: var(--text-main);
}

.node-rail__view-button.is-active {
  background:
    linear-gradient(135deg, color-mix(in srgb, var(--accent-start) 18%, transparent), color-mix(in srgb, var(--accent-end) 18%, transparent)),
    color-mix(in srgb, var(--panel-bg) 86%, transparent);
  color: var(--text-main);
  box-shadow:
    inset 0 0 0 1px color-mix(in srgb, var(--accent-line) 18%, transparent),
    0 8px 18px rgba(15, 23, 42, 0.12);
}

.node-rail__view-button:focus-visible {
  outline: none;
  color: var(--text-main);
  transform: translateY(-1px);
  box-shadow:
    0 0 0 4px color-mix(in srgb, var(--accent-end) 14%, transparent),
    0 8px 18px rgba(15, 23, 42, 0.12);
}

.node-rail__view-icon {
  width: 14px;
  height: 14px;
}

.node-rail__header div {
  display: grid;
  gap: 6px;
}

.node-rail__header p,
.node-rail__header span,
.node-rail__header strong {
  margin: 0;
}

.node-rail__header p {
  font-size: 1.05rem;
  font-weight: 700;
  letter-spacing: 0.02em;
}

.node-rail__header span {
  color: var(--text-soft);
  font-size: 0.86rem;
}

.node-rail__header strong {
  padding: 8px 12px;
  border: 1px solid var(--border-soft);
  border-radius: 999px;
  background: color-mix(in srgb, var(--panel-bg) 86%, transparent);
  font-size: 0.84rem;
  line-height: 1;
}

.node-rail__create {
  position: relative;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 44px;
  height: 44px;
  border: 1px solid color-mix(in srgb, var(--accent-start) 28%, transparent);
  border-radius: 14px;
  padding: 0;
  background: linear-gradient(135deg, var(--accent-start), var(--accent-end));
  color: #fff;
  font: inherit;
  font-weight: 600;
  cursor: pointer;
  box-shadow: var(--shadow-panel);
  transition:
    transform 200ms ease-out,
    box-shadow 200ms ease-out;
}

.node-rail__create:hover {
  transform: translateY(-2px);
  box-shadow: var(--shadow-panel-hover);
}

.node-rail__create:focus-visible {
  outline: none;
  transform: translateY(-2px);
  box-shadow:
    var(--shadow-panel-hover),
    0 0 0 4px color-mix(in srgb, var(--accent-end) 18%, transparent);
}

.node-rail__create-icon {
  width: 18px;
  height: 18px;
}

.node-rail__create[data-tooltip]::before,
.node-rail__create[data-tooltip]::after {
  position: absolute;
  left: 50%;
  opacity: 0;
  pointer-events: none;
  transition:
    opacity 180ms ease-out,
    transform 180ms ease-out;
}

.node-rail__create[data-tooltip]::before {
  content: '';
  bottom: calc(100% + 5px);
  width: 10px;
  height: 10px;
  border-left: 1px solid var(--border-soft);
  border-bottom: 1px solid var(--border-soft);
  background: color-mix(in srgb, var(--panel-bg) 96%, transparent);
  transform: translate(-50%, 6px) rotate(-45deg);
  z-index: 1;
}

.node-rail__create[data-tooltip]::after {
  content: attr(data-tooltip);
  bottom: calc(100% + 11px);
  padding: 7px 10px;
  border: 1px solid var(--border-soft);
  border-radius: 10px;
  background: color-mix(in srgb, var(--panel-bg) 96%, transparent);
  color: var(--text-main);
  font-size: 0.75rem;
  font-weight: 600;
  line-height: 1;
  white-space: nowrap;
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
  transform: translate(-50%, 6px);
  z-index: 2;
}

.node-rail__create[data-tooltip]:hover::before,
.node-rail__create[data-tooltip]:hover::after,
.node-rail__create[data-tooltip]:focus-visible::before,
.node-rail__create[data-tooltip]:focus-visible::after {
  opacity: 1;
  transform: translate(-50%, 0);
}

.node-rail__create[data-tooltip]:hover::before,
.node-rail__create[data-tooltip]:focus-visible::before {
  transform: translate(-50%, 0) rotate(-45deg);
}

.node-rail__timeline {
  position: relative;
  display: grid;
  align-content: start;
  gap: 14px;
  min-height: 0;
  overflow-y: auto;
  padding-right: 8px;
}

.node-list-move,
.node-list-enter-active,
.node-list-leave-active {
  transition:
    transform 240ms ease-out,
    opacity 200ms ease-out;
}

.node-list-enter-from,
.node-list-leave-to {
  opacity: 0;
}

.node-rail__timeline::before {
  content: '';
  position: absolute;
  top: 12px;
  bottom: 12px;
  left: 13px;
  width: 1px;
  background: linear-gradient(
    180deg,
    color-mix(in srgb, var(--accent-end) 42%, transparent),
    color-mix(in srgb, var(--border-soft) 92%, transparent)
  );
}

.node-rail__row {
  position: relative;
  display: grid;
  grid-template-columns: 28px minmax(0, 1fr);
  gap: 14px;
  transition:
    opacity 180ms ease-out,
    transform 180ms ease-out;
}

.node-rail__row.is-dragging {
  opacity: 0.56;
}

.node-rail__row.is-drop-target .node-rail__item {
  border-color: color-mix(in srgb, var(--accent-line) 62%, transparent);
  box-shadow: var(--shadow-panel-hover);
}

.node-rail__track {
  position: relative;
  display: flex;
  justify-content: center;
  padding-top: 16px;
}

.node-rail__content {
  display: grid;
  gap: 10px;
}

.node-rail__dot {
  position: relative;
  z-index: 1;
  width: 10px;
  height: 10px;
  border-radius: 50%;
  border: 2px solid color-mix(in srgb, var(--glass-bg-strong) 92%, transparent);
  background: linear-gradient(135deg, var(--accent-start), var(--accent-end));
  box-shadow: 0 0 0 6px color-mix(in srgb, var(--accent-start) 12%, transparent);
}

.node-rail__row--done .node-rail__dot {
  background: color-mix(in srgb, var(--text-soft) 76%, var(--panel-bg));
  box-shadow: 0 0 0 6px color-mix(in srgb, var(--text-soft) 10%, transparent);
}

.node-rail__row--delayed .node-rail__dot {
  background: var(--accent-warning);
  box-shadow: 0 0 0 6px color-mix(in srgb, var(--accent-warning) 14%, transparent);
}

.node-rail__item {
  position: relative;
  min-width: 0;
  display: grid;
  gap: 12px;
  text-align: left;
  padding: 16px 18px;
  border: 1px solid var(--border-soft);
  border-radius: 20px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--panel-bg) 86%, transparent), color-mix(in srgb, var(--panel-bg) 96%, transparent)),
    radial-gradient(circle at top right, color-mix(in srgb, var(--accent-start) 12%, transparent), transparent 38%);
  box-shadow: 0 10px 20px rgba(15, 23, 42, 0.06);
  color: var(--text-main);
  transition:
    transform 200ms ease-out,
    border-color 200ms ease-out,
    box-shadow 200ms ease-out,
    background 200ms ease-out;
}

.node-rail__item--compact {
  gap: 0;
  padding: 14px 16px;
}

.node-rail__item.is-interactive {
  cursor: pointer;
}

.node-rail__item[draggable='true'] {
  cursor: grab;
}

.node-rail__item[draggable='true']:active {
  cursor: grabbing;
}

.node-rail__item.is-interactive:hover,
.node-rail__item.is-interactive:focus-visible,
.node-rail__item.is-active {
  transform: translateY(-2px);
  border-color: color-mix(in srgb, var(--accent-line) 52%, transparent);
  box-shadow: var(--shadow-panel-hover);
}

.node-rail__row--done .node-rail__item {
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--panel-bg) 92%, var(--text-soft)), color-mix(in srgb, var(--panel-bg) 98%, transparent));
  color: color-mix(in srgb, var(--text-main) 70%, var(--text-soft));
}

.node-rail__item:focus-visible {
  outline: none;
}

.node-rail__item-top,
.node-rail__compact-head,
.node-rail__progress {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}

.node-rail__item-top-left {
  display: inline-flex;
  align-items: center;
  gap: 10px;
  min-width: 0;
}

.node-rail__sequence {
  color: var(--text-soft);
  font-size: 0.78rem;
  font-weight: 600;
  letter-spacing: 0.08em;
  text-transform: uppercase;
}

.node-rail__drag-pill {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  padding: 5px 10px;
  border: 1px dashed color-mix(in srgb, var(--accent-line) 54%, transparent);
  border-radius: 999px;
  color: var(--text-soft);
  font-size: 0.72rem;
  font-weight: 700;
  letter-spacing: 0.04em;
}

.node-rail__drag-icon {
  width: 12px;
  height: 12px;
}

.node-rail__status,
.node-rail__hover-status {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 6px;
  padding: 6px 12px;
  border-radius: 999px;
  font-size: 0.76rem;
  font-weight: 700;
  line-height: 1;
}

.node-rail__status--pending,
.node-rail__hover-status--pending {
  background: color-mix(in srgb, var(--text-soft) 18%, transparent);
  color: var(--text-soft);
}

.node-rail__status--active,
.node-rail__hover-status--active {
  background: color-mix(in srgb, var(--accent-start) 18%, transparent);
  color: var(--accent-start);
}

.node-rail__status--done,
.node-rail__hover-status--done {
  background: color-mix(in srgb, var(--text-soft) 18%, transparent);
  color: color-mix(in srgb, var(--text-main) 66%, var(--text-soft));
}

.node-rail__status--delayed,
.node-rail__hover-status--delayed {
  background: color-mix(in srgb, var(--accent-warning) 18%, transparent);
  color: var(--accent-warning);
}

.node-rail__status--unknown,
.node-rail__hover-status--unknown {
  background: color-mix(in srgb, var(--text-soft) 18%, transparent);
  color: var(--text-soft);
}

.node-rail__name {
  display: block;
  min-width: 0;
  font-size: 1.08rem;
  font-weight: 700;
  line-height: 1.4;
}

.node-rail__name--compact {
  flex: 1;
  font-size: 1rem;
  line-height: 1.35;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.node-rail__progress {
  color: var(--text-soft);
  font-size: 0.88rem;
}

.node-rail__hover-card {
  width: min(320px, calc(100vw - 56px));
  display: grid;
  gap: 12px;
  padding: 16px;
  border: 1px solid color-mix(in srgb, var(--accent-line) 26%, transparent);
  border-radius: 18px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--glass-bg-strong) 96%, transparent), var(--glass-bg-strong)),
    radial-gradient(circle at top right, color-mix(in srgb, var(--accent-end) 14%, transparent), transparent 40%);
  box-shadow: 0 18px 34px rgba(15, 23, 42, 0.16);
  backdrop-filter: var(--backdrop-blur);
  transform: translate3d(0, 0, 0);
  isolation: isolate;
}

.node-rail__hover-card--right::before {
  content: '';
  position: absolute;
  top: 22px;
  right: calc(100% - 7px);
  width: 14px;
  height: 14px;
  border-left: 1px solid color-mix(in srgb, var(--accent-line) 26%, transparent);
  border-bottom: 1px solid color-mix(in srgb, var(--accent-line) 26%, transparent);
  background: color-mix(in srgb, var(--glass-bg-strong) 98%, transparent);
  transform: rotate(45deg);
}

.node-rail__hover-head {
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  gap: 12px;
}

.node-rail__hover-head strong,
.node-rail__hover-card p,
.node-rail__hover-meta dt,
.node-rail__hover-meta dd {
  margin: 0;
}

.node-rail__hover-card p {
  color: var(--text-soft);
  font-size: 0.92rem;
  line-height: 1.6;
}

.node-rail__hover-meta {
  display: grid;
  grid-template-columns: repeat(2, minmax(0, 1fr));
  gap: 10px;
}

.node-rail__hover-meta div {
  display: grid;
  gap: 4px;
  padding: 10px 12px;
  border: 1px solid var(--border-soft);
  border-radius: 14px;
  background: color-mix(in srgb, var(--panel-bg) 82%, transparent);
}

.node-rail__hover-meta dt {
  color: var(--text-soft);
  font-size: 0.76rem;
}

.node-rail__hover-meta dd {
  font-size: 0.84rem;
  font-weight: 600;
  line-height: 1.5;
}

.node-hover-card-enter-active,
.node-hover-card-leave-active {
  transition:
    opacity 180ms ease-out,
    transform 180ms ease-out;
}

.node-hover-card-enter-from,
.node-hover-card-leave-to {
  opacity: 0;
  transform: translate3d(-10px, 0, 0);
}
</style>
