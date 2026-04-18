<script setup lang="ts">
import type { ChartConfig } from '@/components/ui/chart'
import { CurveType } from '@unovis/ts'
import { VisAxis, VisLine, VisXYContainer } from '@unovis/vue'
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card'
import {
  ChartContainer,
  ChartCrosshair,
  ChartTooltip,
  ChartTooltipContent,
  componentToString,
} from '@/components/ui/chart'

import { computed } from 'vue'
import { type Data, useESP } from '@/composables/useESP.ts'

const { voltageData } = useESP()

const chartData = computed(() => {
  return voltageData.value.slice(-10)
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
  <Card>
    <CardHeader>
      <CardTitle>Merenje napona</CardTitle>
      <CardDescription> Automatsko ažuriranje na svakih {{ 1 }} sekundi</CardDescription>
    </CardHeader>
    <CardContent>
      <ChartContainer :config="chartConfig">
        <VisXYContainer :data="chartData" :y-domain="[0, undefined]">
          <VisLine :x="(d: Data) => d.date" :y="(d: Data) => d.value" :color="chartConfig.value.color"
            :curve-type="CurveType.Linear" />
          <VisAxis type="x" :x="(d: Data) => d.date" :tick-line="false" :domain-line="false" :grid-line="false"
            :num-ticks="6" :tick-format="formatTime" />
          <VisAxis type="y" :num-ticks="3" :tick-line="false" :domain-line="false" />
          <ChartTooltip />
          <ChartCrosshair :template="componentToString(chartConfig, ChartTooltipContent, { hideLabel: true })"
            :color="chartConfig.value.color" />
        </VisXYContainer>
      </ChartContainer>
    </CardContent>
  </Card>
</template>
