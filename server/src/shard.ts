import { Packet, PacketType } from './packet'
import { UUID } from './utils'
import type { Socket as BunSocket, TCPSocketListener } from 'bun'

type SocketData = { sessionId: UUID }
export type Socket = BunSocket<SocketData>

export type Client = {
  socket: Socket
  timeout?: ReturnType<typeof setTimeout>
  queue: Packet[]
  config?: Record<string, any>
}

export class Shard {
  readonly hostname: string
  readonly port: number
  readonly socket: TCPSocketListener<SocketData>
  readonly farmPorts: number[]
  private clients: Map<UUID, Client>
  private inboundConnections = new Map<UUID, Socket>()

  constructor(hostname: string, port: number, clients: Map<UUID, Client>, farmPorts: number[]) {
    this.hostname = hostname
    this.port = port
    this.clients = clients
    this.farmPorts = farmPorts
    this.socket = Bun.listen<SocketData>({
      hostname: this.hostname,
      port: this.port,
      socket: {
        open: (socket) => this.onOpen(socket),
        data: (socket, data) => this.onData(socket, data),
        close: (socket) => this.onClose(socket),
      },
    })

    console.debug(`(${this.port}): Started listening as "${this.hostname}"`)
  }

  private onOpen(socket: Socket) {
    socket.data = { sessionId: crypto.randomUUID() }
    this.inboundConnections.set(socket.data.sessionId, socket)
    const packet = new Packet({
      id: socket.data.sessionId,
      type: PacketType.ISSUE_SESSION_ID,
    })

    console.info(
      `(${this.port}): ${socket.remoteAddress} attempting to connect as (transitive) session ${socket.data.sessionId}`
    )
    socket.write(packet.toBuffer())
    setTimeout(() => {
      if (this.inboundConnections.delete(socket.data.sessionId)) {
        socket.end('timeout')
        console.info(`(${this.port}): ${socket.remoteAddress} did not respond in time.`)
      }
    }, 10000)
  }

  private generateSuspension() {
    const rand = new Uint32Array(3)
    crypto.getRandomValues(rand)
    const portIdx = rand[0] % Math.max(0, this.farmPorts.length)
    const timeOffsetScalar = rand[1] / 4_294_967_295 - 0.5
    const timeOffset = 10 + 5 * timeOffsetScalar
    return {
      port: this.farmPorts[portIdx],
      time: Math.floor(Date.now() / 1000 + timeOffset),
    }
  }

  private async onData(socket: Socket, data: Buffer) {
    if (!socket.data.sessionId) {
      socket.end('invalid session')
      return
    }

    const packetIn = Packet.readFromBuffer(data)
    if (!packetIn) {
      console.error(
        `(${this.port}): Invalid packet of length ${data.length} received from ${socket.remoteAddress}!`
      )

      socket.end('unsupported protocol')
      return
    }

    /**
     * Resume session handshake. Assigns existing or new session to connected socket, then adds an ISSUE_SESSION_ID packet to the client's queue.
     */
    if (packetIn.type === PacketType.RESUME_SESSION) {
      this.inboundConnections.delete(socket.data.sessionId)
      const { id: sessionId } = packetIn
      socket.data.sessionId = sessionId
      let existing = this.clients.get(sessionId)
      if (existing?.timeout) clearTimeout(existing.timeout)
      this.clients.set(sessionId, { socket, queue: existing?.queue || [] })
      console.info(
        `(${this.port}): ${socket.remoteAddress} ${existing?.timeout ? 're' : ''}connected as session ${socket.data.sessionId}`
      )

      existing = this.clients.get(sessionId)
      if (!existing) {
        socket.terminate()
        console.error(
          `(${this.port}): forcefully disconnected error client ${socket.remoteAddress}!`
        )
        return
      }
    } else if (packetIn.type === PacketType.SYS_CONF) {
      const { data } = packetIn as { data: Uint8Array }
      const res = Buffer.from(data).toString('ascii', 0, 4)
      const len = Buffer.from(data).readBigUInt64BE(4)
      console.log('res', res, 'len', len)
    }

    /**
     * Grab client queue and send the first packet (or SUSPEND_SESSION if no work exists in queue)
     */
    const { queue } = this.clients.get(socket.data.sessionId) || { queue: [] }
    let packetOut = queue.shift()
    if (!packetOut) {
      const existing = this.clients.get(socket.data.sessionId)
      const suspension = this.generateSuspension()
      if (existing)
        existing.timeout = setTimeout(
          () => this.clients.delete(socket.data.sessionId),
          suspension.time * 2000 - Date.now()
        )

      packetOut = Packet.from(PacketType.SUSPEND_SESSION, suspension)
    }

    socket.write(packetOut.toBuffer())
  }

  private onClose(socket: Socket) {
    this.inboundConnections.delete(socket.data.sessionId)
    const { timeout } = this.clients.get(socket.data.sessionId) || {}
    if (!timeout) this.clients.delete(socket.data.sessionId)
    console.info(`(${this.port}): ${socket.remoteAddress} disconnected.`)
  }
}
