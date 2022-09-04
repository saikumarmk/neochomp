# neochomp
Software for rendering animations on a LED-Matrix.


## Current Useage

The required Python libraries are contained in `requirements.txt`. To install this, run `python -m pip install -r requirements.txt`. 

Run `app.py` and head to `127.0.0.1:8000/docs` to change the GIF. Include the relative directory e.g `gifs/breakdancing.gif` to load in properly. You can start a websocket connection through `ws://192.168.0.xx:8000/ws` or view it at `127.0.0.1:8000`.