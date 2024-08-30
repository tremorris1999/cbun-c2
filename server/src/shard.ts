import { Packet, PacketType } from './packet'
import { UUID } from './utils'
import type { Socket as BunSocket, TCPSocketListener } from 'bun'

type SocketData = { sessionId: UUID }
export type Socket = BunSocket<SocketData>

export type Client = {
  socket: Socket
  timeout?: ReturnType<typeof setTimeout>
}

export class Shard {
  readonly hostname: string
  readonly port: number
  readonly socket: TCPSocketListener<SocketData>
  private clients: Map<UUID, Client>
  private inboundConnections = new Map<UUID, Socket>()

  constructor(hostname: string, port: number, clients: Map<UUID, Client>) {
    this.hostname = hostname
    this.port = port
    this.clients = clients
    this.socket = Bun.listen<SocketData>({
      hostname: this.hostname,
      port: this.port,
      socket: {
        open: (socket) => this.onOpen(socket),
        data: (socket, data) => this.onData(socket, data),
        close: (socket) => this.onClose(socket)
      },
    })

    console.debug(`(${this.port}): Started listening as "${this.hostname}"`)
  }

  private onOpen(socket: Socket) {
    console.info(`(${this.port}): ${socket.remoteAddress} attempting to connect...`)
    socket.data = { sessionId: crypto.randomUUID() }
    this.inboundConnections.set(socket.data.sessionId, socket)
    const packet = new Packet({
      id: socket.data.sessionId,
      type: PacketType.ISSUE_SESSION_ID,
    })

    socket.write(packet.toBuffer())
    setTimeout(() => {
      if (this.inboundConnections.delete(socket.data.sessionId)) {
        socket.end('timeout')
        console.info(`(${this.port}): ${socket.remoteAddress} did not respond in time.`)
      }
    }, 10000)
  }

  private onData(socket: Socket, data: Buffer) {
    if (!socket.data.sessionId) {
      socket.end('invalid session')
      return
    }

    try {
      const packet = Packet.fromBuffer(data)
      if (packet.type === PacketType.RESUME_SESSION) {
        if (this.inboundConnections.delete(socket.data.sessionId)) {
          socket.data.sessionId = packet.id
          const { timeout } = this.clients.get(socket.data.sessionId) || {}
          if (timeout) clearTimeout(timeout)
          this.clients.set(socket.data.sessionId, { socket })
          console.info(
            `(${this.port}): ${socket.remoteAddress} ${timeout ? 're' : ''}connected as session ${socket.data.sessionId}`
          )
        }
      }
    } catch (e) {
      console.error(
        `(${this.port}): Invalid packet of length ${data.length} received from ${socket.remoteAddress}!`
      )

      socket.end('unsupported protocol')
    }
  }

  private onClose(socket: Socket) {
    this.inboundConnections.delete(socket.data.sessionId)
    const { timeout } = this.clients.get(socket.data.sessionId) || {}
    if (!timeout) this.clients.delete(socket.data.sessionId)
    console.info(`(${this.port}): ${socket.remoteAddress} disconnected.`)
  }
}
