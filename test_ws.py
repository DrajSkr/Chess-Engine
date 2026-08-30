import asyncio
import websockets
import json

async def test_engine():
    uri = "ws://localhost:8080"
    try:
        async with websockets.connect(uri) as websocket:
            print("Connected")
            msg = await websocket.recv()
            print("Received:", msg)
            
            req = {
                "type": "move",
                "move": "e2e4",
                "fen": "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1"
            }
            print("Sending:", req)
            await websocket.send(json.dumps(req))
            
            response = await websocket.recv()
            print("Received:", response)
    except Exception as e:
        print("Error:", e)

asyncio.run(test_engine())
