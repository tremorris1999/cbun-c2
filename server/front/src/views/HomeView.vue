<script setup lang="ts">
import { ref } from 'vue'

const clients = ref()

fetch('/clients')
  .then((res) => res.json())
  .then((c) => (clients.value = c))

const disconnect = (id: string) =>
  fetch(`/clients/${id}`, {
    method: 'DELETE',
  })

const getConfig = (id: string) =>
  fetch(`/clients/${id}/sys`, {
    method: 'PUT',
  })
</script>

<template>
  <main style="max-width: 50dvw">
    <p v-if="!clients.length">No clients connected</p>
    <div class="list-client" v-for="c in clients">
      <p>{{ c.id }}</p>
      <div class="list-client">
        <button style="margin-right: 16px;" @click="() => disconnect(c.id)">Disconnect</button>
        <button @click="() => getConfig(c.id)">Get Config</button>
      </div>  
    </div>
  </main>
</template>

<style scoped>
.list-client {
  display: flex;
  justify-content: space-between;
}
</style>
