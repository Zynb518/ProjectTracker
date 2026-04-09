import type { Directive } from 'vue'

import {
  createSmoothWheelController,
  type SmoothWheelController,
  type SmoothWheelOptions,
} from '@/utils/smoothWheel'

const controllerKey = Symbol('smooth-wheel-controller')

type SmoothWheelElement = HTMLElement & {
  [controllerKey]?: SmoothWheelController
}

export const smoothWheelDirective: Directive<HTMLElement, SmoothWheelOptions | undefined> = {
  mounted(element, binding) {
    const smoothWheelElement = element as SmoothWheelElement
    const controller = createSmoothWheelController(element, binding.value)

    controller.mount()
    smoothWheelElement[controllerKey] = controller
  },
  updated(element, binding) {
    const controller = (element as SmoothWheelElement)[controllerKey]
    controller?.updateOptions(binding.value)
  },
  beforeUnmount(element) {
    const smoothWheelElement = element as SmoothWheelElement

    smoothWheelElement[controllerKey]?.destroy()
    delete smoothWheelElement[controllerKey]
  },
}
