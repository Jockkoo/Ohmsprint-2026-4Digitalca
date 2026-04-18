<script setup lang="ts">
import type { ChartConfig } from '@/components/ui/chart'
import { CurveType } from '@unovis/ts'
import { VisAxis, VisLine, VisXYContainer } from '@unovis/vue'

import {
  ChartContainer,
  ChartCrosshair,
  ChartTooltip,
  ChartTooltipContent,
  componentToString,
} from '@/components/ui/chart'

import { computed } from 'vue'
import { type Data } from '@/composables/useESP.ts'

const props = defineProps<{
  unit: string
  data: Data[]
}>()

const chartData = computed(() => {
  return props.data.slice(-10)
})

const chartConfig = {
  value: {
    label: 'value',
    color: 'var(--chart-1)',
  },
} satisfies ChartConfig

function formatTime(count: number) {
  return ((Date.now() - count) / 1000).toFixed(1)
}
</script>

<template>
  <ChartContainer :config="chartConfig">
    <VisXYContainer :data="chartData" :y-domain="[0, undefined]">
      <VisLine :x="(d: Data) => d.date" :y="(d: Data) => d.value" :color="chartConfig.value.color"
        :curve-type="CurveType.Linear" />
      <VisAxis type="x" :x="(d: Data) => d.date" :tick-line="false" :domain-line="false" :grid-line="false"
        :num-ticks="6" :tick-format="formatTime" />
      <VisAxis type="y" :num-ticks="3" :tick-line="false" :domain-line="false" />
      <ChartTooltip />
      <ChartCrosshair v-if="chartData.length > 0"
        :template="componentToString(chartConfig, ChartTooltipContent, { hideLabel: true })"
        :color="chartConfig.value.color" />
    </VisXYContainer>
  </ChartContainer>
</template>
