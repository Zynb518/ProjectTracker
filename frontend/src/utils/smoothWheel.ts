export type SmoothWheelAxis = 'horizontal' | 'vertical'
export type SmoothWheelBehavior = 'block' | 'smooth'

export type SmoothWheelOptions = {
  axis?: SmoothWheelAxis
  easing?: number
  multiplier?: number
  wheelBehavior?: SmoothWheelBehavior
}

export type NormalizedSmoothWheelOptions = {
  axis: SmoothWheelAxis
  easing: number
  multiplier: number
  wheelBehavior: SmoothWheelBehavior
}

export type SmoothWheelController = {
  destroy: () => void
  mount: () => void
  updateOptions: (options?: SmoothWheelOptions) => void
}

const PIXELS_PER_LINE = 16
const PAGE_SIZE_RATIO = 0.9
const MIN_SCROLL_DELTA = 0.1
const MIN_ANIMATION_DISTANCE = 0.5
const DEFAULT_OPTIONS: NormalizedSmoothWheelOptions = {
  axis: 'vertical',
  easing: 0.24,
  multiplier: 1.18,
  wheelBehavior: 'smooth',
}

function clamp(value: number, min: number, max: number) {
  return Math.min(Math.max(value, min), max)
}

function getViewportPageSize(element: HTMLElement, axis: SmoothWheelAxis) {
  if (axis === 'horizontal') {
    return element.clientWidth || window.innerWidth || 0
  }

  return element.clientHeight || window.innerHeight || 0
}

function getScrollPosition(element: HTMLElement, axis: SmoothWheelAxis) {
  return axis === 'horizontal' ? element.scrollLeft : element.scrollTop
}

function setScrollPosition(element: HTMLElement, axis: SmoothWheelAxis, value: number) {
  if (axis === 'horizontal') {
    element.scrollLeft = value
    return
  }

  element.scrollTop = value
}

function getMaxScrollPosition(element: HTMLElement, axis: SmoothWheelAxis) {
  if (axis === 'horizontal') {
    return Math.max(element.scrollWidth - element.clientWidth, 0)
  }

  return Math.max(element.scrollHeight - element.clientHeight, 0)
}

function canConsumeDelta(currentPosition: number, maxPosition: number, delta: number) {
  if (maxPosition <= 0 || Math.abs(delta) < MIN_SCROLL_DELTA) {
    return false
  }

  if (delta > 0) {
    return currentPosition < maxPosition - MIN_ANIMATION_DISTANCE
  }

  return currentPosition > MIN_ANIMATION_DISTANCE
}

function prefersReducedMotion() {
  return typeof window !== 'undefined'
    && typeof window.matchMedia === 'function'
    && window.matchMedia('(prefers-reduced-motion: reduce)').matches
}

export function normalizeSmoothWheelOptions(options?: SmoothWheelOptions): NormalizedSmoothWheelOptions {
  return {
    axis: options?.axis ?? DEFAULT_OPTIONS.axis,
    easing: clamp(options?.easing ?? DEFAULT_OPTIONS.easing, 0.08, 0.45),
    multiplier: clamp(options?.multiplier ?? DEFAULT_OPTIONS.multiplier, 0.4, 2.6),
    wheelBehavior: options?.wheelBehavior ?? DEFAULT_OPTIONS.wheelBehavior,
  }
}

export function normalizeWheelDelta(
  event: Pick<WheelEvent, 'deltaMode' | 'deltaX' | 'deltaY'>,
  pageSize = 0,
) {
  const factor
    = event.deltaMode === 1
      ? PIXELS_PER_LINE
      : event.deltaMode === 2
        ? Math.max(pageSize * PAGE_SIZE_RATIO, 0)
        : 1

  return {
    x: event.deltaX * factor,
    y: event.deltaY * factor,
  }
}

function resolveScrollDelta(
  element: HTMLElement,
  event: Pick<WheelEvent, 'deltaMode' | 'deltaX' | 'deltaY'>,
  axis: SmoothWheelAxis,
) {
  const { x, y } = normalizeWheelDelta(event, getViewportPageSize(element, axis))

  if (axis === 'horizontal') {
    return Math.abs(x) > Math.abs(y) ? x : y
  }

  return y
}

function shouldBlockAxisWheel(
  element: HTMLElement,
  event: Pick<WheelEvent, 'deltaMode' | 'deltaX' | 'deltaY'>,
  axis: SmoothWheelAxis,
) {
  const { x, y } = normalizeWheelDelta(event, getViewportPageSize(element, axis))
  const primary = axis === 'horizontal' ? x : y
  const secondary = axis === 'horizontal' ? y : x

  return Math.abs(primary) >= MIN_SCROLL_DELTA && Math.abs(primary) > Math.abs(secondary)
}

export function createSmoothWheelController(
  element: HTMLElement,
  initialOptions?: SmoothWheelOptions,
): SmoothWheelController {
  let options = normalizeSmoothWheelOptions(initialOptions)
  let frameId: number | null = null
  let targetPosition = getScrollPosition(element, options.axis)

  function stopAnimation() {
    if (frameId !== null) {
      window.cancelAnimationFrame(frameId)
      frameId = null
    }
  }

  function syncTarget() {
    targetPosition = clamp(
      getScrollPosition(element, options.axis),
      0,
      getMaxScrollPosition(element, options.axis),
    )
  }

  function step() {
    frameId = null

    const currentPosition = getScrollPosition(element, options.axis)
    const distance = targetPosition - currentPosition

    if (Math.abs(distance) <= MIN_ANIMATION_DISTANCE) {
      setScrollPosition(element, options.axis, targetPosition)
      return
    }

    setScrollPosition(element, options.axis, currentPosition + distance * options.easing)
    frameId = window.requestAnimationFrame(step)
  }

  function handleWheel(event: WheelEvent) {
    if (event.defaultPrevented || event.ctrlKey) {
      return
    }

    if (options.wheelBehavior === 'block') {
      if (shouldBlockAxisWheel(element, event, options.axis)) {
        event.preventDefault()
      }

      return
    }

    const delta = resolveScrollDelta(element, event, options.axis)

    if (Math.abs(delta) < MIN_SCROLL_DELTA) {
      return
    }

    const currentPosition = frameId === null
      ? getScrollPosition(element, options.axis)
      : targetPosition
    const maxPosition = getMaxScrollPosition(element, options.axis)

    if (!canConsumeDelta(currentPosition, maxPosition, delta)) {
      return
    }

    const nextTarget = clamp(currentPosition + delta * options.multiplier, 0, maxPosition)

    if (Math.abs(nextTarget - currentPosition) < MIN_SCROLL_DELTA) {
      return
    }

    event.preventDefault()
    targetPosition = nextTarget

    if (prefersReducedMotion()) {
      stopAnimation()
      setScrollPosition(element, options.axis, targetPosition)
      return
    }

    if (frameId === null) {
      frameId = window.requestAnimationFrame(step)
    }
  }

  return {
    mount() {
      syncTarget()
      element.addEventListener('wheel', handleWheel, { passive: false })
    },
    updateOptions(nextOptions) {
      options = normalizeSmoothWheelOptions(nextOptions)
      syncTarget()

      if (prefersReducedMotion()) {
        stopAnimation()
        setScrollPosition(element, options.axis, targetPosition)
      }
    },
    destroy() {
      stopAnimation()
      element.removeEventListener('wheel', handleWheel)
    },
  }
}
