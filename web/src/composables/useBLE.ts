import { ref } from 'vue'

export type Data = {
  date: number,
  value: number,
}

const data = ref([] as Data[])
const loading = ref(false)
const error = ref<string | null>(null)
const isConnected = ref(false)
// there is no Bluetooth device type still
const device = ref<any>(null)

async function connect() {
  loading.value = true
  error.value = null

  try {
    device.value = await navigator.bluetooth.requestDevice({
      filters: [{ name: "ESP32" }],
      optionalServices: [0xFF00]
    })

    const server = await device.value.gatt?.connect()
    if(!server) {
      throw new Error("GATT server nije dostupan!")
    }

    isConnected.value = true

    device.value.addEventListener('gattserverdisconnected', () => {
      isConnected.value = false
      device.value = null
    })

    const service = await server.getPrimaryService(0xFF00)
    const characteristic = await service.getCharacteristic(0xFF01)

    characteristic.addEventListener('characteristicvaluechanged', (event: Event) => {
      const target = event.target as any

      if(target.value) {
        const value = target.value.getUint16(0, true) // little endian
        data.value.push({ date: Date.now(), value: value})
      }
    })

    await characteristic.startNotifications()
  } catch(err: any) {
    error.value = err.message || "Greška pri povezivanju sa ESP32"
  } finally {
    loading.value = false
  }
}

async function disconnect() {
  if(device.value?.gatt?.connected) {
    device.value.gatt.disconnect()
  }

  isConnected.value = false
  device.value = null
}


export function useBLE() {
  return {
    name: "ESP32",
    data,
    loading,
    error,
    isConnected,
    connect,
    disconnect
  }
}

