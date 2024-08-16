import { ActionType, Command } from "./c2-command";
console.log("listening on 8080");

const commands = [
  new Command(ActionType.EXIT),
  new Command(ActionType.GET_ENV),
  new Command(ActionType.RUN_COMMAND, { foo: 'bar' }),
  new Command(ActionType.RUN_COMMAND, {
    cmd: "whoami",
    args: null
  })
]

Bun.listen({
  hostname: "127.0.0.1",
  port: 8080,
  socket: {
    data(socket, data) {
      console.log(`(${socket.remoteAddress}): ${data}`);
    }, // message received from client
    open(socket) {
      console.log(`(${socket.remoteAddress}): connected`);

      const idx = Math.round(Math.random() * (commands.length - 1))
      const command = commands[idx];
      socket.write(command.toBuffer());
    }, // socket opened
    close(socket) {
      `(${socket.remoteAddress}): closed`;
    }, // socket closed
    drain(socket) {}, // socket ready for more data
    error(socket, error) {
      console.log(`ERR (${socket.remoteAddress}): ${error}`);
    }, // error handler
  },
});
