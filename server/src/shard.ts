import { ActionType, Command } from './c2-command'
import type { Socket, TCPSocketListenOptions } from 'bun'

type UUID = ReturnType<typeof crypto.randomUUID>
type Connector = (id: 0 | UUID) => Promise<BaseClient>

export interface Connection {
  connector: Connector
  abortController: AbortController
}

interface BaseClient {
  id: UUID
}

export interface Client extends BaseClient {
  socket: Socket<unknown>
}

export const uuidRegex = /[A-z,0-9]{8}\-[A-z,0-9]{4}-[A-z,0-9]{4}-[A-z,0-9]{4}-[A-z,0-9]{12}/

export const Shard = (
  port: number,
  inboundConnectionsRef: Map<string, Connection>,
  clientsRef: Client[]
): TCPSocketListenOptions<undefined> => {
  console.info(`(${port}): listening...`)

  return {
    hostname: '127.0.0.1',
    port,
    socket: {
      data(socket, data) {
        const id =
          data.byteLength === 1 ? data.readUint8(0) : (data.toString('ascii', 0, 36) as UUID)

        const combo = `${socket.remoteAddress}:${socket.localPort}`
        const matchZero = typeof id === 'number' && id === 0
        const matchUUID = typeof id === 'string' && uuidRegex.test(id)
        if (matchZero || matchUUID) {
          inboundConnectionsRef
            .get(combo)
            ?.connector(id)
            .then((client) => {
              clientsRef.push({
                ...client,
                socket,
              })

              console.info(`(${port}): ${socket.remoteAddress} connected as "${client.id}".`)
              socket.write(client.id, 0, 36)
            })
            .catch(() => {
              console.info(`(${port}): ${socket.remoteAddress} failed to connect.`)
              socket.end('fail')
            })
        }

        inboundConnectionsRef.delete(combo)
      }, // message received from client
      open(socket) {
        console.info(`(${port}): ${socket.remoteAddress} attempting to connect...`)
        const abortController = new AbortController()

        const connector = (id: 0 | UUID) =>
          new Promise<BaseClient>((res, rej) => {
            if (abortController.signal.aborted) rej()
            abortController.signal.onabort = () => rej()

            res({
              id: id === 0 ? crypto.randomUUID() : id,
            })
          })

        const combo = `${socket.remoteAddress}:${socket.localPort}`
        setTimeout(() => {
          abortController.abort()
          if (inboundConnectionsRef.has(combo)) {
            inboundConnectionsRef.delete(combo)
            console.info(`(${port}): ${socket.remoteAddress} did not respond in time.`)
            socket.end('timeout')
          }
        }, 10000)

        // Abort any pending connection from this address and port combo
        inboundConnectionsRef.get(combo)?.abortController.abort()

        // Add pending connection
        inboundConnectionsRef.set(combo, {
          abortController,
          connector,
        })
      }, // socket opened
      close(socket) {}, // socket closed
      drain(socket) {}, // socket ready for more data
      error(socket, error) {
        console.log(`ERR (${socket.remoteAddress}): ${error}`)
      }, // error handler
    },
  }
}
