#include <FastLED.h>
#include "ESP32_NOW_Serial.h"
#include "MacAddress.h"
#include "WiFi.h"
#include "esp_wifi.h"

#define NUM_LEDS 59
#define LED_PIN_1 D1

// FastLED brightness, 0 (min) to 255 (max)
#define BRIGHTNESS 255

#define digital_pir_sensor_1 D0

CRGB leds_1[NUM_LEDS];

unsigned long lastMotionTime_1 = 0;
const unsigned long motionTimeout = 5000; // 5 seconds

const int motionCountThreshold = 2; // Number of consecutive motion detections to trigger flicker effect
int motionCount_1 = 0;
bool meteorDirection = true; // true for right to left, false for left to right

unsigned long lastTriggerTime = 0;
const unsigned long triggerInterval = 5000; // 1秒内只触发一次效果
bool effectTriggered = false;

// 0: AP mode, 1: Station mode
#define ESPNOW_WIFI_MODE_STATION 1

// Channel to be used by the ESP-NOW protocol
#define ESPNOW_WIFI_CHANNEL 1

#if ESPNOW_WIFI_MODE_STATION          // ESP-NOW using WiFi Station mode
#define ESPNOW_WIFI_MODE WIFI_STA     // WiFi Mode
#define ESPNOW_WIFI_IF   WIFI_IF_STA  // WiFi Interface
#else                                 // ESP-NOW using WiFi AP mode
#define ESPNOW_WIFI_MODE WIFI_AP      // WiFi Mode
#define ESPNOW_WIFI_IF   WIFI_IF_AP   // WiFi Interface
#endif

// Set the MAC address of the device that will receive the data
// For example: F4:12:FA:40:64:4C
// 8c:bf:ea:cb:c5:3c
const MacAddress peer_mac_7({0x8C, 0xBF, 0xEA, 0xCC, 0xBD, 0xE4});
const MacAddress peer_mac_9({0x8C, 0xBF, 0xEA, 0xCC, 0xC1, 0x44});

ESP_NOW_Serial_Class NowSerial_7(peer_mac_7, ESPNOW_WIFI_CHANNEL, ESPNOW_WIFI_IF);
ESP_NOW_Serial_Class NowSerial_9(peer_mac_9, ESPNOW_WIFI_CHANNEL, ESPNOW_WIFI_IF);

void setAll(CRGB *leds, int numLeds, byte red, byte green, byte blue) {
  for(int i = 0; i < numLeds; i++ ) {
    setPixel(leds, i, red, green, blue);
  }
  showStrip();
}

void setPixel(CRGB *leds, int Pixel, byte red, byte green, byte blue) {
  leds[Pixel] = CRGB(red, green, blue);
}

void showStrip() {
  FastLED.show();
}

void meteorRain(CRGB *leds, int numLeds, byte meteorSize, byte meteorTrailDecay, boolean meteorRandomDecay, int SpeedDelay, bool rightToLeft) {  
  setAll(leds, numLeds, 0, 0, 0);
  for(int i = 0; i < numLeds+numLeds; i++) {
    // fade brightness all LEDs one step
    for(int j=0; j<numLeds; j++) {
      if( (!meteorRandomDecay) || (random(10)>5) ) {
        fadeToBlack(leds, j, meteorTrailDecay );        
      }
    }
    // draw meteor
    for(int j = 0; j < meteorSize; j++) {
      if(rightToLeft) {
        if( ( i-j <numLeds) && (i-j>=0) ) {
          setPixel(leds, i-j, random(256), random(256), random(256));
        }
      } else {
        if( ( i-j <numLeds) && (i-j>=0) ) {
          setPixel(leds, numLeds-1-(i-j), random(256), random(256), random(256));
        }
      }
    }
    showStrip();
    delay(SpeedDelay);
  }
}

void fadeToBlack(CRGB *leds, int ledNo, byte fadeValue) {
  leds[ledNo].fadeToBlackBy(fadeValue);
}

void randomFlicker(CRGB *leds, int numLeds) {
  for(int i=0; i<numLeds; i++) {
    if(random(10) < 2) {
      setPixel(leds, i, 255, 255, 255);
    } else {
      setPixel(leds, i, 0, 0, 0);
    }
  }
  showStrip();
  delay(100);
}

void setup()
{
  Serial.begin(115200);  // set baud rate as 9600

  pinMode(digital_pir_sensor_1, INPUT); // set Pin mode as input

  FastLED.addLeds<WS2812B, LED_PIN_1, GRB>(leds_1, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

  Serial.print("WiFi Mode: ");
  Serial.println(ESPNOW_WIFI_MODE == WIFI_AP ? "AP" : "Station");
  WiFi.mode(ESPNOW_WIFI_MODE);

  Serial.print("Channel: ");
  Serial.println(ESPNOW_WIFI_CHANNEL);
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

  while (!(WiFi.STA.started() || WiFi.AP.started())) {
    delay(100);
  }

  Serial.print("MAC Address: ");
  Serial.println(ESPNOW_WIFI_MODE == WIFI_AP ? WiFi.softAPmacAddress() : WiFi.macAddress());

  // Start the ESP-NOW communication
  Serial.println("ESP-NOW communication starting...");
  NowSerial_7.begin(115200);
  NowSerial_9.begin(115200);
  Serial.println("You can now send data to the peer device using the Serial Monitor.\n");
}

void loop()
{
  if (NowSerial_7.available()) {
    int receivedData = NowSerial_7.read();
    Serial.print("Received data from device 0: ");
    Serial.println(receivedData);

    if (receivedData == 1) {
      unsigned long currentTime = millis();
      if (!effectTriggered || (currentTime - lastTriggerTime >= triggerInterval)) {
        // 如果效果尚未触发或者距离上次触发已经超过了触发间隔,则触发效果
        meteorRain(leds_1, NUM_LEDS, 10, 64, true, 30, meteorDirection);
        // meteorDirection = !meteorDirection;
        effectTriggered = true;
        lastTriggerTime = currentTime;
      }

    // 发送 ESP-NOW 消息给下一个设备
    if (NowSerial_9.availableForWrite()) {
        if (NowSerial_9.write(receivedData) <= 0) {
          Serial.println("Failed to send data to device 2");
        } else {
          Serial.println("Send data to device 2 Successfully");
        }
      }
    }
  }
  // else {
    // bool state_1 = digitalRead(digital_pir_sensor_1); // read from PIR sensor 1
    // if (state_1 == 1){
    //   Serial.println("A Motion has occured on sensor 1");  // When there is a response
    //   motionCount_1++;
    //   if(motionCount_1 >= motionCountThreshold) {
    //     meteorRain(leds_1, NUM_LEDS, 10, 64, true, 30, meteorDirection);
    //     meteorDirection = !meteorDirection;
    //   } else {
    //     meteorRain(leds_1, NUM_LEDS, 10, 64, true, 30, true);
    //   }
    //   lastMotionTime_1 = millis();
    // }
    // else if (millis() - lastMotionTime_1 > motionTimeout) {
    //   Serial.println("Nothing Happened on sensor 1");  // Far from PIR sensor 1
    //   motionCount_1 = 0;
    //   setAll(leds_1, NUM_LEDS_1, 0, 0, 0); // Turn off all LEDs when no motion is detected
    // }
  // }
  else{
    Serial.println("Nothing Happened");  // Far from PIR sensor
    motionCount_1 = 0;
    setAll(leds_1, NUM_LEDS, 0, 0, 0); // Turn off all LEDs when no motion is detected
    delay(100);
  }
}