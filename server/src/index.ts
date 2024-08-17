import { Shard, type Connection, type Client } from './shard'

const inboundConnections = new Map<string, Connection>()
const clients = [] as Client[]

Bun.listen(Shard(8080, inboundConnections, clients))
Bun.listen(Shard(8081, inboundConnections, clients))
