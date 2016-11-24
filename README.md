# Jake's "Griswold" LED controller for Christmas Lights.

I bought 1000 WS2811 nodes for my outdoor Christmas light installation this year.
Based on the "Russell's FASTLEDs" project by @russp81, which is in turn based on the "McLighting" project by @toblum

It seemed necessary to name the thing after Clark Griswold, but really just to differentiate this fork from the originals.

![Clark Griswold](http://i.giphy.com/gB9wIPXav2Ryg.gif)

@russp81 mixed the work of @toblum with the @FastLED (FastLED library 3.1.3 as of this writing), the colorjs colorpicker, color spectrums created via FastLED Palette Knife, and some additional strip animations.

Improvements:

- Added ArduinoOTA support so I can update the firmware over WiFi, which will be important when its installed outside.
- Added the ability to store the settings in EEPROM and restore on boot.
- Merged the jscolor interface into the original McLighting interface
- Updated the McLighting interface to retrieve the current settings from the device, and update the UI with the current settings, rather than always default to the defaults.


Russell's FASTLEDs:
https://github.com/russp81/LEDLAMP_FASTLEDs

FastLED 3.1.3 library:
https://github.com/FastLED/FastLED

McLighting library:
https://github.com/toblum/McLighting

jscolor Color Picker:
http://jscolor.com/

FastLED Palette Knife:
http://fastled.io/tools/paletteknife/

# Portions of @russp81's original README

If you aren't familiar with how to setup your ESP8266, see the readme on McLighting's git.  It's well written and should get you up and running.

In short you will:

1.  Configure the Arduino IDE to communicate with the ESP8266
2.  Upload the sketch (from this repo) The sketch is setup for a 240 pixel WS2812B GRB LED Strip.   
    (change the applicable options in "definitions.h" to your desire)
3.  On first launch, the ESP8266 will advertise it's own WiFi network for you to connect to, once you connect to it, launch your browser
    and the web interface is self explanatory.  (If the interface doesn't load, type in "192.168.4.1" into your browser and hit go)
4.  Once the ESP is on your wifi network, you can then upload the required files for the web interface by typing the in IP address
    of the ESP followed by "/edit" (i.e. 192.168.1.20/edit).  Then upload the files from the folder labeled "upload these" from this         repo. 
5.  Once you have finished uploading, type in the IP of the ESP into your browser and you should be up and running!


My work was all on adding FastLED (and other tweaks / animations) into the McLighting sketch instead of using Adafruit NeoPixel.

I am a self taught coder who learns by a few books, google, and looking at other's code, 
and I just liked the things you can do in FastLED better, so I decided to tackle the 
idea of integrating FastLED into the already awesome work of @toblum.

I have a limited grasp on the h/w and s/w relationships (do's and don't do's, etc).  
I edited clockless_esp8266.h (in the FastLED platforms folder) and 
kept getting flickering until I incremented the WAIT_TIME up to 18us. 
(also I did "#define FASTLED_INTERRUPT_RETRY_COUNT 3" inside my sketch).

If I disabled interrupts altogether "#define FASTLED_ALLOW_INTERRUPTS 0", the strip would stop flickering but I would get
what I believe to be "watchdog resets" every 5 to 20 minutes depending on what animation was running, wifi traffic, etc...

For reference, I learned more about the interrupts issue from here:  https://github.com/FastLED/FastLED/issues/306

If anyone can shed more light on this I am all ears!  I'm not sure exactly what the implications are
for setting the WAIT_TIME = 18us??  Everything appears to function properly, and so far I have not seen 
a reset in a few hours.

Also, I added a separate color picker from (http://jscolor.com/).  My idea with this is to eventually create
spectrums using multiple color pickers via the web interface (instead of grinding out coding in the Arduino IDE)
and eventually animate them as well.  When I am finished with this project, I (we) will hopefully be able to build those
spectrums, save them to the ESP8266 flash memory, and have a universal routine in the Arduino Sketch that can handle 
the input / output of the spectrums to the strip (even running animations with them).  I also might even try making a web interface
to create custom animations from, but that seems like a pretty decent challenge from what I can tell. (I am just now finding my way 
around in html/css/js so I have A LOT of learning to do!)

I will say again, I'm a rookie who has tinkered around in a little of each major language with no formal education, so 
if you see something that doesn't look right, it probably isn't!  I am very open to suggestions / learning anything 
anyone is willing to share.

