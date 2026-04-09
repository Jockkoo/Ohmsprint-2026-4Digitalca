import { ref, onMounted, onUnmounted } from 'vue'

export function useVoltageTrack() {
  const chartData = ref<{ time: string; voltage: number }[]>([])
  const maxPoints = 20 // Koliko tačaka želimo da vidimo na ekranu odjednom
  let timer: ReturnType<typeof setInterval>

  const generateData = () => {
    const now = new Date()
    const timeStr = `${now.getHours()}:${now.getMinutes()}:${now.getSeconds()}`

    // Generiše napon između 5V i 8V (sa malim šumom)
    const newVoltage = Number((Math.random() * (8 - 5) + 5).toFixed(2))

    const newData = {
      time: timeStr,
      voltage: newVoltage
    }

    // Dodajemo podatak i držimo listu fiksne dužine radi performansi
    chartData.value = [...chartData.value, newData].slice(-maxPoints)
  }

  onMounted(() => {
    // Inicijalni podaci
    for(let i=0; i<5; i++) generateData()

    // Simulacija API-ja na svaku sekundu
    timer = setInterval(generateData, 1000)
  })

  onUnmounted(() => {
    clearInterval(timer)
  })

  return { chartData }
}
