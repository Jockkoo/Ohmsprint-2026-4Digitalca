import { ref, watch } from 'vue'

export type Data = {
  date: number
  value: number
}

const SRVC_UUID = 0xff00
const CURRENT_UUID = 0xff01
const VOLTAGE_UUID = 0xff02
const CURRENT_MEAS_PERIOD_UUID = 0xff03
const VOLTAGE_MEAS_PERIOD_UUID = 0xff04

// there is no Bluetooth device type so we use any
const device = ref<any>(null)

const currentMeasPeriodMs = ref(1000)
const voltageMeasPeriodMs = ref(1000)

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
let currentMeasPeriodChar: any = null
let voltageMeasPeriodChar: any = null

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

    currentMeasPeriodChar = await service.getCharacteristic(CURRENT_MEAS_PERIOD_UUID)
    if (!currentMeasPeriodChar) {
      throw new Error('current meas period char not found')
    }

    voltageMeasPeriodChar = await service.getCharacteristic(VOLTAGE_MEAS_PERIOD_UUID)
    if (!voltageMeasPeriodChar) {
      throw new Error('voltage meas period char not found')
    }
  } catch (err: any) {
    error.value = err.message || 'unknown error'
  } finally {
    loadingConnection.value = false
  }
}

async function startCurrentMeas() {
  if (!currentChar) {
    return
  }

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
  if (!voltageChar) {
    return
  }

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
  await currentChar.stopNotifications()
  isMeasCurrent.value = false
}

async function stopVoltageMeas() {
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

function asU32(value: number) {
  const buffer = new ArrayBuffer(4)
  const view = new DataView(buffer)

  view.setUint32(0, value, true)

  return view.buffer
}

async function writeCurrentMeasPeriod(value: number) {
  if (!currentMeasPeriodChar) {
    return
  }

  await currentMeasPeriodChar.writeValue(asU32(value))
}

async function writeVoltageMeasPeriod(value: number) {
  if (!voltageMeasPeriodChar) {
    return
  }

  await voltageMeasPeriodChar.writeValue(asU32(value))
}

watch(currentMeasPeriodMs, () => {
  console.log('saljemo current')
  writeCurrentMeasPeriod(currentMeasPeriodMs.value)
})

watch(voltageMeasPeriodMs, () => {
  console.log('saljemo voltage')
  writeVoltageMeasPeriod(voltageMeasPeriodMs.value)
})

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
    currentMeasPeriodMs,
    voltageMeasPeriodMs,
  }
}
