import asyncio
import websockets
import json

clients = set()
tick = 0

async def handler(ws):
    global tick
    clients.add(ws)
    try:
        async for msg in ws:
            if msg.startswith("drb42"):
                v = int(msg[5:])
                data = json.dumps({"x": tick, "y": v})
                tick += 1
                for c in clients:
                    await c.send(data)
                print(data)
    finally:
        clients.remove(ws)

async def main():
    async with websockets.serve(handler, "0.0.0.0", 8080):
        print("WS server on :8080")
        await asyncio.Future()

asyncio.run(main())
