import { http, unwrapResponse } from '@/api/http'
import type { AiProjectDraft } from '@/types/projectAi'

export async function generateProjectDraft(prompt: string): Promise<AiProjectDraft> {
  return unwrapResponse<AiProjectDraft>(
    http.post('/api/ai/project-draft/generate', {
      prompt,
    }),
  )
}
