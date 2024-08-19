export enum ActionType {
  EXIT = 0,
  SLEEP = 1,
}

export class Command {
  id: ReturnType<typeof crypto.randomUUID>
  action: ActionType
  payload?: string

  constructor(action: ActionType, payload: Record<string, any>) {
    this.id = crypto.randomUUID()
    this.action = action
    this.payload = Object.keys(payload)
      .map((k) => `${k}=${payload[k]}`)
      .join(';')
  }

  static fromAction(action: ActionType) {
    const instance = new Command(action, {})
    instance.payload = undefined
    return instance
  }

  private static offsets = {
    packetLength: 0,
    id: 4,
    actionType: 40,
    payloadLength: 41,
    payload: 45,
  }

  toBuffer(): Buffer {
    const len = 45 + (this.payload?.length || 0)
    const buffer = Buffer.alloc(len)
    buffer.writeUint32BE(len - 4, Command.offsets.packetLength)
    buffer.write(this.id, Command.offsets.id, 'ascii')
    buffer.writeUint8(this.action, Command.offsets.actionType)
    buffer.writeUint32BE(this.payload?.length || 0, Command.offsets.payloadLength)
    buffer.write(this.payload || '', Command.offsets.payload, 'ascii')

    return buffer
  }
}
