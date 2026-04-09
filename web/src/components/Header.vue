<script setup lang="ts">
import LightDarkMode from '@/components/header/LightDarkMode.vue'
import { Button } from '@/components/ui/button'
import { useBLE } from "@/composables/useBLE.ts"

const {
    loading,
    isConnected,
    connect,
    disconnect
} = useBLE();

function handleClick() {
  if(isConnected.value) {
    console.log("disconnect")
    disconnect();
  } else if(!loading.value) {
    console.log("connect")
    connect();
  }
}
</script>

<template>
  <div class="w-full flex justify-between p-4">
    <h1 class="text-4xl font-bold italic text-blue-500">4 DIGITALCA</h1>
    <div class="flex gap-4">
      <LightDarkMode />
      <Button @click="handleClick" class="w-fit">
        <p v-if="loading">
          Povezivanje...
        </p>
        <p v-else-if="isConnected">
          Odveži se
        </p>
        <p v-else>
          Poveži na ESP32
        </p>
      </Button>
    </div>
  </div>
</template>

