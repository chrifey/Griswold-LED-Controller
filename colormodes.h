// Copyright (c) 2016 @jake-b, @russp81, @toblum
// Griswold LED Lighting Controller

// Griswold is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as 
// published by the Free Software Foundation, either version 3 of 
// the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// Griswold is a fork of the LEDLAMP project at 
//        https://github.com/russp81/LEDLAMP_FASTLEDs

// The LEDLAMP project is a fork of the McLighting Project at
//        https://github.com/toblum/McLighting

// ***************************************************************************
// Color modes
// ***************************************************************************
//#include "definitions.h"

char* listStatusJSON();

extern WebSocketsServer webSocket;

// These functions originally displayed the color using a call to FastLed.show()
// This has been refactored out, theser functions now simply render into the
// leds[] array. The FastLed.show() call happens in the main loop now.
// Furthermore, the 'add glitter' option also refactored out to the main loop.

void addGlitter(fract8 chanceOfGlitter) {
  if (random8() < chanceOfGlitter) {
    leds[random16(NUM_LEDS)] +=
        CRGB(settings.glitter_color.red, settings.glitter_color.green,
             settings.glitter_color.blue);
  }
}

void rainbow() {
  // FastLED's built-in rainbow generator
  fill_rainbow(leds, NUM_LEDS, gHue, 7);

  // if (settings.glitter_on == true){addGlitter(settings.glitter_density);}
  // frame has been created, now show it
  // FastLED.show();
  // insert a delay to keep the framerate modest
  // FastLED.delay(int(float(500/settings.fps)));
}

void confetti() {
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy(leds, NUM_LEDS, settings.ftb_speed);
  for (int x=0; x<settings.confetti_dens; x++) {
    int pos = random16(NUM_LEDS);
    leds[pos] += CHSV(gHue + random8(64), 200, settings.effect_brightness);
  }
  // if (settings.glitter_on == true){addGlitter(settings.glitter_density);}
  // frame has been created, now show it
  // FastLED.show();
  // insert a delay to keep the framerate modest
  // FastLED.delay(int(float(1000/settings.fps)));
}

void sinelon() {
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy(leds, NUM_LEDS, settings.ftb_speed);
  int pos = beatsin16(13, 0, NUM_LEDS);
  leds[pos] += CHSV(gHue, 255, settings.effect_brightness);
  // if (settings.glitter_on == true){addGlitter(settings.glitter_density);}
  // frame has been created, now show it
  // FastLED.show();
  // insert a delay to keep the framerate modest
  // FastLED.delay(int(float(1000/settings.fps)));
}

void bpm() {
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8(BeatsPerMinute, 64, settings.effect_brightness);
  for (int i = 0; i < NUM_LEDS; i++) {  // 9948
    leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
  }
  // if (settings.glitter_on == true){addGlitter(settings.glitter_density);}

  // frame has been created, now show it
  // FastLED.show();
  // insert a delay to keep the framerate modest
  // FastLED.delay(int(float(1000/settings.fps)));
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy(leds, NUM_LEDS, settings.ftb_speed);
  int dothue = 0;
  for (int i = 0; i < 8; i++) {
    leds[beatsin16(i + 7, 0, NUM_LEDS)] |=
        CHSV(dothue, 200, settings.effect_brightness);
    dothue += 32;
  }
  // if (settings.glitter_on == true){addGlitter(settings.glitter_density);}

  // frame has been created, now show it
  // FastLED.show();
  // insert a delay to keep the framerate modest
  // FastLED.delay(int(float(1000/settings.fps)));
}
//******************************************************************************************
//                     PALETTE ANIMATION FUNCTIONS
//******************************************************************************************
int wipeInProgress = 0;

void FillLEDsFromPaletteColors(CRGBPalette16 palette, uint8_t paletteStartIndex, uint16_t endingLEDIndex=0xFFFF) {
  uint8_t colorIndex = paletteStartIndex;

  for (int i = 0; i < NUM_LEDS; i++) {
    if (i > endingLEDIndex) return;  //stop condition
    
    // leds[i] = ColorFromPalette( currentPalette, colorIndex + sin8(i*16),
    // brightness);
    leds[i] = ColorFromPalette(palette, colorIndex,
                               settings.effect_brightness);
    if (anim_direction == FORWARD) {
      colorIndex += 3;
    }
    if (anim_direction == BACK) {
      colorIndex -= 3;
    }
  }
}

void ChangePalettePeriodically(bool forceNow) {
  if (forceNow || millis() - paletteMillis > (settings.show_length * 1000)) {
    paletteMillis = millis();

    targetPaletteIndex = random(0, paletteCount);

    currentPalette = targetPalette;

    anim_direction = (DIRECTION)!anim_direction; // DIRECTION enum allows flipping by boolean not.

    loadPaletteFromFile(targetPaletteIndex, &targetPalette);

    DBG_OUTPUT_PORT.printf("New pallet index: %d\n", targetPaletteIndex);

    if (settings.glitter_wipe_on) {
       DBG_OUTPUT_PORT.println("Begin glitter wipe");
       wipeInProgress = true;
    }
  }
}

void colorWipe() {
  static CRGB prevColor = CHSV(gHue, 255, settings.effect_brightness);
  static CRGB currentColor = CHSV(gHue+60, 255, settings.effect_brightness);

  // Wrap around if necessary
  if (wipePos >= NUM_LEDS) {
    wipePos = 0;
    prevColor = currentColor;
    gHue += 60;
    currentColor = CHSV(gHue, 255, settings.effect_brightness);
  }
  
  // Render the first half of the wipe
  for (int x=0; x<wipePos; x++) {
    leds[x] = currentColor;
  }
  // Render the second half
  for (int x=wipePos; x<NUM_LEDS; x++) {
    leds[x] = prevColor;
  }

  //Render the glitter at the intersection
  if (settings.glitter_wipe_on) {
    for (int x=0; x < 3; x++) {
      int speckle = wipePos + random(-SPARKLE_SPREAD,SPARKLE_SPREAD);
      if (speckle >= 0 && speckle < NUM_LEDS) {
          leds[speckle] +=  CRGB(settings.glitter_color.red, settings.glitter_color.green,
               settings.glitter_color.blue);
      }    
    }
  }
  
  // Advance for next frame
  wipePos+=WIPE_SPEED;
}

void palette_anims() {
  currentBlending = LINEARBLEND;

  if (settings.palette_ndx == -1) ChangePalettePeriodically(false);

  if (!settings.glitter_wipe_on) {
    uint8_t maxChanges = int(float(settings.fps / 2));
    nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);

    // Update the current palette if necessary-- and send to any connected clients.
    if (currentPaletteIndex != targetPaletteIndex) {
      currentPaletteIndex = targetPaletteIndex;
    
      // Send current palette name to the UI.
      String name = getPaletteNameWithIndex(currentPaletteIndex);
      webSocket.broadcastTXT("p"+name);
    }
  }
  
  static uint8_t startIndex = 0;

  /* motion speed */
  startIndex = startIndex + 3;

  FillLEDsFromPaletteColors(currentPalette,startIndex);

  if (settings.glitter_wipe_on && wipeInProgress) {
    if (wipePos >= NUM_LEDS) {
      DBG_OUTPUT_PORT.println("End glitter wipe");
      wipeInProgress = false;
      wipePos = 0;
      currentPalette = targetPalette;
      currentPaletteIndex = targetPaletteIndex;

      // Send current palette name to the UI.
      String name = getPaletteNameWithIndex(currentPaletteIndex);
      webSocket.broadcastTXT("p"+name);      
      FillLEDsFromPaletteColors(targetPalette,startIndex);
    } else {
      FillLEDsFromPaletteColors(targetPalette,startIndex, wipePos);
      for (int x=0; x < 3; x++) {
        int speckle = wipePos + random(-SPARKLE_SPREAD,SPARKLE_SPREAD);
        if (speckle >= 0 && speckle < NUM_LEDS) {
            leds[speckle] +=  CRGB(settings.glitter_color.red, settings.glitter_color.green,
                 settings.glitter_color.blue);
        }  
      }
      wipePos+=WIPE_SPEED;
    }
  }

  
}

//*****************LED
//RIPPLE*****************************************************

void one_color_allHSV(int ahue,
                      int abright) {  // SET ALL LEDS TO ONE COLOR (HSV)
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(ahue, 255, abright);
  }
}

int wrap(int step) {
  if (step < 0) return NUM_LEDS + step;
  if (step > NUM_LEDS - 1) return step - NUM_LEDS;
  return step;
}

void ripple() {
  if (currentBg == nextBg) {
    nextBg = random(256);
  } else if (nextBg > currentBg) {
    currentBg++;
  } else {
    currentBg--;
  }
  for (uint16_t l = 0; l < NUM_LEDS; l++) {
    leds[l] = CHSV(currentBg, 255,
                   settings.effect_brightness);  // strip.setPixelColor(l,
                                                 // Wheel(currentBg, 0.1));
  }

  if (step == -1) {
    center = random(NUM_LEDS);
    color = random(256);
    step = 0;
  }

  if (step == 0) {
    leds[center] = CHSV(
        color, 255, settings.effect_brightness);  // strip.setPixelColor(center,
                                                  // Wheel(color, 1));
    step++;
  } else {
    if (step < maxSteps) {
      //Serial.println(pow(fadeRate, step));

      leds[wrap(center + step)] =
          CHSV(color, 255,
               pow(fadeRate, step) * 255);  //   strip.setPixelColor(wrap(center
                                            //   + step), Wheel(color,
                                            //   pow(fadeRate, step)));
      leds[wrap(center - step)] =
          CHSV(color, 255,
               pow(fadeRate, step) * 255);  //   strip.setPixelColor(wrap(center
                                            //   - step), Wheel(color,
                                            //   pow(fadeRate, step)));
      if (step > 3) {
        leds[wrap(center + step - 3)] =
            CHSV(color, 255, pow(fadeRate, step - 2) *
                                 255);  //   strip.setPixelColor(wrap(center +
                                        //   step - 3), Wheel(color,
                                        //   pow(fadeRate, step - 2)));
        leds[wrap(center - step + 3)] =
            CHSV(color, 255, pow(fadeRate, step - 2) *
                                 255);  //   strip.setPixelColor(wrap(center -
                                        //   step + 3), Wheel(color,
                                        //   pow(fadeRate, step - 2)));
      }
      step++;
    } else {
      step = -1;
    }
  }
  // if (settings.glitter_on == true){addGlitter(settings.glitter_density);}

  // frame has been created, now show it
  // FastLED.show();
  // insert a delay to keep the framerate modest
  // FastLED.delay(int(float(1000/settings.fps)));
}

//***************************END LED
//RIPPLE*****************************************************

void comet() {
  fadeToBlackBy(leds, NUM_LEDS, settings.ftb_speed);
  lead_dot = beatsin16(int(float(settings.fps / 3)), 0, NUM_LEDS);
  leds[lead_dot] = CHSV(dothue, 200, 255);
  dothue += 8;
  // if (settings.glitter_on == true){addGlitter(settings.glitter_density);}
  // FastLED.show();
}

// Theatre-style crawling lights.
void theaterChase() {
  static int8_t frame = 0;

  // turn off the previous frame's led
  for (int i = 0; i < NUM_LEDS; i = i + 3) {
    if (i + frame < NUM_LEDS) {
      leds[i + frame] = CRGB(0, 0, 0);  // turn every third pixel off
    }
  }

  // advance the frame
  frame++;
  if (frame > 2) frame = 0;

  // turn on the current frame's leds
  for (int i = 0; i < NUM_LEDS; i = i + 3) {
    if (i + frame < NUM_LEDS) {
      leds[i + frame] =
          CRGB(settings.main_color.red, settings.main_color.green,
               settings.main_color.blue);  // turn every third pixel on
    }
  }
}


//***********TV
int dipInterval = 10;
int darkTime = 250;
unsigned long currentDipTime;
unsigned long dipStartTime;
unsigned long currentMillis;
int ledState = LOW;
long previousMillis = 0; 
int ledBrightness[NUM_LEDS];
uint16_t ledHue[NUM_LEDS];
int led = 5;
int interval = 2000;
int twitch = 50;
int dipCount = 0;
int analogLevel = 100;
boolean timeToDip = false;

CRGB hsb2rgbAN1(uint16_t index, uint8_t sat, uint8_t bright) {
    // Source: https://blog.adafruit.com/2012/03/14/constant-brightness-hsb-to-rgb-algorithm/
    uint8_t temp[5], n = (index >> 8) % 3;
    temp[0] = temp[3] = (uint8_t)((                                        (sat ^ 255)  * bright) / 255);
    temp[1] = temp[4] = (uint8_t)((((( (index & 255)        * sat) / 255) + (sat ^ 255)) * bright) / 255);
    temp[2] =          (uint8_t)(((((((index & 255) ^ 255) * sat) / 255) + (sat ^ 255)) * bright) / 255);

    return CRGB(temp[n + 2], temp[n + 1], temp[n]);
}

void _tvUpdateLed (int led, int brightness) {
  ledBrightness[led] = brightness;
  for (int i=0; i<NUM_LEDS; i++) {
    uint16_t index = (i%3 == 0) ? 400 : random(0,767);
    ledHue[led] = index;
  }
}

// See: http://forum.mysensors.org/topic/85/phoneytv-for-vera-is-here/13
void tv() {
  if (timeToDip == false) {
    currentMillis = millis();
    if (currentMillis-previousMillis > interval)  {
      previousMillis = currentMillis;
      interval = random(750,4001);//Adjusts the interval for more/less frequent random light changes
      twitch = random(40,100);// Twitch provides motion effect but can be a bit much if too high
      dipCount++;      
    }
    if (currentMillis-previousMillis<twitch) {
      led=random(0, NUM_LEDS-1);
      analogLevel=random(50,255);// set the range of the 3 pwm leds
      ledState = ledState == LOW ? HIGH: LOW; // if the LED is off turn it on and vice-versa:
      
      
      _tvUpdateLed(led, (ledState) ? 255 : 0);
      
      if (dipCount > dipInterval) { 
        //DBG_OUTPUT_PORT.println("dip");
        timeToDip = true;
        dipCount = 0;
        dipStartTime = millis();
        darkTime = random(50,150);
        dipInterval = random(5,250);// cycles of flicker
      }
    } 
  } else {
    //DBG_OUTPUT_PORT.println("Dip Time");
    currentDipTime = millis();
    if (currentDipTime - dipStartTime < darkTime) {
      for (int i=3; i<NUM_LEDS; i++) {
        _tvUpdateLed(i, 0);
      }
    } else {
      timeToDip = false;
    }
  }

  // Render the thing, with a little flicker  
  uint8_t flicker = 255;
  int sat = 200;

  EVERY_N_MILLISECONDS(150) {
    flicker = random(220,255);
    sat = random(180, 220);
  }
  
  for (int i=0; i<NUM_LEDS; i++) {
    uint16_t index = (i%3 == 0) ? 400 : random(0,767);
    //leds[i] = ((index >> 8) % 3, 200, ledBrightness[i]);
    
    leds[i] = hsb2rgbAN1(ledHue[i], sat, ledBrightness[i]).nscale8_video(flicker);
  }
}


//*******************************ARRAY OF SHOW ANIMATIONS FOR MIXED SHOW
//MODE***********************
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = {rainbow, confetti,      sinelon, juggle,
                               bpm,     palette_anims, ripple,  comet};
//**************************************************************************************************
