import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest'

import { createSmoothWheelController, normalizeSmoothWheelOptions, normalizeWheelDelta } from '@/utils/smoothWheel'

type MatchMediaResult = {
  addEventListener: ReturnType<typeof vi.fn>
  addListener: ReturnType<typeof vi.fn>
  dispatchEvent: ReturnType<typeof vi.fn>
  matches: boolean
  media: string
  onchange: null
  removeEventListener: ReturnType<typeof vi.fn>
  removeListener: ReturnType<typeof vi.fn>
}

function createMatchMediaResult(matches = false): MatchMediaResult {
  return {
    matches,
    media: '(prefers-reduced-motion: reduce)',
    onchange: null,
    addEventListener: vi.fn(),
    removeEventListener: vi.fn(),
    addListener: vi.fn(),
    removeListener: vi.fn(),
    dispatchEvent: vi.fn(),
  }
}

function createScrollableElement({
  clientHeight = 0,
  clientWidth = 0,
  scrollHeight = 0,
  scrollLeft = 0,
  scrollTop = 0,
  scrollWidth = 0,
}: {
  clientHeight?: number
  clientWidth?: number
  scrollHeight?: number
  scrollLeft?: number
  scrollTop?: number
  scrollWidth?: number
} = {}) {
  const element = document.createElement('div')

  Object.defineProperties(element, {
    clientHeight: {
      configurable: true,
      value: clientHeight,
    },
    clientWidth: {
      configurable: true,
      value: clientWidth,
    },
    scrollHeight: {
      configurable: true,
      value: scrollHeight,
    },
    scrollLeft: {
      configurable: true,
      value: scrollLeft,
      writable: true,
    },
    scrollTop: {
      configurable: true,
      value: scrollTop,
      writable: true,
    },
    scrollWidth: {
      configurable: true,
      value: scrollWidth,
    },
  })

  return element
}

describe('smoothWheel utils', () => {
  let animationFrames: FrameRequestCallback[] = []

  function flushAnimationFrames(limit = 24) {
    for (let index = 0; index < limit; index += 1) {
      if (animationFrames.length === 0) {
        return
      }

      const queue = [...animationFrames]
      animationFrames = []
      queue.forEach((callback) => callback(index * 16))
    }
  }

  beforeEach(() => {
    animationFrames = []

    vi.spyOn(window, 'requestAnimationFrame').mockImplementation((callback: FrameRequestCallback) => {
      animationFrames.push(callback)
      return animationFrames.length
    })

    vi.spyOn(window, 'cancelAnimationFrame').mockImplementation((frameId: number) => {
      const frameIndex = frameId - 1
      if (frameIndex >= 0 && frameIndex < animationFrames.length) {
        animationFrames.splice(frameIndex, 1, () => {})
      }
    })

    vi.stubGlobal(
      'matchMedia',
      vi.fn().mockImplementation(() => createMatchMediaResult(false)),
    )
  })

  afterEach(() => {
    vi.restoreAllMocks()
    vi.unstubAllGlobals()
  })

  it('normalizes line and page wheel deltas to pixel-based motion', () => {
    expect(normalizeWheelDelta({ deltaX: 1, deltaY: 2, deltaMode: 1 })).toEqual({
      x: 16,
      y: 32,
    })

    expect(normalizeWheelDelta({ deltaX: 0, deltaY: 1, deltaMode: 2 }, 500)).toEqual({
      x: 0,
      y: 450,
    })
  })

  it('uses a faster global default response so smooth scrolling feels more immediate', () => {
    expect(normalizeSmoothWheelOptions()).toMatchObject({
      axis: 'vertical',
      easing: 0.24,
      multiplier: 1.18,
      wheelBehavior: 'smooth',
    })
  })

  it('smooths vertical scrolling only while the container can still move', () => {
    const element = createScrollableElement({
      clientHeight: 200,
      scrollHeight: 600,
      scrollTop: 0,
    })

    const controller = createSmoothWheelController(element, { axis: 'vertical' })
    controller.mount()

    const scrollEvent = new window.WheelEvent('wheel', {
      cancelable: true,
      deltaY: 48,
    })

    element.dispatchEvent(scrollEvent)
    flushAnimationFrames()

    expect(scrollEvent.defaultPrevented).toBe(true)
    expect(element.scrollTop).toBeGreaterThan(0)

    element.scrollTop = 400

    const boundaryEvent = new window.WheelEvent('wheel', {
      cancelable: true,
      deltaY: 48,
    })

    element.dispatchEvent(boundaryEvent)

    expect(boundaryEvent.defaultPrevented).toBe(false)
    controller.destroy()
  })

  it('maps vertical wheel input onto horizontal scroll surfaces', () => {
    const element = createScrollableElement({
      clientWidth: 320,
      scrollLeft: 0,
      scrollWidth: 1080,
    })

    const controller = createSmoothWheelController(element, { axis: 'horizontal' })
    controller.mount()

    const scrollEvent = new window.WheelEvent('wheel', {
      cancelable: true,
      deltaY: 72,
    })

    element.dispatchEvent(scrollEvent)
    flushAnimationFrames()

    expect(scrollEvent.defaultPrevented).toBe(true)
    expect(element.scrollLeft).toBeGreaterThan(0)
    controller.destroy()
  })

  it('can block wheel-driven horizontal movement while still letting vertical wheel intent bubble outward', () => {
    const element = createScrollableElement({
      clientWidth: 320,
      scrollLeft: 0,
      scrollWidth: 1080,
    })

    const controller = createSmoothWheelController(element, {
      axis: 'horizontal',
      wheelBehavior: 'block',
    })
    controller.mount()

    const horizontalEvent = new window.WheelEvent('wheel', {
      cancelable: true,
      deltaX: 72,
    })

    element.dispatchEvent(horizontalEvent)
    flushAnimationFrames()

    expect(horizontalEvent.defaultPrevented).toBe(true)
    expect(element.scrollLeft).toBe(0)

    const verticalEvent = new window.WheelEvent('wheel', {
      cancelable: true,
      deltaY: 72,
    })

    element.dispatchEvent(verticalEvent)
    flushAnimationFrames()

    expect(verticalEvent.defaultPrevented).toBe(false)
    expect(element.scrollLeft).toBe(0)
    controller.destroy()
  })

  it('respects reduced motion by applying scroll updates immediately', () => {
    vi.stubGlobal(
      'matchMedia',
      vi.fn().mockImplementation(() => createMatchMediaResult(true)),
    )

    const element = createScrollableElement({
      clientWidth: 300,
      scrollLeft: 20,
      scrollWidth: 900,
    })

    const controller = createSmoothWheelController(element, { axis: 'horizontal' })
    controller.mount()

    const scrollEvent = new window.WheelEvent('wheel', {
      cancelable: true,
      deltaY: 60,
    })

    element.dispatchEvent(scrollEvent)

    expect(scrollEvent.defaultPrevented).toBe(true)
    expect(element.scrollLeft).toBeCloseTo(90.8, 5)
    expect(animationFrames).toHaveLength(0)
    controller.destroy()
  })

  it('can use a stronger smooth response without jumping straight to the final vertical position', () => {
    const baselineElement = createScrollableElement({
      clientHeight: 200,
      scrollHeight: 600,
      scrollTop: 0,
    })
    const tunedElement = createScrollableElement({
      clientHeight: 200,
      scrollHeight: 600,
      scrollTop: 0,
    })

    const baselineController = createSmoothWheelController(baselineElement, {
      axis: 'vertical',
    })
    const tunedController = createSmoothWheelController(tunedElement, {
      axis: 'vertical',
      easing: 0.36,
      multiplier: 1.45,
    })
    baselineController.mount()
    tunedController.mount()

    const baselineEvent = new window.WheelEvent('wheel', {
      cancelable: true,
      deltaY: 48,
    })
    const tunedEvent = new window.WheelEvent('wheel', {
      cancelable: true,
      deltaY: 48,
    })

    baselineElement.dispatchEvent(baselineEvent)
    tunedElement.dispatchEvent(tunedEvent)
    flushAnimationFrames(1)

    expect(baselineEvent.defaultPrevented).toBe(true)
    expect(tunedEvent.defaultPrevented).toBe(true)
    expect(tunedElement.scrollTop).toBeGreaterThan(baselineElement.scrollTop)
    expect(tunedElement.scrollTop).toBeLessThan(69.6)
    expect(animationFrames.length).toBeGreaterThan(0)
    baselineController.destroy()
    tunedController.destroy()
  })

  it('can hand off vertical wheel motion to browser-native smooth scrolling without scheduling js animation frames', () => {
    const element = createScrollableElement({
      clientHeight: 200,
      scrollHeight: 600,
      scrollTop: 0,
    })
    const scrollTo = vi.fn().mockImplementation((options: ScrollToOptions) => {
      if (typeof options.top === 'number') {
        element.scrollTop = options.top
      }
    })

    Object.defineProperty(element, 'scrollTo', {
      configurable: true,
      value: scrollTo,
    })

    const controller = createSmoothWheelController(element, {
      axis: 'vertical',
      multiplier: 1.3,
      wheelBehavior: 'native',
    })
    controller.mount()

    const scrollEvent = new window.WheelEvent('wheel', {
      cancelable: true,
      deltaY: 48,
    })

    element.dispatchEvent(scrollEvent)

    expect(scrollEvent.defaultPrevented).toBe(true)
    expect(scrollTo).toHaveBeenCalledTimes(1)
    expect(scrollTo.mock.calls[0]?.[0]).toMatchObject({
      behavior: 'smooth',
    })
    expect((scrollTo.mock.calls[0]?.[0] as ScrollToOptions | undefined)?.top).toBeCloseTo(62.4, 5)
    expect(element.scrollTop).toBeCloseTo(62.4, 5)
    expect(animationFrames).toHaveLength(0)
    controller.destroy()
  })
})
