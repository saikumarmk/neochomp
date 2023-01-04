from fastapi import FastAPI, WebSocket, WebSocketDisconnect
import uvicorn
import asyncio
import time


app = FastAPI()


app.state.current_gif = "gifs/ado-uta.gif"
app.state.gif_changed = False
app.state.pixlet_module = ''


app.state.configstate = {"Location": "Melbourne, Australia", "Name": "Sai"}


@app.post("/change_animation")
async def change_animation(animation_file: str, pixlet_module: str = ''):
    app.state.current_gif = animation_file
    
    app.state.pixlet_module = pixlet_module


async def render_pixlet():
    """
    Renders the current pixlet module, if any is selected.
    """

    # Renders the currently set pixlet module.

    if (module := app.state.pixlet_module) != '':
        proc = await asyncio.create_subprocess_shell(
            f'pixlet render pixlet-apps/{module}.star -o {module}.gif --gif',
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE)

        await proc.communicate()


@app.post('/set_pixlet_app')
async def set_pixlet_app(module: str):

    # Sets the pixlet application, and then renders it for use.

    await change_animation(f'{module}', module)
    await render_pixlet()
    app.state.gif_changed = True


@app.post('/kill_server_client')
async def kill_server_client():
    print(hasattr(app.state, 'willKill'))
    app.state.willKill = True


@app.get("/configuration")
async def configuration():
    return app.state.configstate


@app.websocket("/")
async def button_cycle_websocket(websocket: WebSocket):
    await websocket.accept()

    rerenderTime = time.time()

    gif_array = ["london.gif", "sky3.gif"]
    gif_counter = 0

    try:
        while True:

            data = await websocket.receive_text()

            # Handle C physical change
            if(data == "BUTTON"):
                gif_counter = (gif_counter + 1) % len(gif_array)
                await websocket.send_text(gif_array[gif_counter])

            if hasattr(app.state, 'willKill'):
                app.state.clientKilled = True
                await websocket.send_text('terminate')

            if (time.time() - rerenderTime > 20):
                await render_pixlet()
                rerenderTime = time.time()

    except WebSocketDisconnect:  # User disconnect
        quit()


@app.websocket("/alternate")
async def pixlet_controller_websocket(websocket: WebSocket):
    await websocket.accept()

    rerenderTime = time.time()

    try:
        while True:

            data = await websocket.receive_text()
            if(data == "BUTTON"):
                pass

            # Pixlet app change reflects new GIF to be loaded in
            if app.state.gif_changed:
                app.state.gif_changed = False
                await websocket.send_text(app.state.current_gif)

            # DC C client and quit Py controller
            if hasattr(app.state, 'willKill'):
                app.state.clientKilled = True
                await websocket.send_text('terminate')

            if (time.time() - rerenderTime > 20):
                rerenderTime = time.time()
                await render_pixlet()
                await websocket.send_text(app.state.current_gif)

    except WebSocketDisconnect:  # User disconnect
        quit()


uvicorn.run(app, host='0.0.0.0', port=8080)
