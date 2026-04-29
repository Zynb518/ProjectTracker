import { defineStore } from 'pinia'

export type BeginnerTutorialStep =
  | 'idle'
  | 'intro'
  | 'choose-mode'
  | 'manual-form'
  | 'ai-prompt'
  | 'ai-review'
  | 'ai-submit'
  | 'created-success'

export type BeginnerTutorialMode = 'manual' | 'ai' | null
export type BeginnerTutorialDialogRequest = 'manual' | 'ai' | null

export const useBeginnerTutorialStore = defineStore('beginner-tutorial', {
  state: () => ({
    active: false,
    step: 'idle' as BeginnerTutorialStep,
    mode: null as BeginnerTutorialMode,
    dialogRequest: null as BeginnerTutorialDialogRequest,
    createdProjectId: null as number | null,
  }),
  actions: {
    start() {
      this.active = true
      this.step = 'intro'
      this.mode = null
      this.dialogRequest = null
      this.createdProjectId = null
    },
    showModeChoice() {
      if (!this.active) {
        return
      }

      this.step = 'choose-mode'
      this.mode = null
      this.dialogRequest = null
    },
    chooseManualBranch() {
      if (!this.active) {
        this.start()
      }

      this.mode = 'manual'
      this.step = 'manual-form'
      this.dialogRequest = 'manual'
    },
    chooseAiBranch() {
      if (!this.active) {
        this.start()
      }

      this.mode = 'ai'
      this.step = 'ai-prompt'
      this.dialogRequest = 'ai'
    },
    consumeDialogRequest(kind: Exclude<BeginnerTutorialDialogRequest, null>) {
      if (this.dialogRequest === kind) {
        this.dialogRequest = null
      }
    },
    handleDialogClosed(kind: Exclude<BeginnerTutorialMode, null>) {
      if (!this.active || this.mode !== kind || this.createdProjectId !== null) {
        return
      }

      this.showModeChoice()
    },
    markAiDraftGenerated() {
      if (!this.active || this.mode !== 'ai' || this.step !== 'ai-prompt') {
        return
      }

      this.step = 'ai-review'
    },
    advanceAiReview() {
      if (!this.active || this.mode !== 'ai' || this.step !== 'ai-review') {
        return
      }

      this.step = 'ai-submit'
    },
    markCreated(projectId: number) {
      if (!this.active) {
        return
      }

      this.createdProjectId = projectId
      this.step = 'created-success'
    },
    finish() {
      this.active = false
      this.step = 'idle'
      this.mode = null
      this.dialogRequest = null
      this.createdProjectId = null
    },
  },
})
