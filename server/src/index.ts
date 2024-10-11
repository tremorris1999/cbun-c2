import { Shard, type Client } from './shard'
import { UUID } from './utils'

const clients = new Map<UUID, Client>()

const ports = [8080, 8081]
const shards = [
  new Shard('127.0.0.1', 8080, clients, ports),
  new Shard('127.0.0.1', 8081, clients, ports),
]
