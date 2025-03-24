import time
import gifio
import board
import gc
import displayio
from adafruit_qualia.graphics import Graphics, Displays

print("Hello World!")
jorb = gifio.OnDiskGif('/smoljorb2.gif')
group = displayio.Group(scale=3)
graphics = Graphics(Displays.ROUND40, default_bg=None, auto_refresh=True)

start = time.monotonic()
next_delay = jorb.next_frame()  # Load the first frame
end = time.monotonic()
call_delay = end - start

face = displayio.TileGrid(jorb.bitmap,
                          pixel_shader=displayio.ColorConverter
                          (input_colorspace=displayio.Colorspace.RGB565_SWAPPED))
#face.x = (graphics.display.width - face.width) // 2
#face.y = (graphics.display.height - face.height) // 2
group.append(face)
graphics.display.root_group = group

# Play the GIF file until screen is touched
while True:
    time.sleep(max(0, next_delay - call_delay))
    next_delay = jorb.next_frame()
# End while
# Clean up memory
jorb.deinit()
jorb = None
gc.collect()
