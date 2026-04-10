<script setup lang="ts">
import Chart from '@/components/Chart.vue'
import { Card, CardHeader, CardDescription, CardTitle, CardContent } from '@/components/ui/card'
</script>

<template>
  <main class="flex gap-8 justify-center items-stretch p-16">
    <div class="flex-2">
      <Chart />
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
        <CardHeader>
          <CardTitle>Podešavanja</CardTitle>
          <CardDescription>Ovde se moze podesavati sta se i koliko cesto meri etc</CardDescription>
        </CardHeader>
        <CardContent> *zamislite kontrole ovde* </CardContent>
      </Card>
    </div>
  </main>
</template>
