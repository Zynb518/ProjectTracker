<script setup lang="ts">
import { computed, nextTick, onBeforeUnmount, onMounted, ref, watch } from 'vue'

import { useBeginnerTutorialStore } from '@/stores/beginnerTutorial'

type TutorialRect = {
  top: number
  left: number
  width: number
  height: number
}

type TutorialCardPlacement = 'right' | 'left' | 'below' | 'above'

type TutorialContent = {
  badge: string
  title: string
  description: string
  target: string | null
  preferredPlacement?: TutorialCardPlacement
  showNextAction?: boolean
}

const tutorialStore = useBeginnerTutorialStore()
const targetRect = ref<TutorialRect | null>(null)
let animationFrameId = 0
let measureTimeoutIds: number[] = []

const spotlightPadding = 10
const spotlightViewportPadding = 8
const remeasureDelays = [0, 60, 140, 240, 360, 520]
const tutorialCardWidth = 360
const tutorialCardHeight = 220
const tutorialCardGap = 18
const tutorialCardHorizontalPadding = 24
const tutorialCardVerticalPadding = 24

const content = computed<TutorialContent | null>(() => {
  switch (tutorialStore.step) {
    case 'intro':
      return {
        badge: 'Beginner Path',
        title: '创建第一个项目',
        description: '这套教程只做一件事：带你走完第一次项目创建，不让你在入口按钮和弹窗之间来回试。',
        target: 'beginner-tutorial-entry',
      }
    case 'choose-mode':
      return {
        badge: 'Create Mode',
        title: '选择创建方式',
        description: '你可以直接手动创建，也可以先让 AI 生成项目草稿，再确认结构和日期。',
        target: 'project-create-actions',
      }
    case 'manual-form':
      return {
        badge: 'Manual Create',
        title: '填写项目基础信息',
        description: '先填项目名称和计划日期。描述可以先写简版，创建成功后会直接带你进入项目详情页。',
        target: 'project-form-dialog',
      }
    case 'ai-prompt':
      return {
        badge: 'AI Create',
        title: '先输入项目需求提示词',
        description: '把项目目标、阶段拆分和时间范围写进去，然后点击“AI 生成草稿”。草稿出来后，我会继续带你审阅。',
        target: 'project-ai-prompt',
      }
    case 'ai-review':
      return {
        badge: 'Draft Review',
        title: '先检查结构树和右侧属性编辑',
        description: '这里先确认阶段、子任务和日期关系是否合理。你可以在中间查看结构树，在右侧直接修改名称、描述和计划时间。',
        target: 'project-ai-review-stage',
        preferredPlacement: 'left',
        showNextAction: true,
      }
    case 'ai-submit':
      return {
        badge: 'Submit Draft',
        title: '确认无误后提交整份草稿',
        description: '如果结构和日期都确认好了，点击这个“最终提交”按钮，系统会按当前草稿顺序创建项目、阶段和子任务。',
        target: 'project-ai-submit',
        preferredPlacement: 'left',
      }
    case 'created-success':
      return {
        badge: 'Next Step',
        title: '第一个项目已经创建完成',
        description: '接下来你可以在项目详情页继续拆阶段、补子任务，并逐步进入协作流。',
        target: null,
      }
    default:
      return null
  }
})

const cardStyle = computed(() => {
  if (spotlightRect.value === null) {
    return {
      top: '50%',
      left: '50%',
      transform: 'translate(-50%, -50%)',
    }
  }

  return resolveCardStyle(spotlightRect.value)
})

const spotlightRect = computed<TutorialRect | null>(() => {
  if (targetRect.value === null || typeof window === 'undefined') {
    return null
  }

  const left = Math.max(targetRect.value.left - spotlightPadding, spotlightViewportPadding)
  const top = Math.max(targetRect.value.top - spotlightPadding, spotlightViewportPadding)
  const right = Math.min(
    targetRect.value.left + targetRect.value.width + spotlightPadding,
    window.innerWidth - spotlightViewportPadding,
  )
  const bottom = Math.min(
    targetRect.value.top + targetRect.value.height + spotlightPadding,
    window.innerHeight - spotlightViewportPadding,
  )

  return {
    top,
    left,
    width: Math.max(right - left, 24),
    height: Math.max(bottom - top, 24),
  }
})

const spotlightStyle = computed(() => {
  if (spotlightRect.value === null) {
    return undefined
  }

  return {
    top: `${spotlightRect.value.top}px`,
    left: `${spotlightRect.value.left}px`,
    width: `${spotlightRect.value.width}px`,
    height: `${spotlightRect.value.height}px`,
  }
})

const scrimSegments = computed(() => {
  if (typeof window === 'undefined') {
    return [] as TutorialRect[]
  }

  const viewportWidth = window.innerWidth
  const viewportHeight = window.innerHeight

  if (spotlightRect.value === null) {
    return [{
      top: 0,
      left: 0,
      width: viewportWidth,
      height: viewportHeight,
    }]
  }

  const rect = spotlightRect.value
  const right = rect.left + rect.width
  const bottom = rect.top + rect.height

  return [
    {
      top: 0,
      left: 0,
      width: viewportWidth,
      height: Math.max(rect.top, 0),
    },
    {
      top: rect.top,
      left: 0,
      width: Math.max(rect.left, 0),
      height: rect.height,
    },
    {
      top: rect.top,
      left: right,
      width: Math.max(viewportWidth - right, 0),
      height: rect.height,
    },
    {
      top: bottom,
      left: 0,
      width: viewportWidth,
      height: Math.max(viewportHeight - bottom, 0),
    },
  ].filter((segment) => segment.width > 0 && segment.height > 0)
})

function clamp(value: number, min: number, max: number) {
  if (max < min) {
    return min
  }

  return Math.min(Math.max(value, min), max)
}

function resolveCardStyle(rect: TutorialRect) {
  if (typeof window === 'undefined') {
    return {
      top: '50%',
      left: '50%',
      transform: 'translate(-50%, -50%)',
    }
  }

  const viewportWidth = window.innerWidth
  const viewportHeight = window.innerHeight
  const cardWidth = Math.min(tutorialCardWidth, viewportWidth - tutorialCardHorizontalPadding * 2)
  const maxLeft = viewportWidth - cardWidth - tutorialCardHorizontalPadding
  const maxTop = viewportHeight - tutorialCardHeight - tutorialCardVerticalPadding

  const placements = {
    right: {
      fits: rect.left + rect.width + tutorialCardGap + cardWidth + tutorialCardHorizontalPadding <= viewportWidth,
      style: {
        left: `${clamp(
          rect.left + rect.width + tutorialCardGap,
          tutorialCardHorizontalPadding,
          maxLeft,
        )}px`,
        top: `${clamp(rect.top, tutorialCardVerticalPadding, maxTop)}px`,
      },
      remainingSpace: viewportWidth - rect.left - rect.width,
    },
    left: {
      fits: rect.left - tutorialCardGap - cardWidth >= tutorialCardHorizontalPadding,
      style: {
        left: `${clamp(
          rect.left - cardWidth - tutorialCardGap,
          tutorialCardHorizontalPadding,
          maxLeft,
        )}px`,
        top: `${clamp(rect.top, tutorialCardVerticalPadding, maxTop)}px`,
      },
      remainingSpace: rect.left,
    },
    below: {
      fits: rect.top + rect.height + tutorialCardGap + tutorialCardHeight + tutorialCardVerticalPadding <= viewportHeight,
      style: {
        left: `${clamp(rect.left, tutorialCardHorizontalPadding, maxLeft)}px`,
        top: `${clamp(
          rect.top + rect.height + tutorialCardGap,
          tutorialCardVerticalPadding,
          maxTop,
        )}px`,
      },
      remainingSpace: viewportHeight - rect.top - rect.height,
    },
    above: {
      fits: rect.top - tutorialCardGap - tutorialCardHeight >= tutorialCardVerticalPadding,
      style: {
        left: `${clamp(rect.left, tutorialCardHorizontalPadding, maxLeft)}px`,
        top: `${clamp(
          rect.top - tutorialCardHeight - tutorialCardGap,
          tutorialCardVerticalPadding,
          maxTop,
        )}px`,
      },
      remainingSpace: rect.top,
    },
  }

  const automaticOrder = rect.height >= tutorialCardHeight * 1.55 || rect.width >= cardWidth * 0.92
    ? ['right', 'left', 'below', 'above'] as const
    : ['below', 'right', 'left', 'above'] as const
  const preferredOrder = content.value?.preferredPlacement
    ? [
        content.value.preferredPlacement,
        ...automaticOrder.filter((placement) => placement !== content.value?.preferredPlacement),
      ] as const
    : automaticOrder

  for (const placement of preferredOrder) {
    if (placements[placement].fits) {
      return placements[placement].style
    }
  }

  const fallbackPlacement = [...preferredOrder].sort(
    (leftPlacement, rightPlacement) =>
      placements[rightPlacement].remainingSpace - placements[leftPlacement].remainingSpace,
  )[0]

  return placements[fallbackPlacement].style
}

function measureTarget() {
  animationFrameId = 0

  if (!tutorialStore.active || content.value?.target === null || content.value === null) {
    targetRect.value = null
    return
  }

  const target = document.querySelector(`[data-tutorial-target="${content.value.target}"]`) as HTMLElement | null

  if (target === null) {
    targetRect.value = null
    return
  }

  const rect = target.getBoundingClientRect()
  targetRect.value = {
    top: rect.top,
    left: rect.left,
    width: rect.width,
    height: rect.height,
  }
}

function scheduleMeasure() {
  if (animationFrameId !== 0) {
    cancelAnimationFrame(animationFrameId)
  }

  animationFrameId = window.requestAnimationFrame(() => {
    measureTarget()
  })
}

function clearMeasureBurst() {
  for (const timeoutId of measureTimeoutIds) {
    window.clearTimeout(timeoutId)
  }

  measureTimeoutIds = []
}

function scheduleMeasureBurst() {
  clearMeasureBurst()

  for (const delay of remeasureDelays) {
    if (delay === 0) {
      scheduleMeasure()
      continue
    }

    measureTimeoutIds.push(window.setTimeout(() => {
      scheduleMeasure()
    }, delay))
  }
}

function handlePrimaryAction() {
  if (tutorialStore.step === 'intro') {
    tutorialStore.showModeChoice()
  }
}

function handleChooseAi() {
  tutorialStore.chooseAiBranch()
}

function handleChooseManual() {
  tutorialStore.chooseManualBranch()
}

function handleBackToChoice() {
  tutorialStore.showModeChoice()
}

function handleNextStep() {
  if (tutorialStore.step === 'ai-review') {
    tutorialStore.advanceAiReview()
  }
}

function handleFinish() {
  tutorialStore.finish()
}

watch(
  () => [tutorialStore.active, tutorialStore.step, content.value?.target] as const,
  async () => {
    await nextTick()
    scheduleMeasureBurst()
  },
  { immediate: true },
)

onMounted(() => {
  window.addEventListener('resize', scheduleMeasure)
  window.addEventListener('scroll', scheduleMeasure, true)
})

onBeforeUnmount(() => {
  window.removeEventListener('resize', scheduleMeasure)
  window.removeEventListener('scroll', scheduleMeasure, true)
  clearMeasureBurst()

  if (animationFrameId !== 0) {
    cancelAnimationFrame(animationFrameId)
  }
})
</script>

<template>
  <Teleport to="body">
    <div v-if="tutorialStore.active && content" class="beginner-tutorial" data-testid="beginner-tutorial-overlay">
      <div class="beginner-tutorial__backdrop" />

      <div
        v-for="(segment, index) in scrimSegments"
        :key="`${index}-${segment.top}-${segment.left}-${segment.width}-${segment.height}`"
        class="beginner-tutorial__scrim"
        :style="{
          top: `${segment.top}px`,
          left: `${segment.left}px`,
          width: `${segment.width}px`,
          height: `${segment.height}px`,
        }"
      />

      <div v-if="spotlightRect" class="beginner-tutorial__spotlight" :style="spotlightStyle" />

      <section class="beginner-tutorial__card" :style="cardStyle" @pointerdown.stop @click.stop>
        <p class="beginner-tutorial__badge">{{ content.badge }}</p>
        <h2>{{ content.title }}</h2>
        <p class="beginner-tutorial__description">{{ content.description }}</p>

        <div class="beginner-tutorial__actions">
          <template v-if="tutorialStore.step === 'intro'">
            <button data-testid="tutorial-next" type="button" class="beginner-tutorial__primary" @click="handlePrimaryAction">
              开始教程
            </button>
            <button type="button" class="beginner-tutorial__ghost" @click="handleFinish">结束教程</button>
          </template>

        <template v-else-if="tutorialStore.step === 'choose-mode'">
          <button type="button" class="beginner-tutorial__primary" @click="handleChooseAi">AI 创建</button>
          <button type="button" class="beginner-tutorial__ghost" @click="handleChooseManual">手动创建</button>
          <button type="button" class="beginner-tutorial__ghost" @click="handleFinish">结束教程</button>
        </template>

        <template v-else-if="content.showNextAction">
          <button type="button" class="beginner-tutorial__ghost" @click="handleBackToChoice">重新选择方式</button>
          <button type="button" class="beginner-tutorial__primary" @click="handleNextStep">下一步</button>
        </template>

        <template v-else-if="tutorialStore.step === 'created-success'">
          <button type="button" class="beginner-tutorial__primary" @click="handleFinish">结束教程</button>
        </template>

          <template v-else>
            <button type="button" class="beginner-tutorial__ghost" @click="handleBackToChoice">重新选择方式</button>
            <button type="button" class="beginner-tutorial__primary" @click="handleFinish">结束教程</button>
          </template>
        </div>
      </section>
    </div>
  </Teleport>
</template>

<style scoped>
.beginner-tutorial {
  position: fixed;
  inset: 0;
  z-index: 1300;
  pointer-events: none;
  isolation: isolate;
}

.beginner-tutorial__backdrop {
  position: absolute;
  inset: 0;
  z-index: 0;
  background:
    radial-gradient(circle at 15% 18%, rgba(0, 194, 255, 0.16), transparent 24%),
    radial-gradient(circle at 82% 74%, rgba(10, 102, 255, 0.18), transparent 24%),
    rgba(4, 10, 24, 0.18);
}

.beginner-tutorial__scrim {
  position: fixed;
  z-index: 1;
  background: rgba(4, 10, 24, 0.42);
  backdrop-filter: blur(8px);
  pointer-events: auto;
}

.beginner-tutorial__spotlight {
  position: fixed;
  z-index: 2;
  border-radius: 24px;
  border: 1px solid color-mix(in srgb, var(--accent-end) 64%, white 18%);
  box-shadow:
    0 0 0 1px color-mix(in srgb, var(--accent-start) 44%, transparent),
    0 0 0 6px color-mix(in srgb, var(--accent-start) 10%, transparent),
    0 24px 48px rgba(2, 6, 18, 0.2);
  background: rgba(255, 255, 255, 0.02);
  pointer-events: none;
}

.beginner-tutorial__card {
  position: fixed;
  z-index: 3;
  width: min(360px, calc(100vw - 32px));
  display: grid;
  gap: 14px;
  padding: 22px 22px 20px;
  border: 1px solid var(--dialog-surface-border);
  border-radius: 22px;
  background: var(--dialog-surface-glow), var(--dialog-surface-bg), var(--card-sheen);
  box-shadow: var(--dialog-surface-shadow);
  pointer-events: auto;
}

.beginner-tutorial__badge,
.beginner-tutorial__card h2,
.beginner-tutorial__description {
  margin: 0;
}

.beginner-tutorial__badge {
  font-size: 0.76rem;
  font-weight: 700;
  letter-spacing: 0.16em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.beginner-tutorial__card h2 {
  font-size: 1.48rem;
  line-height: 1.12;
}

.beginner-tutorial__description {
  color: var(--text-soft);
  line-height: 1.65;
}

.beginner-tutorial__actions {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
}

.beginner-tutorial__actions button {
  min-height: 42px;
  border-radius: 14px;
  padding: 0 16px;
  font: inherit;
  font-weight: 600;
  cursor: pointer;
}

.beginner-tutorial__primary {
  border: none;
  background: var(--gradient-primary);
  color: var(--text-inverse);
  box-shadow:
    inset 0 1px 0 color-mix(in srgb, white 18%, transparent),
    0 16px 30px color-mix(in srgb, var(--accent-start) 24%, transparent);
}

.beginner-tutorial__ghost {
  border: 1px solid var(--dialog-control-border);
  background: var(--dialog-control-bg);
  color: var(--text-main);
  box-shadow: var(--dialog-control-shadow);
}

@media (max-width: 760px) {
  .beginner-tutorial__card {
    width: min(100vw - 24px, 360px);
    padding: 20px 18px 18px;
  }

  .beginner-tutorial__actions {
    display: grid;
  }
}
</style>
