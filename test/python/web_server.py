
import asyncio
import websockets

async def echo(websocket, path):
    async for message in websocket:
        print(f"{message}")
        msg = "I got your message: {}".format(message)
        await websocket.send(msg)

asyncio.get_event_loop().run_until_complete(websockets.serve(echo, '127.0.0.1', 20002))
asyncio.get_event_loop().run_forever()
