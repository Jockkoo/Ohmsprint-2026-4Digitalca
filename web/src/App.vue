<template>
  <div>
    <button @click="readStatus">Proveri Status (Read)</button>
    <div v-if="statusMessage">
      <p>Status sa uređaja: <strong>{{ statusMessage }}</strong></p>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue';

const statusMessage = ref("");

const readStatus = async () => {
  try {
    // 1. Iniciramo povezivanje (mora na korisnički klik)
    const device = await navigator.bluetooth.requestDevice({
      filters: [
        { name: "ESP32" } // Možeš filtrirati po imenu koje si stavio u esp_ble_gap_set_device_name
      ],
      // KLJUČNA STVAR: Ovde moraš navesti UUID-ove servisa kojima planiraš da pristupaš
      optionalServices: [0xFF00]
    });

    const server = await device.gatt.connect();
    const service = await server.getPrimaryService(0xFF00);
    const characteristic = await service.getCharacteristic(0xFF01);

    // 2. Čitamo vrednost sa karakteristike
    // Ovo direktno okida tvoj 'handle_read' na ESP32
    const value = await characteristic.readValue();

    // 3. Dekodiramo "WE UP!" iz bajtova
    const decoder = new TextDecoder('utf-8');
    statusMessage.value = decoder.decode(value);

    console.log("Pročitano:", statusMessage.value);

    // Opciono: Diskonektuj se ako ti više ne treba veza
    // await device.gatt.disconnect();

  } catch (error) {
    console.error("Greška pri čitanju:", error);
  }
};
</script>
