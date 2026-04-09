import { readFileSync } from 'node:fs'
import { resolve } from 'node:path'

import { describe, expect, it } from 'vitest'

describe('subtask reopen backend sql', () => {
  it('casts reopen status parameters to integer so CASE does not fall back to text', () => {
    const source = readFileSync(
      resolve(process.cwd(), '../src/modules/task/repository/TaskRepository.cpp'),
      'utf8',
    )

    expect(source).toContain('WHEN $2::integer = $4::integer THEN $4::integer')
    expect(source).toContain(
      "WHEN (NOW() AT TIME ZONE 'Asia/Shanghai')::date > st.planned_end_date THEN $4::integer",
    )
    expect(source).toContain('ELSE $2::integer')
    expect(source).toContain('st.status = $5::integer')
  })
})
