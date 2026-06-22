<script setup lang="ts">
import { computed, onMounted, ref } from 'vue'
import { useRouter } from 'vue-router'
import { listProjects } from '@/api/projects'
import { getErrorMessage } from '@/api/http'
import { useNotificationStore } from '@/stores/notifications'
import type { ProjectListItem } from '@/types/project'

const router = useRouter()
const notificationStore = useNotificationStore()

const loading = ref(true)
const tickets = ref<ProjectListItem[]>([])

async function fetchTickets() {
  try {
    loading.value = true
    const response = await listProjects({ page: 1, page_size: 1000 })
    tickets.value = response.list
  } catch (error) {
    notificationStore.notifyError(getErrorMessage(error, '获取工单统计数据失败'))
  } finally {
    loading.value = false
  }
}

onMounted(() => {
  fetchTickets()
})

// Metrics computation
const totalCount = computed(() => tickets.value.length)
const pendingCount = computed(() => tickets.value.filter(t => t.status === 1).length)
const activeCount = computed(() => tickets.value.filter(t => t.status === 2).length)
const completedCount = computed(() => tickets.value.filter(t => t.status === 3).length)

// Overdue check (Status is 4 or end date is passed and it's not completed)
const todayStr = computed(() => {
  const d = new Date()
  const year = d.getFullYear()
  const month = String(d.getMonth() + 1).padStart(2, '0')
  const day = String(d.getDate()).padStart(2, '0')
  return `${year}-${month}-${day}`
})

const overdueTickets = computed(() => {
  return tickets.value.filter(t => {
    if (t.status === 3) return false
    if (t.status === 4) return true
    return t.planned_end_date && t.planned_end_date < todayStr.value
  })
})

const overdueCount = computed(() => overdueTickets.value.length)

// Category breakdown
const categoryBreakdown = computed(() => {
  const counts: Record<string, number> = {}
  tickets.value.forEach(t => {
    const match = t.name.match(/^【(.*?)】/)
    const cat = match ? match[1] : '其他'
    counts[cat] = (counts[cat] || 0) + 1
  })
  
  const total = totalCount.value || 1
  return Object.entries(counts).map(([name, count]) => {
    return {
      name,
      count,
      percentage: Math.round((count / total) * 100)
    }
  }).sort((a, b) => b.count - a.count)
})

const pieSlices = computed(() => {
  const total = totalCount.value
  if (!total) return []
  
  const circumference = 2 * Math.PI * 55 // ~345.575
  let accumulatedPercent = 0
  
  return categoryBreakdown.value.map(cat => {
    const exactPercent = (cat.count / total) * 100
    const strokeDashArray = `${(exactPercent / 100) * circumference} ${circumference}`
    const accumulatedLength = (accumulatedPercent / 100) * circumference
    accumulatedPercent += exactPercent
    
    return {
      ...cat,
      strokeDashArray,
      strokeDashOffset: -accumulatedLength,
      color: getCategoryColor(cat.name)
    }
  })
})

// 7 Days Flow trend
const last7Days = computed(() => {
  const dates = []
  for (let i = 6; i >= 0; i--) {
    const d = new Date()
    d.setDate(d.getDate() - i)
    const year = d.getFullYear()
    const month = String(d.getMonth() + 1).padStart(2, '0')
    const day = String(d.getDate()).padStart(2, '0')
    dates.push(`${year}-${month}-${day}`)
  }
  return dates
})

const trendData = computed(() => {
  const dates = last7Days.value
  
  // Initialize counts
  const createdCounts: Record<string, number> = {}
  const resolvedCounts: Record<string, number> = {}
  
  dates.forEach(d => {
    createdCounts[d] = 0
    resolvedCounts[d] = 0
  })
  
  tickets.value.forEach(t => {
    // Check created date
    const createdDate = t.created_at?.slice(0, 10)
    if (createdDate && createdDate in createdCounts) {
      createdCounts[createdDate]++
    }
    // Check completed date
    if (t.status === 3 && t.completed_at) {
      const completedDate = t.completed_at.slice(0, 10)
      if (completedDate && completedDate in resolvedCounts) {
        resolvedCounts[completedDate]++
      }
    }
  })
  
  return dates.map(date => {
    // Short date like "06-21" for display
    const label = date.slice(5)
    return {
      date,
      label,
      created: createdCounts[date],
      resolved: resolvedCounts[date]
    }
  })
})

// SVG chart dimensions and layout helpers
const chartWidth = 600
const chartHeight = 280
const padding = { top: 25, right: 20, bottom: 40, left: 35 }

const maxCount = computed(() => {
  let max = 5
  trendData.value.forEach(d => {
    if (d.created > max) max = d.created
    if (d.resolved > max) max = d.resolved
  })
  // round to next multiple of 5
  return Math.ceil(max / 5) * 5
})

const getX = (index: number) => {
  const plotWidth = chartWidth - padding.left - padding.right
  const colWidth = plotWidth / 7
  return padding.left + index * colWidth + colWidth / 2
}

const getY = (count: number) => {
  const plotHeight = chartHeight - padding.top - padding.bottom
  const pct = count / maxCount.value
  return chartHeight - padding.bottom - pct * plotHeight
}

const getBarHeight = (count: number) => {
  const plotHeight = chartHeight - padding.top - padding.bottom
  const pct = count / maxCount.value
  return pct * plotHeight
}

// Hover state for tooltip
const hoveredDay = ref<any | null>(null)
const tooltipX = ref(0)
const tooltipY = ref(0)

function showTooltip(event: MouseEvent, dayData: any) {
  hoveredDay.value = dayData
  
  // Calculate relative tooltip position
  const target = event.currentTarget as any
  const parent = target.parentElement as any
  if (!parent) return
  
  const rect = target.getBoundingClientRect()
  const parentRect = parent.getBoundingClientRect()
  
  tooltipX.value = rect.left - parentRect.left + rect.width / 2
  tooltipY.value = rect.top - parentRect.top - 80
}

function hideTooltip() {
  hoveredDay.value = null
}

function handleTicketClick(id: number) {
  router.push(`/projects/${id}`)
}

function getCategoryColor(name: string): string {
  if (name === '故障') return 'var(--accent-danger)'
  if (name === '需求') return 'var(--accent-start)'
  if (name === '配置') return 'var(--accent-tertiary)'
  if (name === '巡检') return 'var(--accent-success)'
  return 'var(--accent-neutral)'
}

function getOverdueDays(plannedEndDate: string): number {
  const planned = new Date(plannedEndDate)
  const today = new Date()
  today.setHours(0, 0, 0, 0)
  const diffTime = today.getTime() - planned.getTime()
  return Math.max(0, Math.ceil(diffTime / (1000 * 60 * 60 * 24)))
}

function getCleanTitle(fullName: string): string {
  return fullName.replace(/^【.*?】/, '')
}
</script>

<template>
  <div class="stats-view">
    <!-- Header -->
    <section class="stats-view__hero-shell">
      <header class="stats-view__header">
        <div class="stats-view__header-text">
          <p class="stats-view__eyebrow">Data Analytics</p>
          <h2>统计报表</h2>
          <p class="stats-view__desc">实时监控全网运维工单流转效率与超时积压状态</p>
        </div>
        <button class="stats-view__refresh" type="button" @click="fetchTickets" :disabled="loading">
          <svg viewBox="0 0 24 24" class="refresh-icon" :class="{ 'is-loading': loading }">
            <path d="M19 8l-4 4h3c0 3.3-2.7 6-6 6-1.5 0-2.8-.5-3.8-1.5l-1.4 1.4C8.3 19.4 10 20 12 20c4.4 0 8-3.6 8-8h3l-4-4zM6 12c0-3.3 2.7-6 6-6 1.5 0 2.8.5 3.8 1.5l1.4-1.4C15.7 4.6 14 4 12 4c-4.4 0-8 3.6-8 8H1l4 4 4-4H6z" fill="currentColor"/>
          </svg>
          <span>刷新数据</span>
        </button>
      </header>
    </section>

    <div v-if="loading && tickets.length === 0" class="stats-view__loading loading-panel">
      正在加载统计分析看板...
    </div>

    <template v-else>
      <!-- Metrics Overview -->
      <div class="stats-view__metrics">
        <div class="metric-card bg-all">
          <div class="metric-card__glow" />
          <div class="metric-card__content">
            <span class="metric-card__label">总工单数</span>
            <strong class="metric-card__value">{{ totalCount }}</strong>
          </div>
        </div>
        
        <div class="metric-card bg-pending">
          <div class="metric-card__glow" />
          <div class="metric-card__content">
            <span class="metric-card__label">待审批</span>
            <strong class="metric-card__value text-pending">{{ pendingCount }}</strong>
          </div>
        </div>

        <div class="metric-card bg-active">
          <div class="metric-card__glow" />
          <div class="metric-card__content">
            <span class="metric-card__label">处理中</span>
            <strong class="metric-card__value text-active">{{ activeCount }}</strong>
          </div>
        </div>

        <div class="metric-card bg-done">
          <div class="metric-card__glow" />
          <div class="metric-card__content">
            <span class="metric-card__label">已回单完结</span>
            <strong class="metric-card__value text-done">{{ completedCount }}</strong>
          </div>
        </div>

        <div class="metric-card bg-delayed" :class="{ 'has-overdue': overdueCount > 0 }">
          <div class="metric-card__glow" />
          <div class="metric-card__content">
            <span class="metric-card__label">
              超时预警
              <span v-if="overdueCount > 0" class="overdue-dot" />
            </span>
            <strong class="metric-card__value text-delayed">{{ overdueCount }}</strong>
          </div>
        </div>
      </div>

      <!-- Main Visualizations Grid -->
      <div class="stats-view__grid">
        <!-- Flow Trends -->
        <div class="stats-card trend-card-wrapper">
          <div class="stats-card__header">
            <h3>工单流转趋势（近 7 日）</h3>
            <div class="trend-legends">
              <span class="legend-item"><span class="legend-color color-created" />新建工单</span>
              <span class="legend-item"><span class="legend-color color-resolved" />回单结单</span>
            </div>
          </div>
          
          <div class="chart-container">
            <svg :width="chartWidth" :height="chartHeight" viewBox="0 0 600 280" class="trend-chart-svg">
              <defs>
                <linearGradient id="created-grad" x1="0" y1="0" x2="0" y2="1">
                  <stop offset="0%" stop-color="var(--accent-start)" stop-opacity="0.85" />
                  <stop offset="100%" stop-color="var(--accent-end)" stop-opacity="0.2" />
                </linearGradient>
                <linearGradient id="resolved-grad" x1="0" y1="0" x2="0" y2="1">
                  <stop offset="0%" stop-color="var(--accent-success)" stop-opacity="0.85" />
                  <stop offset="100%" stop-color="var(--accent-success)" stop-opacity="0.2" />
                </linearGradient>
              </defs>
              
              <!-- Grid Lines -->
              <g class="grid-lines">
                <line 
                  v-for="tick in 5" 
                  :key="tick"
                  :x1="padding.left"
                  :y1="getY((maxCount / 5) * tick)"
                  :x2="chartWidth - padding.right"
                  :y2="getY((maxCount / 5) * tick)"
                  stroke="var(--border-soft)"
                  stroke-width="1"
                  stroke-dasharray="4,4"
                />
              </g>
              
              <!-- Y Axis Labels -->
              <g class="axis-labels y-labels">
                <text 
                  v-for="tick in 6" 
                  :key="tick"
                  :x="padding.left - 8"
                  :y="getY((maxCount / 5) * (tick - 1)) + 4"
                  text-anchor="end"
                  fill="var(--text-muted)"
                  font-size="10"
                >
                  {{ Math.round((maxCount / 5) * (tick - 1)) }}
                </text>
              </g>

              <!-- X Axis Labels -->
              <g class="axis-labels x-labels">
                <text 
                  v-for="(day, index) in trendData" 
                  :key="index"
                  :x="getX(index)"
                  :y="chartHeight - padding.bottom + 20"
                  text-anchor="middle"
                  fill="var(--text-muted)"
                  font-size="10"
                >
                  {{ day.label }}
                </text>
              </g>

              <!-- Bars -->
              <g 
                v-for="(day, index) in trendData" 
                :key="index"
                class="bar-group"
                @mouseenter="showTooltip($event, day)"
                @mouseleave="hideTooltip"
              >
                <!-- Created Bar (Left) -->
                <rect 
                  :x="getX(index) - 13"
                  :y="getY(day.created)"
                  width="10"
                  :height="getBarHeight(day.created)"
                  rx="3"
                  fill="url(#created-grad)"
                  class="chart-bar created-bar"
                />
                
                <!-- Resolved Bar (Right) -->
                <rect 
                  :x="getX(index) + 3"
                  :y="getY(day.resolved)"
                  width="10"
                  :height="getBarHeight(day.resolved)"
                  rx="3"
                  fill="url(#resolved-grad)"
                  class="chart-bar resolved-bar"
                />

                <!-- Invisible trigger rect for hover area -->
                <rect 
                  :x="getX(index) - 24"
                  :y="padding.top"
                  width="48"
                  :height="chartHeight - padding.top - padding.bottom"
                  fill="transparent"
                  style="cursor: pointer;"
                />
              </g>
            </svg>

            <!-- Chart Tooltip -->
            <div 
              v-if="hoveredDay"
              class="chart-tooltip"
              :style="{ left: tooltipX + 'px', top: tooltipY + 'px' }"
            >
              <p class="tooltip-date">{{ hoveredDay.date }}</p>
              <div class="tooltip-row">
                <span class="tooltip-dot color-created" />
                <span>新建工单: <strong>{{ hoveredDay.created }}</strong></span>
              </div>
              <div class="tooltip-row">
                <span class="tooltip-dot color-resolved" />
                <span>结单完成: <strong>{{ hoveredDay.resolved }}</strong></span>
              </div>
            </div>
          </div>

          <!-- Type Distribution Below SVG -->
          <div class="stats-card__sub-section">
            <h4>工单类型构成分布</h4>
            <div class="distribution-row">
              <div class="donut-chart-container">
                <svg width="160" height="160" viewBox="0 0 160 160" class="donut-chart-svg">
                  <!-- Empty base track -->
                  <circle
                    cx="80"
                    cy="80"
                    r="55"
                    fill="transparent"
                    stroke="var(--border-soft)"
                    stroke-width="14"
                  />
                  <!-- Slices -->
                  <circle
                    v-for="slice in pieSlices"
                    :key="slice.name"
                    cx="80"
                    cy="80"
                    r="55"
                    fill="transparent"
                    :stroke="slice.color"
                    stroke-width="14"
                    :stroke-dasharray="slice.strokeDashArray"
                    :stroke-dashoffset="slice.strokeDashOffset"
                    transform="rotate(-90 80 80)"
                    class="donut-segment"
                  />
                  <!-- Center text: Total Count -->
                  <g class="donut-center-text">
                    <text x="80" y="75" text-anchor="middle" class="donut-center-label">总工单</text>
                    <text x="80" y="102" text-anchor="middle" class="donut-center-val">{{ totalCount }}</text>
                  </g>
                </svg>
              </div>
              <div class="distribution-list">
                <div v-for="cat in pieSlices" :key="cat.name" class="dist-item">
                  <div class="dist-item__header">
                    <span class="dist-item__name">
                      <span class="dist-item__dot" :style="{ backgroundColor: cat.color }" />
                      {{ cat.name }}类工单
                    </span>
                    <span class="dist-item__value">{{ cat.count }}单 ({{ cat.percentage }}%)</span>
                  </div>
                  <div class="dist-item__bar-bg">
                    <div 
                      class="dist-item__bar-fill" 
                      :style="{ 
                        width: cat.percentage + '%',
                        backgroundColor: cat.color
                      }"
                    />
                  </div>
                </div>
              </div>
            </div>
          </div>
        </div>

        <!-- Overdue Warnings -->
        <div class="stats-card overdue-card-wrapper">
          <div class="stats-card__header">
            <h3>超时预警工单中心</h3>
            <span class="warning-badge" v-if="overdueCount > 0">需要督办</span>
          </div>

          <div class="overdue-list" v-smooth-wheel>
            <div v-if="overdueTickets.length === 0" class="overdue-list__empty">
              <svg viewBox="0 0 24 24" class="success-check-icon">
                <path d="M9 16.17L4.83 12l-1.42 1.41L9 19 21 7l-1.41-1.41z" fill="currentColor"/>
              </svg>
              <p>暂无超时工单，流转效率良好！</p>
            </div>

            <template v-else>
              <div class="overdue-summary-alert">
                检测到有 <strong>{{ overdueCount }}</strong> 项工单处于延期未结单状态，请及时联系各执行责任人催办。
              </div>

              <div 
                v-for="ticket in overdueTickets" 
                :key="ticket.id" 
                class="overdue-item"
                @click="handleTicketClick(ticket.id)"
              >
                <div class="overdue-item__header">
                  <span 
                    class="category-tag" 
                    :style="{ 
                      backgroundColor: getCategoryColor(ticket.name.match(/^【(.*?)】/)?.[1] || '其他'),
                      color: '#fff'
                    }"
                  >
                    {{ ticket.name.match(/^【(.*?)】/)?.[1] || '其他' }}
                  </span>
                  <span class="overdue-time">
                    已延期 <strong>{{ getOverdueDays(ticket.planned_end_date) }}</strong> 天
                  </span>
                </div>
                
                <h4 class="overdue-item__title">{{ getCleanTitle(ticket.name) }}</h4>
                
                <div class="overdue-item__meta">
                  <span class="meta-owner">责任人: {{ ticket.owner_real_name }}</span>
                  <span class="meta-date">截止日期: {{ ticket.planned_end_date }}</span>
                </div>
                
                <div class="overdue-item__actions">
                  <span>点击查看流转详情 ➔</span>
                </div>
              </div>
            </template>
          </div>
        </div>
      </div>
    </template>
  </div>
</template>

<style scoped>
.stats-view {
  display: grid;
  gap: 24px;
  padding: 0 8px 8px;
  min-height: calc(100vh - 80px);
}

.stats-view__hero-shell {
  display: grid;
  padding: 24px 26px;
  border: 1px solid var(--border-soft);
  border-radius: 20px;
  background: var(--gradient-surface), var(--project-card-glow);
  box-shadow: var(--shadow-panel-hover);
}

.stats-view__header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 20px;
}

.stats-view__eyebrow {
  margin: 0 0 6px;
  font-size: 0.85rem;
  letter-spacing: 0.16em;
  text-transform: uppercase;
  color: var(--accent-start);
  font-weight: 600;
}

.stats-view__header h2 {
  font-size: 2.2rem;
  margin: 0 0 8px;
  color: var(--text-main);
  line-height: 1.1;
}

.stats-view__desc {
  margin: 0;
  color: var(--text-soft);
  font-size: 0.98rem;
}

.stats-view__refresh {
  appearance: none;
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 12px 20px;
  border-radius: 14px;
  border: 1px solid var(--border-soft);
  background: var(--project-card-bg);
  color: var(--text-main);
  font-weight: 600;
  cursor: pointer;
  box-shadow: var(--shadow-panel);
  transition: transform 200ms, border-color 200ms, box-shadow 200ms;
}

.stats-view__refresh:hover {
  transform: translateY(-2px);
  border-color: var(--accent-line);
  box-shadow: var(--shadow-panel-hover);
}

.refresh-icon {
  width: 16px;
  height: 16px;
}

.refresh-icon.is-loading {
  animation: rotate 1s linear infinite;
}

@keyframes rotate {
  100% { transform: rotate(360deg); }
}

.stats-view__loading {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 300px;
  border: 1px solid var(--border-soft);
  border-radius: 20px;
  font-size: 1.1rem;
  font-weight: 600;
  color: var(--text-soft);
  background: var(--gradient-surface);
}

/* Metrics Cards */
.stats-view__metrics {
  display: grid;
  grid-template-columns: repeat(5, 1fr);
  gap: 16px;
}

.metric-card {
  position: relative;
  overflow: hidden;
  border-radius: 18px;
  border: 1px solid var(--border-soft);
  padding: 22px 20px;
  background: var(--project-card-bg);
  box-shadow: var(--shadow-panel);
  transition: transform 300ms, box-shadow 300ms;
}

.metric-card:hover {
  transform: translateY(-4px);
  box-shadow: var(--shadow-panel-hover);
}

.metric-card__glow {
  position: absolute;
  inset: 0;
  background: radial-gradient(circle at top right, rgba(0, 194, 255, 0.08), transparent 50%);
  pointer-events: none;
}

.metric-card__content {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.metric-card__label {
  font-size: 0.88rem;
  font-weight: 600;
  color: var(--text-soft);
  display: inline-flex;
  align-items: center;
  gap: 6px;
}

.metric-card__value {
  font-size: 2.2rem;
  font-weight: 800;
  line-height: 1.1;
  color: var(--text-main);
}

.overdue-dot {
  width: 8px;
  height: 8px;
  background-color: var(--accent-danger);
  border-radius: 50%;
  display: inline-block;
  box-shadow: 0 0 10px var(--accent-danger);
  animation: pulse 1.5s infinite;
}

@keyframes pulse {
  0% { transform: scale(0.9); opacity: 0.6; }
  50% { transform: scale(1.3); opacity: 1; }
  100% { transform: scale(0.9); opacity: 0.6; }
}

.has-overdue {
  border-color: color-mix(in srgb, var(--accent-danger) 40%, var(--border-soft));
  box-shadow: 0 10px 24px rgba(255, 95, 115, 0.08);
}

.text-pending { color: var(--text-muted); }
.text-active { color: var(--accent-start); }
.text-done { color: var(--accent-success); }
.text-delayed { color: var(--accent-danger); }

/* Grid Visualizations */
.stats-view__grid {
  display: grid;
  grid-template-columns: minmax(0, 1.4fr) minmax(280px, 1fr);
  gap: 24px;
  align-items: start;
}

.stats-card {
  border: 1px solid var(--border-soft);
  border-radius: 20px;
  background: var(--gradient-surface), var(--project-card-glow);
  box-shadow: var(--shadow-panel);
  padding: 24px;
  display: flex;
  flex-direction: column;
  gap: 20px;
}

.stats-card__header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 12px;
}

.stats-card__header h3 {
  margin: 0;
  font-size: 1.25rem;
  font-weight: 700;
  color: var(--text-main);
}

/* Trend Chart */
.trend-legends {
  display: flex;
  gap: 14px;
  font-size: 0.88rem;
  color: var(--text-soft);
}

.legend-item {
  display: flex;
  align-items: center;
  gap: 6px;
}

.legend-color {
  width: 10px;
  height: 10px;
  border-radius: 3px;
}

.color-created { background: var(--accent-start); }
.color-resolved { background: var(--accent-success); }

.chart-container {
  position: relative;
  width: 100%;
  display: flex;
  justify-content: center;
}

.trend-chart-svg {
  overflow: visible;
  max-width: 100%;
}

.chart-bar {
  transition: transform 200ms ease, opacity 200ms ease;
}

.bar-group:hover .chart-bar {
  opacity: 0.88;
  transform: scaleY(1.02);
}

/* Tooltip overlay */
.chart-tooltip {
  position: absolute;
  transform: translateX(-50%);
  background: var(--glass-bg-strong);
  backdrop-filter: var(--backdrop-blur);
  border: 1px solid var(--border-soft);
  border-radius: 12px;
  padding: 12px 14px;
  box-shadow: var(--shadow-panel-hover);
  z-index: 10;
  min-width: 130px;
  pointer-events: none;
  display: flex;
  flex-direction: column;
  gap: 6px;
  transition: left 150ms ease-out, top 150ms ease-out;
}

.tooltip-date {
  margin: 0;
  font-size: 0.82rem;
  color: var(--text-muted);
  border-bottom: 1px solid var(--border-soft);
  padding-bottom: 4px;
  font-weight: 600;
}

.tooltip-row {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 0.88rem;
  color: var(--text-main);
}

.tooltip-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
}

/* Breakdown category list */
.stats-card__sub-section {
  border-top: 1px solid var(--border-soft);
  padding-top: 20px;
}

.stats-card__sub-section h4 {
  margin: 0 0 16px;
  font-size: 1rem;
  font-weight: 700;
  color: var(--text-main);
}

.distribution-row {
  display: flex;
  align-items: center;
  gap: 32px;
}

.donut-chart-container {
  flex-shrink: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  width: 160px;
  height: 160px;
  position: relative;
}

.donut-chart-svg {
  width: 100%;
  height: 100%;
}

.donut-segment {
  transition: stroke-dasharray 0.3s ease, stroke-dashoffset 0.3s ease, stroke-width 0.2s ease;
}

.donut-segment:hover {
  stroke-width: 16;
  cursor: pointer;
}

.donut-center-text {
  fill: var(--text-main);
}

.donut-center-label {
  font-size: 0.8rem;
  font-weight: 600;
  fill: var(--text-soft);
  letter-spacing: 0.05em;
}

.donut-center-val {
  font-size: 1.6rem;
  font-weight: 800;
  fill: var(--text-main);
}

.distribution-list {
  flex-grow: 1;
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 16px;
}

.dist-item {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.dist-item__header {
  display: flex;
  justify-content: space-between;
  font-size: 0.88rem;
  color: var(--text-soft);
}

.dist-item__name {
  display: flex;
  align-items: center;
  gap: 6px;
  font-weight: 600;
}

.dist-item__dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
}

.dist-item__value {
  color: var(--text-main);
  font-weight: 600;
}

.dist-item__bar-bg {
  height: 6px;
  background: var(--border-soft);
  border-radius: 3px;
  overflow: hidden;
}

.dist-item__bar-fill {
  height: 100%;
  border-radius: 3px;
  transition: width 800ms cubic-bezier(0.16, 1, 0.3, 1);
}

/* Overdue Warnings card */
.overdue-card-wrapper {
  max-height: calc(100vh - 180px);
  overflow: hidden;
  display: flex;
  flex-direction: column;
}

.warning-badge {
  background: var(--work-status-delayed-bg);
  border: 1px solid var(--work-status-delayed-border);
  color: var(--accent-danger);
  font-size: 0.78rem;
  font-weight: 700;
  padding: 4px 10px;
  border-radius: 99px;
  letter-spacing: 0.05em;
}

.overdue-list {
  overflow-y: auto;
  display: flex;
  flex-direction: column;
  gap: 14px;
  padding-right: 4px;
}

.overdue-list__empty {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 60px 20px;
  text-align: center;
  color: var(--text-muted);
}

.success-check-icon {
  width: 48px;
  height: 48px;
  color: var(--accent-success);
  margin-bottom: 14px;
  opacity: 0.8;
}

.overdue-summary-alert {
  padding: 12px 14px;
  background: color-mix(in srgb, var(--accent-danger) 8%, var(--panel-bg));
  border: 1px solid color-mix(in srgb, var(--accent-danger) 18%, transparent);
  border-radius: 12px;
  color: var(--text-soft);
  font-size: 0.88rem;
  line-height: 1.5;
}

.overdue-summary-alert strong {
  color: var(--accent-danger);
  font-weight: 700;
}

.overdue-item {
  border: 1px solid var(--border-soft);
  border-radius: 14px;
  padding: 16px;
  background: var(--project-card-bg);
  cursor: pointer;
  transition: transform 200ms, border-color 200ms, box-shadow 200ms;
}

.overdue-item:hover {
  transform: translateY(-2px);
  border-color: color-mix(in srgb, var(--accent-danger) 30%, var(--border-soft));
  box-shadow: 0 8px 20px rgba(255, 95, 115, 0.06);
}

.overdue-item__header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 10px;
}

.category-tag {
  font-size: 0.75rem;
  font-weight: 700;
  padding: 3px 8px;
  border-radius: 6px;
}

.overdue-time {
  font-size: 0.82rem;
  color: var(--text-muted);
}

.overdue-time strong {
  color: var(--accent-danger);
  font-weight: 700;
}

.overdue-item__title {
  margin: 0 0 12px;
  font-size: 1rem;
  font-weight: 600;
  line-height: 1.4;
  color: var(--text-main);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.overdue-item__meta {
  display: flex;
  justify-content: space-between;
  font-size: 0.82rem;
  color: var(--text-soft);
  border-bottom: 1px dashed var(--border-soft);
  padding-bottom: 10px;
  margin-bottom: 10px;
}

.overdue-item__actions {
  text-align: right;
  font-size: 0.82rem;
  font-weight: 600;
  color: var(--accent-start);
}

/* Theme Adjustments for dark mode h3/h4 shadow */
:global(html.dark) .metric-card {
  box-shadow: var(--meta-surface-shadow);
}

/* Responsiveness */
@media (max-width: 1200px) {
  .stats-view__metrics {
    grid-template-columns: repeat(3, 1fr);
  }
}

@media (max-width: 1024px) {
  .stats-view__grid {
    grid-template-columns: 1fr;
  }
  
  .overdue-card-wrapper {
    max-height: 480px;
  }
}

@media (max-width: 768px) {
  .stats-view__header {
    flex-direction: column;
    align-items: flex-start;
    gap: 12px;
  }
  
  .stats-view__metrics {
    grid-template-columns: repeat(2, 1fr);
  }
  
  .distribution-row {
    flex-direction: column;
    align-items: center;
    gap: 20px;
  }
  
  .distribution-list {
    width: 100%;
    grid-template-columns: 1fr;
  }
}

@media (max-width: 480px) {
  .stats-view__metrics {
    grid-template-columns: 1fr;
  }
}
</style>
