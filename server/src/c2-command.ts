export enum ActionType {
  EXIT = 0,
  GET_ENV = 1,
  RUN_COMMAND = 2,
}

export class Command {
  constructor(
    action: ActionType,
    payload: Record<string, any> | undefined = undefined,
  ) {
    this.id = crypto.randomUUID();
    this.action = action;
    this.payload = payload ? JSON.stringify(payload) : "";
  }

  id: ReturnType<typeof crypto.randomUUID>;
  action: ActionType;
  payload: string;

  private static offsets = {
    packetLength: 0,
    id: 4,
    actionType: 40,
    payloadLength: 41,
    payload: 45,
  };

  toBuffer(): Buffer {
    const len = 45 + this.payload.length;
    const buffer = Buffer.alloc(len);
    buffer.writeUint32BE(len - 4, Command.offsets.packetLength);
    buffer.write(this.id, Command.offsets.id, "ascii");
    buffer.writeUint8(this.action, Command.offsets.actionType);
    buffer.writeUint32BE(this.payload.length, Command.offsets.payloadLength);
    buffer.write(this.payload, Command.offsets.payload, "ascii");

    return buffer;
  }
}
