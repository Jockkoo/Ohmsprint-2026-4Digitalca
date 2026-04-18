<script setup lang="ts">
import { CardHeader, CardContent, CardTitle, CardDescription } from '@/components/ui/card'
import { Slider } from '@/components/ui/slider'
import { useESP } from '@/composables/useESP.ts'

import { ref, watchEffect, computed } from 'vue'
import { refDebounced } from '@vueuse/core'

const MIN = 100
const MAX = 10000

const measPeriodMs = defineModel<number>('measPeriodMs', { required: true })

const { isConnected } = useESP()

const value = ref([measPeriodMs.value])
const debouncedValue = refDebounced(value, 200)

watchEffect(() => {
  measPeriodMs.value = debouncedValue.value[0]!
})

const currentValueText = computed(() => {
  return value.value[0]! >= 1000
    ? (value.value[0]! / 1000).toFixed(1) + ' s'
    : value.value[0] + ' ms'
})
</script>

<template>
  <CardHeader>
    <CardTitle>Podešavanja</CardTitle>
    <CardDescription>Ovde možete podesiti željene parametre</CardDescription>
  </CardHeader>

  <CardContent class="space-y-6">
    <div class="flex flex-col gap-4">
      <div class="flex items-center justify-between">
        <label class="font-medium peer-disabled:cursor-not-allowed peer-disabled:opacity-70">
          Period ažuriranja
        </label>
        <span class="text-lg font-bold text-secondary-foreground">{{ currentValueText }} </span>
      </div>

      <div class="space-y-3">
        <Slider
          v-model="value"
          :disabled="!isConnected"
          :min="MIN"
          :max="MAX"
          :step="50"
          class="cursor-pointer"
        />
        <div class="flex justify-between items-center px-1">
          <span class="text-sm text-muted-foreground"> {{ MIN }} ms </span>
          <span class="text-sm text-muted-foreground"> {{ MAX / 1000 }} s </span>
        </div>
      </div>
    </div>
  </CardContent>
</template>
