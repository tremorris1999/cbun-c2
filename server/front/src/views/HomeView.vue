<script setup lang="ts">
import { ref } from 'vue';

const clients = ref()

fetch('/clients').then(res => res.json()).then(c => clients.value = c)

const disconnect = (id: string) => fetch(`/clients/${id}`, {
  method: 'DELETE'
})

const getConfig = (id: string) => fetch(`/clients/${id}/sys`, {
  method: 'PUT'
})

</script>

<template>
  <main>
    <div class="list-client" v-for="c in clients">
      <p>{{ c.id }}</p>
      <button @click="() => disconnect(c.id)">Disconnect</button>
      <button @click="() => getConfig(c.id)">Config</button>
    </div>
  </main>
</template>

<style scoped>
.list-client {
  display: flex;
  justify-content: space-between; 
}
</style>
