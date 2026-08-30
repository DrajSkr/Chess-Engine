import asyncio
import websockets
import json

async def test_engine():
    uri = "ws://localhost:8080"
    async with websockets.connect(uri) as ws:
        # Handshake
        initial = await ws.recv()
        print("Initial state:", initial)
        
        await ws.send(json.dumps({"type": "new_game"}))
        initial = await ws.recv()
        print("New game state:", initial)
        
        moves = ["e2e4", "d2d4", "e4e5", "e5f6", "f6g7"]
        for m in moves:
            print(f"Sending move: {m}")
            await ws.send(json.dumps({"type": "move", "move": m}))
            result = await ws.recv()
            print(f"Engine reply: {result}")

asyncio.run(test_engine())
