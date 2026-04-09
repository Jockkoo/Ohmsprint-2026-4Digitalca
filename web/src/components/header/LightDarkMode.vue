<script setup>
import { useDark, useToggle } from '@vueuse/core'
import { Label } from '@/components/ui/label'
import { Switch } from '@/components/ui/switch'

const isDark = useDark()
const toggleDark = useToggle(isDark)
</script>

<template>
  <div class="flex items-center space-x-2">
    <Label for="dark-mode">Dark Mode</Label>
    <Switch id="dark-mode" :defaultValue="isDark" @update:modelValue="toggleDark()"/>
  </div>
</template>
