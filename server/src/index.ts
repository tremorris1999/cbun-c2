import { ActionType, Command } from './c2-command'
import { Shard, type Connection, type Client } from './shard'

const inboundConnections = new Map<string, Connection>()
const clients = [] as Client[]

Bun.listen(Shard(8080, inboundConnections, clients))
Bun.listen(Shard(8081, inboundConnections, clients))
await new Promise<void>(res => setTimeout(() => res(), 5000))
clients.forEach((c) => {
  c.socket.write(new Command(ActionType.CONFIG, {interval: 5, jitter: 0.25}).toBuffer())
})
