import { Packet, PacketType } from "./packet";
import { Shard, type Client } from "./shard";
import { UUID } from "./utils";
import { Elysia } from "elysia";
import path from "path";

const clients = new Map<UUID, Client>();

const ports = [8080, 8081];
const shards = ports.map(p => new Shard("127.0.0.1", p, clients, ports))

console.log("listening on http://127.0.0.1:3000");
new Elysia()
  .get(
    "/",
    () =>
      new Response(Bun.file(path.join(import.meta.dir, "index.html")), {
        headers: {
          "Content-Type": "text/html",
        },
      }),
  )
  .get(
    "/clients",
    () =>
      new Response(
        JSON.stringify(Array.from(clients.entries()).map((c) => ({
          id: c[0]
        }))),
      ),
  )
  .delete("/clients/:id", ({ params: { id } }) => {
    clients.get(id as UUID)?.queue.push(Packet.from(PacketType.EXIT));
    return new Response();
  })
  .put("/clients/:id/sys", ({ params: { id } }) => {
    clients.get(id as UUID)?.queue.push(Packet.from(PacketType.SYS_CONF));
    return new Response();
  })
  .post("clients/exit", () => {
    shards.forEach((s) => s.socket.stop(true));
    return new Response();
  })
  .listen(3000);
