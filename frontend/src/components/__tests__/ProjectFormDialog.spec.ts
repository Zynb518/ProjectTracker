import { mount } from '@vue/test-utils'
import { readFileSync } from 'node:fs'
import { resolve } from 'node:path'

import ProjectFormDialog from '@/components/projects/ProjectFormDialog.vue'

describe('ProjectFormDialog', () => {
  it('wraps the dialog in an appear transition for smoother entrance motion', () => {
    const wrapper = mount(ProjectFormDialog, {
      props: {
        modelValue: true,
        mode: 'create',
        initialValue: {
          name: '',
          description: '',
          planned_start_date: '',
          planned_end_date: '',
        },
      },
      global: {
        stubs: {
          transition: true,
        },
      },
    })

    const transition = wrapper.get('transition-stub')

    expect(transition.attributes('name')).toBe('project-dialog-fade')
    expect(transition.attributes('appear')).toBe('true')
  })

  it('renders a dedicated glow layer for the dialog entrance effect', () => {
    const wrapper = mount(ProjectFormDialog, {
      props: {
        modelValue: true,
        mode: 'create',
        initialValue: {
          name: '',
          description: '',
          planned_start_date: '',
          planned_end_date: '',
        },
      },
    })

    expect(wrapper.get('.project-dialog__glow').attributes('aria-hidden')).toBe('true')
  })

  it('applies motion origin variables when a trigger position is provided', () => {
    const wrapper = mount(ProjectFormDialog, {
      props: {
        modelValue: true,
        mode: 'create',
        initialValue: {
          name: '',
          description: '',
          planned_start_date: '',
          planned_end_date: '',
        },
        motionOrigin: {
          x: 94,
          y: 58,
          translateX: -418,
          translateY: -326,
        },
      },
    })

    const backdropStyle = wrapper.get('.project-dialog__backdrop').attributes('style')

    expect(backdropStyle).toContain('--project-dialog-origin-x: 94px;')
    expect(backdropStyle).toContain('--project-dialog-origin-y: 58px;')
    expect(backdropStyle).toContain('--project-dialog-translate-x: -418px;')
    expect(backdropStyle).toContain('--project-dialog-translate-y: -326px;')
  })

  it('uses a slower and softer entrance motion profile for the dialog transition', () => {
    const source = readFileSync(
      resolve(process.cwd(), 'src/components/projects/ProjectFormDialog.vue'),
      'utf8',
    )

    expect(source).toContain('opacity 420ms ease-out')
    expect(source).toContain('transform 460ms cubic-bezier(0.16, 1, 0.3, 1)')
    expect(source).toContain('scale(0.72)')
    expect(source).toContain('rotateX(4deg)')
    expect(source).toContain('filter: blur(6px)')
  })
})
