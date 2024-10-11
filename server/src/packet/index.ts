import type { UUID } from '../utils'

export enum PacketType {
  EXIT = 0,
  ISSUE_SESSION_ID = 1,
  RESUME_SESSION = 2,
  SUSPEND_SESSION = 3,
}

const PacketMemory = {
  length: {
    start: 0,
    length: 4,
  },
  id: {
    start: 4,
    length: 36,
  },
  type: {
    start: 40,
    length: 1,
  },
  dataLength: {
    start: 41,
    length: 4,
  },
  data: {
    start: 45,
  },
}

export class Packet {
  readonly direction: 'IN' | 'OUT'
  readonly length: number
  readonly id: UUID
  readonly type: PacketType
  readonly data: string | Uint8Array

  constructor({
    id,
    type,
    data = new Uint8Array(0),
    direction = 'OUT',
  }: {
    id: UUID
    type: PacketType
    data?: string | Uint8Array
    direction?: 'IN' | 'OUT'
  }) {
    this.id = id
    this.type = type
    this.data = data
    this.direction = direction
    this.length = 41 + data.length
  }

  static readFromBuffer(buffer: Buffer): Packet | null {
    try {
      const params = {
        direction: 'IN',
        length: buffer.readUInt32BE(PacketMemory.length.start),
        id: buffer.toString('ascii', PacketMemory.id.start, PacketMemory.id.length),
        type: buffer.readUInt8(PacketMemory.type.start),
      } as Packet

      return new Packet({
        ...params,
        data: Uint8Array.prototype.slice
          .bind(buffer)
          .call(PacketMemory.data.start, params.length - 40),
      })
    } catch {
      return null
    }
  }

  private static fromRecord(type: PacketType, obj?: Record<string, string | number>) {
    const data = obj
      ? Object.keys(obj)
          .map((k) => `${k}=${obj[k]}`)
          .join(';')
      : ''

    return new Packet({
      id: crypto.randomUUID(),
      type: type,
      data: data,
      direction: 'OUT',
    })
  }

  toBuffer() {
    const buffer = Buffer.alloc(this.length + 4)
    buffer.writeUInt32BE(this.length)
    buffer.write(this.id.toString(), 4, 36, 'ascii')
    buffer.writeUInt8(this.type, 40)
    buffer.writeUInt32BE(
      typeof this.data === 'string' ? this.data.length : this.data.byteLength,
      41
    )
    if (typeof this.data === 'string') buffer.write(this.data, 45, this.data.length, 'ascii')
    else Buffer.from(this.data).copy(buffer, 45)

    return buffer
  }

  static from(type: PacketType.SUSPEND_SESSION, obj: { port: number; time: number }): Packet
  static from(type: PacketType.ISSUE_SESSION_ID): Packet
  static from(type: PacketType, obj?: Record<string, string | number>) {
    return Packet.fromRecord(type, obj)
  }
}
