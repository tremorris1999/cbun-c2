import { Packet, PacketType } from './packet'
import { Shard, type Client } from './shard'
import { UUID } from './utils'
import { Elysia } from 'elysia'
import staticPlugin from '@elysiajs/static'
import path from 'path'

const clients = new Map<UUID, Client>()

await Bun.spawn({
  cmd: ['bun', 'run', 'build'],
  cwd: path.join(import.meta.dir, '..', 'front'),
}).exited

const ports = [8080, 8081, 8082]
const shards = ports.map((p) => new Shard('127.0.0.1', p, clients, ports))

console.log('listening on http://127.0.0.1:3000')
new Elysia()
  .use(
    staticPlugin({
      assets: path.join(path.dirname(import.meta.dir), 'front', 'dist'),
      prefix: '/',
    })
  )
  .get('/clients', async () => {
    return new Response(
      JSON.stringify(
        Array.from(clients.entries()).map((c) => ({
          id: c[0],
          queue: c[1].queue
        }))
      )
    )
  })
  .delete('/clients/:id', ({ params: { id } }) => {
    clients.get(id as UUID)?.queue.push(Packet.from(PacketType.EXIT))
    return new Response()
  })
  .put('/clients/:id/sys', ({ params: { id } }) => {
    clients.get(id as UUID)?.queue.push(Packet.from(PacketType.SYS_CONF))
    return new Response()
  })
  .post('clients/exit', () => {
    shards.forEach((s) => s.socket.stop(true))
    return new Response()
  })
  .listen(3000)
