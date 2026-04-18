<script setup lang="ts">
import { Button } from '@/components/ui/button'
import { useESP } from '@/composables/useESP.ts'
import DarkMode from '@/components/header/DarkMode.vue'

const { loadingConnection, isConnected, connect, disconnect } = useESP()

function handleClick() {
  if (isConnected.value) {
    disconnect()
  } else if (!loadingConnection.value) {
    connect()
  }
}
</script>

<template>
  <div class="w-full flex justify-between p-4">
    <h1 class="text-4xl font-bold italic text-blue-500">4 DIGITALCA</h1>
    <div class="flex gap-4">
      <DarkMode />
      <Button @click="handleClick" class="w-fit">
        <p v-if="loadingConnection">Povezivanje...</p>
        <p v-else-if="isConnected">Odveži se</p>
        <p v-else>Poveži na ESP32</p>
      </Button>
    </div>
  </div>
</template>
