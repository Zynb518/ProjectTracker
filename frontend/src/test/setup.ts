import { config } from '@vue/test-utils'

config.global.stubs = {
  transition: false,
  'transition-group': false,
}

config.global.directives = {
  'smooth-wheel': {
    mounted() {},
    updated() {},
    beforeUnmount() {},
  },
}
