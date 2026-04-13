<script setup lang="ts">
import { computed, ref } from 'vue'

const emit = defineEmits<{
  'update:projectKeyword': [value: string]
  'apply-project-keyword': [value: string]
  'update:status': [value: string]
  submit: []
}>()

const statusOptions = [
  { value: '', label: '全部状态', tone: 'all' },
  { value: '1', label: '未开始', tone: 'pending' },
  { value: '2', label: '进行中', tone: 'active' },
  { value: '3', label: '已完成', tone: 'done' },
  { value: '4', label: '已延期', tone: 'delayed' },
] as const

const props = defineProps<{
  projectKeyword: string
  projectCandidates: string[]
  status: string
}>()

const isCandidateOpen = ref(false)

const hasProjectCandidates = computed(() => props.projectCandidates.length > 0)
const normalizedProjectKeyword = computed(() => normalizeProjectKeyword(props.projectKeyword))
const projectCandidateItems = computed(() =>
  props.projectCandidates.map((projectName) => {
    const keyword = normalizedProjectKeyword.value
    const normalizedName = normalizeProjectKeyword(projectName)
    const matchStart = keyword === '' ? -1 : projectName.toLocaleLowerCase().indexOf(keyword.toLocaleLowerCase())
    const matchEnd = matchStart === -1 ? -1 : matchStart + keyword.length

    return {
      name: projectName,
      before: matchStart === -1 ? projectName : projectName.slice(0, matchStart),
      match: matchStart === -1 ? '' : projectName.slice(matchStart, matchEnd),
      after: matchStart === -1 ? '' : projectName.slice(matchEnd),
      isMatched: matchStart !== -1,
      isSelected: keyword !== '' && normalizedName.toLocaleLowerCase() === keyword.toLocaleLowerCase(),
    }
  }),
)

function normalizeProjectKeyword(value: string) {
  return value.trim()
}

function handleProjectKeywordInput(event: Event) {
  emit('update:projectKeyword', (event.target as HTMLInputElement).value)
  isCandidateOpen.value = true
}

function handleProjectKeywordFocus() {
  isCandidateOpen.value = true
}

function handleProjectKeywordSubmit(event: KeyboardEvent) {
  const value = normalizeProjectKeyword((event.currentTarget as HTMLInputElement).value)
  emit('update:projectKeyword', value)
  emit('apply-project-keyword', value)
  isCandidateOpen.value = false
}

function handleProjectKeywordBlur(event: FocusEvent) {
  const value = normalizeProjectKeyword((event.currentTarget as HTMLInputElement).value)
  if (value === '') {
    emit('update:projectKeyword', value)
    emit('apply-project-keyword', value)
  }

  isCandidateOpen.value = false
}

function selectProjectCandidate(projectName: string) {
  emit('update:projectKeyword', projectName)
  emit('apply-project-keyword', projectName)
  isCandidateOpen.value = false
}

function handleStatusSelect(value: string) {
  emit('update:status', value)
  emit('submit')
}
</script>

<template>
  <section
    :class="['my-task-filters', { 'my-task-filters--candidate-open': isCandidateOpen && hasProjectCandidates }]"
  >
    <label class="my-task-filters__field">
      <span>项目名称</span>
      <div class="my-task-filters__combobox">
        <span class="my-task-filters__input-icon" aria-hidden="true">
          <svg viewBox="0 0 20 20" fill="none">
            <path
              d="M8.75 3.75a5 5 0 1 0 0 10 5 5 0 0 0 0-10ZM13.1 12.02l3.15 3.15"
              stroke="currentColor"
              stroke-linecap="round"
              stroke-linejoin="round"
              stroke-width="1.7"
            />
          </svg>
        </span>
        <input
          :value="projectKeyword"
          aria-autocomplete="list"
          :aria-expanded="isCandidateOpen && hasProjectCandidates"
          aria-label="项目名称筛选"
          class="my-task-filters__input"
          placeholder="输入项目名称后按回车，或直接选择候选"
          role="combobox"
          type="text"
          @blur="handleProjectKeywordBlur"
          @focus="handleProjectKeywordFocus"
          @input="handleProjectKeywordInput"
          @keydown.enter.prevent="handleProjectKeywordSubmit"
        />
        <span class="my-task-filters__input-affordance" aria-hidden="true">
          <svg viewBox="0 0 20 20" fill="none">
            <path
              d="m5.5 7.75 4.5 4.5 4.5-4.5"
              stroke="currentColor"
              stroke-linecap="round"
              stroke-linejoin="round"
              stroke-width="1.7"
            />
          </svg>
        </span>

        <div
          v-if="isCandidateOpen && hasProjectCandidates"
          class="my-task-filters__candidate-panel"
          role="listbox"
          aria-label="项目候选"
        >
          <button
            v-for="candidate in projectCandidateItems"
            :key="candidate.name"
            :class="['my-task-filters__candidate', { 'is-selected': candidate.isSelected }]"
            :data-testid="`my-task-project-candidate-${candidate.name}`"
            type="button"
            @mousedown.prevent="selectProjectCandidate(candidate.name)"
          >
            <span class="my-task-filters__candidate-label">
              <template v-if="candidate.isMatched">
                <span>{{ candidate.before }}</span>
                <mark class="my-task-filters__candidate-highlight">{{ candidate.match }}</mark>
                <span>{{ candidate.after }}</span>
              </template>
              <template v-else>
                {{ candidate.name }}
              </template>
            </span>
          </button>
        </div>
      </div>
    </label>

    <div class="my-task-filters__field my-task-filters__field--status">
      <span>任务状态</span>
      <div
        data-testid="my-tasks-status"
        class="my-task-filters__status-group"
        role="group"
        aria-label="任务状态"
      >
        <button
          v-for="option in statusOptions"
          :key="option.value || 'all'"
          :aria-pressed="status === option.value"
          :class="[
            'my-task-filters__status-pill',
            `my-task-filters__status-pill--${option.tone}`,
            { 'is-active': status === option.value },
          ]"
          :data-testid="`my-task-filter-status-${option.value || 'all'}`"
          type="button"
          @click="handleStatusSelect(option.value)"
        >
          {{ option.label }}
        </button>
      </div>
    </div>
  </section>
</template>

<style scoped>
.my-task-filters {
  display: grid;
  grid-template-columns: minmax(0, 360px) minmax(0, 1fr);
  align-items: start;
  gap: 14px;
  padding: 18px;
  position: relative;
  z-index: 0;
  border: 1px solid var(--border-soft);
  border-radius: 18px;
  background: var(--glass-bg);
  box-shadow: var(--shadow-panel);
  backdrop-filter: var(--backdrop-blur);
}

.my-task-filters--candidate-open {
  z-index: 12;
}

.my-task-filters__field {
  display: grid;
  gap: 8px;
}

.my-task-filters__field--status {
  min-width: 0;
}

.my-task-filters__field > span {
  font-size: 0.78rem;
  letter-spacing: 0.12em;
  text-transform: uppercase;
  color: var(--text-soft);
}

.my-task-filters__combobox {
  position: relative;
}

.my-task-filters__input-icon,
.my-task-filters__input-affordance {
  position: absolute;
  top: 50%;
  z-index: 1;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 18px;
  height: 18px;
  color: color-mix(in srgb, var(--text-muted) 88%, transparent);
  transform: translateY(-50%);
  transition:
    color 180ms ease-out,
    transform 180ms ease-out,
    opacity 180ms ease-out;
  pointer-events: none;
}

.my-task-filters__input-icon {
  left: 14px;
}

.my-task-filters__input-affordance {
  right: 14px;
  opacity: 0.84;
}

.my-task-filters__input-icon svg,
.my-task-filters__input-affordance svg {
  width: 100%;
  height: 100%;
}

.my-task-filters__input {
  width: 100%;
  min-height: 46px;
  border: 1px solid var(--border-soft);
  border-radius: 12px;
  padding: 0 42px 0 42px;
  background: color-mix(in srgb, var(--panel-bg) 92%, transparent);
  color: var(--text-main);
  font: inherit;
  box-shadow: inset 0 1px 0 color-mix(in srgb, #ffffff 42%, transparent);
  transition:
    border-color 180ms ease-out,
    background-color 180ms ease-out,
    box-shadow 180ms ease-out;
}

.my-task-filters__input::placeholder {
  color: color-mix(in srgb, var(--text-muted) 88%, transparent);
}

.my-task-filters__combobox:focus-within .my-task-filters__input {
  border-color: color-mix(in srgb, var(--accent-line) 56%, transparent);
  background: color-mix(in srgb, var(--glass-bg-strong) 96%, #ffffff 4%);
  box-shadow:
    inset 0 1px 0 color-mix(in srgb, #ffffff 54%, transparent),
    0 0 0 4px color-mix(in srgb, var(--accent-start) 8%, transparent),
    0 18px 34px color-mix(in srgb, var(--accent-end) 10%, transparent);
}

.my-task-filters__combobox:focus-within .my-task-filters__input-icon,
.my-task-filters__combobox:focus-within .my-task-filters__input-affordance {
  color: color-mix(in srgb, var(--accent-start) 78%, var(--text-soft));
}

.my-task-filters__combobox:focus-within .my-task-filters__input-affordance {
  transform: translateY(-50%) rotate(180deg);
}

.my-task-filters__candidate-panel {
  position: absolute;
  top: calc(100% + 10px);
  left: 0;
  right: 0;
  z-index: 5;
  display: grid;
  gap: 6px;
  padding: 6px 0;
  overflow: hidden;
  border: 1px solid color-mix(in srgb, var(--accent-line) 32%, var(--border-soft));
  border-radius: 16px;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--glass-bg) 96%, #ffffff 4%), color-mix(in srgb, var(--panel-bg) 96%, transparent)),
    radial-gradient(circle at top right, color-mix(in srgb, var(--accent-end) 16%, transparent), transparent 44%),
    radial-gradient(circle at 0% 100%, color-mix(in srgb, var(--accent-start) 10%, transparent), transparent 38%);
  box-shadow:
    0 24px 40px color-mix(in srgb, #0f172a 12%, transparent),
    inset 0 1px 0 color-mix(in srgb, #ffffff 42%, transparent);
  backdrop-filter: var(--backdrop-blur);
  animation: my-task-filters-candidate-rise 180ms ease-out;
}

.my-task-filters__candidate {
  display: block;
  width: 100%;
  min-height: 44px;
  padding: 10px 16px;
  border-top: 1px solid color-mix(in srgb, var(--border-soft) 72%, transparent);
  border-right: none;
  border-bottom: 1px solid color-mix(in srgb, var(--border-soft) 72%, transparent);
  border-left: none;
  border-radius: 0;
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--panel-bg) 92%, #ffffff 8%), color-mix(in srgb, var(--panel-bg) 76%, transparent));
  color: var(--text-main);
  text-align: left;
  cursor: pointer;
  transition:
    transform 180ms ease-out,
    border-color 180ms ease-out,
    background-color 180ms ease-out,
    box-shadow 180ms ease-out;
}

.my-task-filters__candidate:hover {
  border-color: color-mix(in srgb, var(--accent-line) 40%, transparent);
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--accent-start) 12%, #ffffff), color-mix(in srgb, var(--panel-bg) 74%, transparent));
  box-shadow:
    0 14px 26px color-mix(in srgb, var(--accent-end) 10%, transparent),
    inset 0 1px 0 color-mix(in srgb, #ffffff 28%, transparent);
  transform: translateY(-1px);
}

.my-task-filters__candidate:first-child {
  border-top-color: transparent;
}

.my-task-filters__candidate.is-selected {
  border-color: color-mix(in srgb, var(--accent-line) 58%, transparent);
  background:
    linear-gradient(180deg, color-mix(in srgb, var(--accent-start) 14%, #ffffff), color-mix(in srgb, var(--panel-bg) 78%, transparent));
  box-shadow:
    0 16px 28px color-mix(in srgb, var(--accent-end) 11%, transparent),
    inset 0 1px 0 color-mix(in srgb, #ffffff 36%, transparent);
}

.my-task-filters__candidate-label {
  display: block;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  font-size: 0.9rem;
  font-weight: 700;
}

.my-task-filters__candidate-highlight {
  display: inline;
  margin: 0 1px;
  padding: 0 4px;
  border-radius: 6px;
  background: color-mix(in srgb, var(--accent-start) 14%, #ffffff);
  color: color-mix(in srgb, var(--accent-start) 88%, var(--text-main));
  font-weight: 800;
}

.my-task-filters__status-group {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  padding: 0;
  border: 1px solid transparent;
  border-radius: 0;
  background: transparent;
  box-shadow: none;
}

.my-task-filters__status-pill {
  min-height: 42px;
  padding: 0 14px;
  border: 1px solid transparent;
  border-radius: 999px;
  background: var(--dialog-control-bg);
  color: var(--text-soft);
  font: inherit;
  font-size: 0.84rem;
  font-weight: 700;
  cursor: pointer;
  transition:
    transform 180ms ease-out,
    box-shadow 180ms ease-out,
    border-color 180ms ease-out,
    background-color 180ms ease-out,
    color 180ms ease-out;
}

.my-task-filters__status-pill:hover {
  transform: translateY(-1px);
  box-shadow:
    inset 0 1px 0 color-mix(in srgb, #ffffff 16%, transparent),
    0 8px 16px color-mix(in srgb, var(--accent-end) 10%, transparent);
}

.my-task-filters__status-pill--all {
  border-color: var(--work-status-unknown-border);
  background: var(--work-status-unknown-bg);
  color: var(--work-status-unknown-color);
}

.my-task-filters__status-pill--pending {
  border-color: var(--work-status-pending-border);
  background: var(--work-status-pending-bg);
  color: var(--work-status-pending-color);
}

.my-task-filters__status-pill--active {
  border-color: var(--work-status-active-border);
  background: var(--work-status-active-bg);
  color: var(--work-status-active-color);
}

.my-task-filters__status-pill--done {
  border-color: var(--work-status-done-border);
  background: var(--work-status-done-bg);
  color: var(--work-status-done-color);
}

.my-task-filters__status-pill--delayed {
  border-color: var(--work-status-delayed-border);
  background: var(--work-status-delayed-bg);
  color: var(--work-status-delayed-color);
}

.my-task-filters__status-pill.is-active {
  transform: translateY(-1px);
  box-shadow:
    inset 0 1px 0 color-mix(in srgb, #ffffff 18%, transparent),
    0 10px 20px color-mix(in srgb, var(--accent-end) 14%, transparent);
}

@keyframes my-task-filters-candidate-rise {
  from {
    opacity: 0;
    transform: translateY(-4px);
  }

  to {
    opacity: 1;
    transform: translateY(0);
  }
}

@media (max-width: 820px) {
  .my-task-filters {
    grid-template-columns: 1fr;
  }
}
</style>
