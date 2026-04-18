<script setup lang="ts">
import DashboardGeneric from '@/components/DashboardGeneric.vue'

import { Tabs, TabsContent, TabsList, TabsTrigger } from '@/components/ui/tabs'

import { useESP } from '@/composables/useESP'

const {
  isMeasCurrent,
  isMeasVoltage,
  startCurrentMeas,
  startVoltageMeas,
  stopCurrentMeas,
  stopVoltageMeas,
  voltageData,
  currentData,
  currentMeasPeriodMs,
  voltageMeasPeriodMs,
} = useESP()
</script>

<template>
  <main class="p-8">
    <Tabs default-value="current" class="w-full flex flex-col gap-8">
      <TabsList class="w-full h-fit">
        <TabsTrigger value="current" class="p-2">Struja</TabsTrigger>
        <TabsTrigger value="voltage" class="p-2">Napon</TabsTrigger>
      </TabsList>
      <TabsContent value="current" key="current">
        <DashboardGeneric
          what="struje"
          unit="A"
          :isMeas="isMeasCurrent"
          :data="currentData"
          @start="startCurrentMeas"
          @stop="stopCurrentMeas"
          v-model:measPeriodMs="currentMeasPeriodMs"
        />
      </TabsContent>
      <TabsContent value="voltage" key="voltage">
        <DashboardGeneric
          what="napona"
          unit="V"
          :isMeas="isMeasVoltage"
          :data="voltageData"
          @start="startVoltageMeas"
          @stop="stopVoltageMeas"
          v-model:measPeriodMs="voltageMeasPeriodMs"
        />
      </TabsContent>
    </Tabs>
  </main>
</template>
