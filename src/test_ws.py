import asyncio
import websockets

async def hello():
    async with websockets.connect("ws://localhost:8080") as websocket:
        await websocket.send('{"type":"new_game"}')
        response = await websocket.recv()
        print(f"< {response}")

asyncio.get_event_loop().run_until_complete(hello())
