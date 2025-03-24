# Introduction
Are you a fan of [Jelly Hoshiumi?](https://www.youtube.com/@JellyHoshiumi) Have you seen the jorb, and found yourself mesmerized by it? 
![glory to the jorb](../doc/jorb.gif "jorb")
Would you like to have a jorb that you can hold and take places?  Then this project is for you!
![finished product](../doc/jorb_assembly.png)

The jorb uses some off-the-shelf components and a simple 3D-printed enclosure.  All the source code you need is in this repository and the tools to compile and load the firmware are all freely available online.  No soldering is required to assemble the electronics, either!

# Parts List
First, you'll need an [Adafruit Qualia ESP32-S3 board](https://www.adafruit.com/product/5800).  This is the "brain" of the jorb! It has an ESP32 microcontroller with all the support circuitry and a ribbon connnector you need to plug-and-play with the round LCDs in this list! Adafruit has a [pretty good online guide](https://learn.adafruit.com/adafruit-qualia-esp32-s3-for-rgb666-displays) for the board, but I found the Arduino section to be a little out-of-date and inaccurate - it's all based on the older Arduino 1.8 IDE, while this project was tested with the 2.3.4 IDE.  More importantly, the 'blink' example sketch *does not work* on this board! There's no LED for it to blink and it fails to compile. 

The ESP32-S3 board uses a USB-C cable to program it and also for power.  If you're using it as a desktop display, you can just leave it plugged into your PC.  I powered it in wearable form by running a USB-C cable to a powerbank in my pocket.  I used it for about 8 hours on a 10,000mAh pack and barely made a dent in it, so you can probably get away with a smaller powerbank! 

Next, you'll need an LCD.  I used the [4" round LCD](https://www.adafruit.com/product/5793), but it's kinda spendy ($55 as I write this).  There is also [a 2.1" version](https://www.adafruit.com/product/5806) that's less than half the cost ($25) if you get the no-touchscreen version, or a middle-ground [2.8" version](https://www.adafruit.com/product/5852) for $35.  (Prices as of March 2025) I tried to write the code so it can work on the smaller displays, but I don't own any to test with, so *be prepared to debug if you don't just buy the 4" display!*

Lastly, you'll want an enclosure to put it in.  I modified the existing [Adafruit Qualia case for the 4" display](https://www.thingiverse.com/thing:6314263) - the modified files I used for the Jorb are posted [here](https://www.thingiverse.com/thing:6989100).  If you combine my modified files with 4tft-frame.stl and 4tft-case.stl, that's everything you need!  

# Software and Libraries Used
This project went through multiple iterations where I switched back and forth between CircuitPython and Arduino IDE (C++).  I was able to get results I was happy with using Arduino, so that version is presented here! There seems to be conflicting info about what versions of the Arduino IDE and graphics libraries are supported on the Qualia ESP32-S3 board!  Fortunately, I tried the latest versions of everything and found that it does, indeed, work.  In case the definition of "latest" changes and things break in the future, here are the exact versions I used to compile the jorb myself!
## Arduino IDE and Board Support Package
- Arduino IDE: 2.3.4
- Board support package: esp32 by Espressif Systems 3.1.3
The board support package is not a "stock" arduino BSP - you need to add a 3rd party board manager to get it.  Once you have Arduino 2.3.4 installed and open, click File->Preferences.  There's a field for "Additional boards manager URLs" - put *https://espressif.github.io/arduino-esp32/package_esp32_index.json* in this box.  You should then be able to add it by clicking the board manager icon (2nd from the top), typing "esp32" in the search bar, and installing the "esp32 by Expressif Systems" board support package.
![bsp screenshot](../doc/esp32_board.png)

## Arduino Libraries
To install libraries, click on the libraries icon (3rd one down) and type the name in the search box. Click 'install' and the library will be added to your IDE.  You might get a popup asking if you want to also install dependencies for each library - you do! 
You might need to use the drop-down to select a specific version.  For example, between when I got the jorb working and when I made this writeup there was already a new version of the graphics library. 
![library screenshot](../doc/library.png)
- GFX Library for Arduino by Moon On Our Nation 1.5.5
- Adafruit GFX Library 1.12.0
- Adafruit FT6206 Library by Adafruit 1.1.0
- Adafruit CST8XX Library by Adafruit 1.1.1
- AnimatedGIF by Larry Bank 2.2.0

# How it works
All the code for this project lives in a single file, desk_jorb.ino.  The best way to understand how it works is to look at that file, but here's a quick rundown:
- There's a big .h file that contains the gif file encoded as a byte array for storage in the arduino flash memory. I used [this tool](https://github.com/bitbank2/image_to_c.git) to generate it.
- There are some size parameters for the original file (240x240) and the display (720x720 or 480x480)
- There's a bunch of boilerplate code for setting up the display
- Skipping down a bit, there's the main loop() that opens the gif "file" (really, the buffer in turbojorb90.h) and repeatedly calls the gif playFrame function to play the gif
- The bufferAndFlush function is really the only place I feel like I can take credit for doing real work! 

bufferAndFlush() is a callback function that the gif object calls repeatedly.  For every call to gif.playFrame() in the main loop, bufferAndFlush gets called 240 times - once per row.  You see, the gif library is meant to work on *extremely* memory-limited devices where you can't even store an entire frame in memory.  (The 4" display is 720x720x16 bits or just shy of a megabyte).  So the gif library we're using is set up to meter out pixels a row at a time so you can pipeline the process and avoid needing a massive buffer.  The ESP32 board this project uses doesn't have that problem!  The problem it has, though, is that the display is [single-buffered](https://en.wikipedia.org/wiki/Multiple_buffering).  This means that if we update the display a row at a time in separate transactions/function calls, we get worse screen tearing.  So, since we have enough RAM to do it, the code buffers an entire frame and writes to the screen only when it gets that 240th row of pixels.  The other thing bufferAndFlush() does is upscaling.  The source gif is 240x240, and making it larger increases the computational work required to decode it.  And there's really no point to making the original gif larger - it's not like I'm remastering it from a high-resolution original.  By upscaling from 240x240 to 720x720, the gif decoding function only has to handle *one ninth* as many pixels (or a fourth if you're using one of the 480x480 LCDs).  So, every time bufferAndFlush() gets a row of pixels, it stretches it out by the scaling factor (2 or 3) and then writes it to that many rows, too.  Oh, and the gif library has a baked-in size limitation that won't let you just use a 720x720 gif that's the native size of the display.  This is why it took me several evenings of tinkering to get it to work. :)

# Performance Considerations and Limitations
The notes on bufferAndFlush() above talk to the first couple tricks I pulled to get the jorb to look correct - simple upscaling and buffering an entire frame.  The last trick that finally made me happy enough to call it "good enough for now" was to rotate 90 degrees.  By aligning the motion of the jorb animation with the refresh scan of the display, any screen tearing just looks like a very slight stutter of the animation instead of an ugly seam splitting the image in half.  To make this truly work, however, I had to actually rotate the gif and the display itself - trying to do it by setting the screen rotation in software hurt the framerate because it added extra work!

This rotation is what I was alluding to in [this tweet](https://x.com/apieinorbit/status/1900405161346163106).  That's where the name of the header file came from - I sped up the gif and rotated it 90 degrees. 
![get rotated, idiot](turbojorb90.gif)

As I mentioned before, I tried doing the same thing with CircuitPython. It worked, but was not as fast.  I also had some difficulty getting the board to actually run CircuitPython and had to follow the Adafruit guide to [factory reset](https://learn.adafruit.com/adafruit-qualia-esp32-s3-for-rgb666-displays/factory-reset) the board and [install the CircuitPython uf2 file](https://learn.adafruit.com/adafruit-qualia-esp32-s3-for-rgb666-displays/circuitpython-5).  I'll put my CircuitPython version in this repo too, but I don't recommend it.

# Future Enhancements?
I've never looked under the hood in this ecosystem before, but I *think* the last of the tearing glitches could be eliminated if the graphics library supported double-buffering for this display.
https://github.com/moononournation/Arduino_GFX/blob/v1.5.5/src/databus/Arduino_ESP32RGBPanel.cpp#L94

This board also has a wifi module that this project does not utilize at all. I have a few ideas. :)
