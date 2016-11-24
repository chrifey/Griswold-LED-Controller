#include "definitions.h"
#include "mycolorpalettes.h"
#include "eepromsettings.h"

// ***************************************************************************
// Load libraries for: WebServer / WiFiManager / WebSockets
// ***************************************************************************
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

// needed for library WiFiManager
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <ArduinoOTA.h>

#include <WebSockets.h>           //https://github.com/Links2004/arduinoWebSockets
#include <WebSocketsServer.h>


// ***************************************************************************
// Instanciate HTTP(80) / WebSockets(81) Server
// ***************************************************************************
ESP8266WebServer server ( 80 );
WebSocketsServer webSocket = WebSocketsServer(81);


// ***************************************************************************
// Load library "ticker" for blinking status led
// ***************************************************************************
#include <Ticker.h>
Ticker ticker;

void tick()
{
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}


// ***************************************************************************
// Callback for WiFiManager library when config mode is entered
// ***************************************************************************
//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  DBG_OUTPUT_PORT.println("Entered config mode");
  DBG_OUTPUT_PORT.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  DBG_OUTPUT_PORT.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}




// ***************************************************************************
// Include: Webserver
// ***************************************************************************
#include "spiffs_webserver.h"

// ***************************************************************************
// Include: Request handlers
// ***************************************************************************
#include "request_handlers.h"

// ***************************************************************************
// Include: Color modes
// ***************************************************************************
#include "colormodes.h"





// ***************************************************************************
// MAIN
// ***************************************************************************
void setup() {
  // ***************************************************************************
  // Setup: EEPROM
  // ***************************************************************************
  initSettings();  // setting loaded from EEPROM or defaults if fail
  printSettings();
  
  //********color palette setup stuff****************
  currentPalette = RainbowColors_p;
  targetPalette = bhw1_purpgreen_gp;
  currentBlending = LINEARBLEND;
  //**************************************************    
  
  DBG_OUTPUT_PORT.begin(115200);

  // set builtin led pin as output
  pinMode(BUILTIN_LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

  // ***************************************************************************
  // Setup: WiFiManager
  // ***************************************************************************
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect(HOSTNAME)) {
    DBG_OUTPUT_PORT.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  DBG_OUTPUT_PORT.println("connected...yeey :)");
  ticker.detach();
  //keep LED on
  digitalWrite(BUILTIN_LED, LOW);
  
  // ***************************************************************************
  // Setup: WiFiManager
  // ***************************************************************************
   ArduinoOTA.setHostname(HOSTNAME);
   ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    DBG_OUTPUT_PORT.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    DBG_OUTPUT_PORT.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    DBG_OUTPUT_PORT.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    DBG_OUTPUT_PORT.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) DBG_OUTPUT_PORT.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) DBG_OUTPUT_PORT.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) DBG_OUTPUT_PORT.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) DBG_OUTPUT_PORT.println("Receive Failed");
    else if (error == OTA_END_ERROR) DBG_OUTPUT_PORT.println("End Failed");
  });
  
  ArduinoOTA.begin();
  DBG_OUTPUT_PORT.println("OTA Ready");
  DBG_OUTPUT_PORT.print("IP address: ");
  DBG_OUTPUT_PORT.println(WiFi.localIP());

  // ***************************************************************************
  // Setup: FASTLED
  // ***************************************************************************
  delay(3000); // 3 second delay for recovery
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(settings.brightness);


  // ***************************************************************************
  // Setup: MDNS responder
  // ***************************************************************************
  MDNS.begin(HOSTNAME);
  DBG_OUTPUT_PORT.print("Open http://");
  DBG_OUTPUT_PORT.print(HOSTNAME);
  DBG_OUTPUT_PORT.println(".local/edit to see the file browser");


  // ***************************************************************************
  // Setup: WebSocket server
  // ***************************************************************************
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);


  // ***************************************************************************
  // Setup: SPIFFS
  // ***************************************************************************
  SPIFFS.begin();
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      DBG_OUTPUT_PORT.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    DBG_OUTPUT_PORT.printf("\n");
  }

  // ***************************************************************************
  // Setup: SPIFFS Webserver handler
  // ***************************************************************************
  //list directory
  server.on("/list", HTTP_GET, handleFileList);
  //load editor
  server.on("/edit", HTTP_GET, []() {
    if (!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");
  });
  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, []() {
    server.send(200, "text/plain", "");
  }, handleFileUpload);
  //get heap status, analog input value and all GPIO statuses in one json call
  server.on("/esp_status", HTTP_GET, []() {
    String json = "{";
    json += "\"heap\":" + String(ESP.getFreeHeap());
    json += ", \"analog\":" + String(analogRead(A0));
    json += ", \"gpio\":" + String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16)));
    json += "}";
    server.send(200, "text/json", json);
    json = String();
  });


  //called when the url is not defined here
  //use it to load content from SPIFFS
  server.onNotFound([]() {
    if (!handleFileRead(server.uri()))
      handleNotFound();
  });

  server.on("/upload", handleMinimalUpload);

  server.on("/restart", []() {
    DBG_OUTPUT_PORT.printf("/restart:\n");
    server.send(200, "text/plain", "restarting..." );
    ESP.restart();
  });

  server.on("/reset_wlan", []() {
    DBG_OUTPUT_PORT.printf("/reset_wlan:\n");
    server.send(200, "text/plain", "Resetting WLAN and restarting..." );
    WiFiManager wifiManager;
    wifiManager.resetSettings();
    ESP.restart();
  });


  // ***************************************************************************
  // Setup: SPIFFS Webserver handler
  // ***************************************************************************
  server.on("/set_brightness", []() {
    if (server.arg("c").toInt() > 0) {
      settings.brightness = (int) server.arg("c").toInt() * 2.55;
    } else {
      settings.brightness = server.arg("p").toInt();
    }
    if (settings.brightness > 255) {
      settings.brightness = 255;
    }
    if (settings.brightness < 0) {
      settings.brightness = 0;
    }
    FastLED.setBrightness(settings.brightness);

    if (settings.mode == HOLD) {
      settings.mode = ALL;
    }

    getStatusJSON();
  });

  server.on("/get_brightness", []() {
    String str_brightness = String((int) (settings.brightness / 2.55));
    server.send(200, "text/plain", str_brightness );
    DBG_OUTPUT_PORT.print("/get_brightness: ");
    DBG_OUTPUT_PORT.println(str_brightness);
  });

  server.on("/get_switch", []() {
    server.send(200, "text/plain", (settings.mode == OFF) ? "0" : "1" );
    DBG_OUTPUT_PORT.printf("/get_switch: %s\n", (settings.mode == OFF) ? "0" : "1");
  });

  server.on("/get_color", []() {
    String rgbcolor = String(settings.main_color.red, HEX) + String(settings.main_color.green, HEX) + String(settings.main_color.blue, HEX);
    server.send(200, "text/plain", rgbcolor );
    DBG_OUTPUT_PORT.print("/get_color: ");
    DBG_OUTPUT_PORT.println(rgbcolor);
  });

  server.on("/status", []() {
    getStatusJSON();
  });

  server.on("/off", []() {
    exit_func = true;
    settings.mode = OFF;
    getArgs();
    getStatusJSON();
  });

  server.on("/all", []() {
    exit_func = true;
    settings.mode = ALL;
    getArgs();
    getStatusJSON();
  });
      
  server.on("/rainbow", []() {
    exit_func = true;
    settings.mode = RAINBOW;
    getArgs();
    getStatusJSON();
  });

  server.on("/confetti", []() {
    exit_func = true;
    settings.mode = CONFETTI;
    getArgs();
    getStatusJSON();
  });

  server.on("/sinelon", []() {
    exit_func = true;
    settings.mode = SINELON;
    getArgs();
    getStatusJSON();
  });

  server.on("/juggle", []() {
    exit_func = true;
    settings.mode = JUGGLE;
    getArgs();
    getStatusJSON();
  });

  server.on("/bpm", []() {
    exit_func = true;
    settings.mode = BPM;
    getArgs();
    getStatusJSON();
  });

  server.on("/ripple", []() {
    exit_func = true;
    settings.mode = RIPPLE;
    getArgs();
    getStatusJSON();
  });  

  server.on("/comet", []() {
    exit_func = true;
    settings.mode = COMET;
    getArgs();
    getStatusJSON();
  });   

  server.begin();
}

void loop() {
  server.handleClient();
  webSocket.loop();
  ArduinoOTA.handle();
//  yield();

  EVERY_N_MILLISECONDS(int(float(1000/settings.fps)))  { gHue++; } // slowly cycle the "base color" through the rainbow

  // Simple statemachine that handles the different modes
  if (settings.mode == OFF) {
    uint16_t i;
    for (i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB(0, 0, 0);
    }
    if (settings.glitter_on == true){addGlitter(settings.glitter_density);}
    FastLED.show();
    FastLED.delay(int(float(1000/settings.fps)));
  }
  if (settings.mode == ALL) {
    uint16_t i;
    for (i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB(settings.main_color.red, settings.main_color.green, settings.main_color.blue);
    }
    if (settings.glitter_on == true){addGlitter(settings.glitter_density);}
    FastLED.show();
    FastLED.delay(int(float(1000/settings.fps)));
  }

  if (settings.mode == MIXEDSHOW) {
    gPatterns[gCurrentPatternNumber]();
    // send the 'leds' array out to the actual LED strip
    int showlength_Millis = settings.show_length*1000;
    //DBG_OUTPUT_PORT.println("showlengthmillis = " + String(showlength_Millis));
      if (((millis()) - (lastMillis)) >= showlength_Millis) {
        nextPattern(); 
        DBG_OUTPUT_PORT.println("void nextPattern was called at " + String(millis()) + " and the current show length set to " + String(showlength_Millis));
      } 
    }

  if (settings.mode == RAINBOW) {
    rainbow();
  }

  if (settings.mode == CONFETTI) {
    confetti();
  }
  
  if (settings.mode == SINELON) {
    sinelon();
  }
  
  if (settings.mode == JUGGLE) {
    juggle();
  }
  
  if (settings.mode == BPM) {
    bpm();
  }   

  if (settings.mode == PALETTE_ANIMS){
    palette_anims();
  }

  if (settings.mode == RIPPLE){
    ripple();
  }  

  if (settings.mode == COMET){
    comet();
  }  

  if (settings.mode == THEATERCHASE){
    theaterChase();
  }   

}

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
//  gCurrentPatternNumber = (gCurrentPatternNumber + random(0, ARRAY_SIZE(gPatterns))) % ARRAY_SIZE( gPatterns);
   gCurrentPatternNumber = random(0, ARRAY_SIZE(gPatterns));
   lastMillis = millis();
  
}

