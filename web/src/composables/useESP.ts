import { ref } from 'vue'

export type Data = {
  date: number
  value: number
}

const SRVC_UUID = 0xff00
const CURRENT_UUID = 0xff01
const VOLTAGE_UUID = 0xff02

// there is no Bluetooth device type so we use any
const device = ref<any>(null)

const currentData = ref([] as Data[])
const voltageData = ref([] as Data[])

const isMeasCurrent = ref(false)
const isMeasVoltage = ref(false)

const loadingData = ref(false)
const loadingConnection = ref(false)

const isConnected = ref(false)

const error = ref<string | null>(null)

let currentChar: any = null
let voltageChar: any = null

async function connect() {
  loadingConnection.value = true
  error.value = null

  try {
    device.value = await navigator.bluetooth.requestDevice({
      filters: [{ name: 'ESP32' }],
      optionalServices: [SRVC_UUID],
    })

    const server = await device.value.gatt?.connect()
    if (!server) {
      throw new Error('server not available')
    }

    isConnected.value = true

    device.value.addEventListener('gattserverdisconnected', () => {
      isConnected.value = false
      device.value = null
    })

    const service = await server.getPrimaryService(SRVC_UUID)
    if (!service) {
      throw new Error('primary service not found')
    }

    currentChar = await service.getCharacteristic(CURRENT_UUID)
    if (!currentChar) {
      throw new Error('current char not found')
    }

    voltageChar = await service.getCharacteristic(VOLTAGE_UUID)
    if (!voltageChar) {
      throw new Error('voltage char not found')
    }
  } catch (err: any) {
    error.value = err.message || 'unknown error'
  } finally {
    loadingConnection.value = false
  }
}

async function startCurrentMeas() {
  loadingData.value = true
  error.value = null

  try {
    currentChar.addEventListener('characteristicvaluechanged', (event: Event) => {
      const target = event.target as any

      if (target.value) {
        const value = target.value.getUint32(0, true) // little endian
        currentData.value.push({ date: Date.now(), value: value })
      }
    })

    await currentChar.startNotifications()
    isMeasCurrent.value = true
  } catch (err: any) {
    error.value = err.message || 'unknown error'
  } finally {
    loadingData.value = false
  }
}

async function startVoltageMeas() {
  loadingData.value = true
  error.value = null

  try {
    voltageChar.addEventListener('characteristicvaluechanged', (event: Event) => {
      const target = event.target as any

      if (target.value) {
        const value = target.value.getUint32(0, true) // little endian
        voltageData.value.push({ date: Date.now(), value: value })
      }
    })

    await voltageChar.startNotifications()
    isMeasVoltage.value = true
  } catch (err: any) {
    error.value = err.message || 'unknown error'
  } finally {
    loadingData.value = false
  }
}

async function stopCurrentMeas() {
  currentChar.removeEventListener('characteristicvaluechanged')
  await currentChar.stopNotifications()
  isMeasCurrent.value = false
}

async function stopVoltageMeas() {
  voltageChar.removeEventListener('characteristicvaluechanged')
  await voltageChar.stopNotifications()
  isMeasVoltage.value = false
}

async function disconnect() {
  if (device.value?.gatt?.connected) {
    device.value.gatt.disconnect()
  }

  isConnected.value = false
  device.value = null
}

export function useESP() {
  return {
    name: 'ESP32',
    isConnected,
    loadingConnection,
    error,
    connect,
    disconnect,
    voltageData,
    currentData,
    startCurrentMeas,
    startVoltageMeas,
    stopCurrentMeas,
    stopVoltageMeas,
    isMeasCurrent,
    isMeasVoltage,
  }
}
