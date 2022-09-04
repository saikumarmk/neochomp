from fastapi import FastAPI, Request, WebSocket, WebSocketDisconnect
from fastapi.templating import Jinja2Templates
import uvicorn
import cv2
from PIL import Image
from numpy import array
import asyncio


app = FastAPI()
templates = Jinja2Templates(directory="templates")

app.state.current_gif = "gifs/ado-uta.gif"
app.state.gif_changed = False

app.state.imageObject = Image.open(app.state.current_gif)
app.state.total_frames = app.state.imageObject.n_frames

@app.get('/')
def index(request: Request):
    return templates.TemplateResponse("index.html", {"request": request})

@app.post("/changegif")
async def changegif(gif_name: str):
    app.state.current_gif = gif_name
    app.state.gif_changed = True


@app.websocket("/ws")
async def get_stream(websocket: WebSocket):
    await websocket.accept()
    try:
        frame_count = 0
        while True:
            if app.state.gif_changed:
                frame_count = 0
                app.state.imageObject = Image.open(app.state.current_gif)
                app.state.total_frames = app.state.imageObject.n_frames
                app.state.gif_changed = False 
            print(frame_count, app.state.total_frames, app.state.current_gif)
            if frame_count == app.state.total_frames:
                frame_count = 0

            app.state.imageObject.seek(frame_count)
            await asyncio.sleep(0.1)

            _, buffer = cv2.imencode('.jpg', array(app.state.imageObject))
            await websocket.send_bytes(buffer.tobytes())
            frame_count += 1

    except WebSocketDisconnect:
        print("Client disconnected")


"""
Head to 127.0.0.1:8000 to see the gif
Head to 127.0.0.1:8000/docs to change GIF


"""

if __name__ == '__main__':
    uvicorn.run(app, host='0.0.0.0', port=8000)
