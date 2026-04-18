<script setup lang="ts">
import Chart from '@/components/Chart.vue'
import Settings from '@/components/Settings.vue'
import { Card, CardHeader, CardDescription, CardTitle, CardContent } from '@/components/ui/card'
import { type Data, useESP } from '@/composables/useESP'

import { Button } from '@/components/ui/button'

const props = defineProps<{
  what: string
  unit: string
  data: Data[]
  isMeas: boolean
}>()

const emit = defineEmits<{
  start: []
  stop: []
}>()

const measPeriodMs = defineModel<number>('measPeriodMs', { required: true })

const { isConnected } = useESP()

function handleClick() {
  if (props.isMeas) {
    emit('stop')
  } else {
    emit('start')
  }
}
</script>

<template>
  <div class="flex gap-8 justify-center items-stretch">
    <div class="flex-2">
      <Card>
        <div class="flex justify-between pr-6">
          <CardHeader class="flex-1">
            <CardTitle>Merenje {{ props.what }}</CardTitle>
            <CardDescription>Automatsko ažuriranje na svakih {{ measPeriodMs }} ms</CardDescription>
          </CardHeader>
          <Button @click="handleClick" :disabled="!isConnected">
            <span v-if="props.isMeas">Zaustavi merenje</span>
            <span v-else>Započni merenje</span>
          </Button>
        </div>
        <CardContent>
          <Chart :unit="props.unit" :data="props.data" />
        </CardContent>
      </Card>
    </div>
    <div class="flex flex-col gap-8 flex-1">
      <Card class="flex-1">
        <CardHeader>
          <CardTitle>Statistika</CardTitle>
          <CardDescription>Neki opis, ne znam</CardDescription>
        </CardHeader>
        <CardContent> *zamislite statistiku ovde* </CardContent>
      </Card>
      <Card class="flex-1">
        <Settings v-model:measPeriodMs="measPeriodMs" />
      </Card>
    </div>
  </div>
</template>
