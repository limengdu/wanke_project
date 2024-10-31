// 导入必要的库
#include <FastLED.h>
#include "ESP32_NOW_Serial.h"
#include "MacAddress.h"
#include "WiFi.h"
#include "esp_wifi.h"


/********************************* 灯相关 *********************************/
#define NUM_LEDS 59
#define LED_PIN D1
#define BRIGHTNESS 255                    // FastLED brightness, 0 (min) to 255 (max)
CRGB leds[NUM_LEDS];


/********************************* PIR相关 *********************************/
#define digital_pir_sensor D0
const unsigned long motionTimeout = 5000; // 5 seconds

const int motionCountThreshold = 2;       // PIR触发持续有人的阈值
bool meteorDirection = true;              // 控制流水灯的流转方向


/********************************* 不倒翁控制相关 *********************************/
int taskflow = 0;                         // 控制当前任务流
uint8_t need_num_xiao, need_extra_leds;   // 0号位上报的灯相关信息
bool tumbler_done = false;                // 标记不倒翁效果是否完成
bool tumbler_executed = false;            // 标记不倒翁效果是否已经执行过


/********************************* ESP-NOW 相关 *********************************/
#define ESPNOW_WIFI_MODE_STATION 1    // 0: AP mode, 1: Station mode
#define ESPNOW_WIFI_CHANNEL 1         // Channel to be used by the ESP-NOW protocol
#if ESPNOW_WIFI_MODE_STATION          // ESP-NOW using WiFi Station mode
#define ESPNOW_WIFI_MODE WIFI_STA     // WiFi Mode
#define ESPNOW_WIFI_IF   WIFI_IF_STA  // WiFi Interface
#else                                 // ESP-NOW using WiFi AP mode
#define ESPNOW_WIFI_MODE WIFI_AP      // WiFi Mode
#define ESPNOW_WIFI_IF   WIFI_IF_AP   // WiFi Interface
#endif

// Set the MAC address of the device that will receive the data
const MacAddress peer_mac_2({0x8C, 0xBF, 0xEA, 0xCB, 0x80, 0x60});
const MacAddress peer_mac_4({0x8C, 0xBF, 0xEA, 0xCB, 0x80, 0xC8});

ESP_NOW_Serial_Class NowSerial_2(peer_mac_2, ESPNOW_WIFI_CHANNEL, ESPNOW_WIFI_IF);
ESP_NOW_Serial_Class NowSerial_4(peer_mac_4, ESPNOW_WIFI_CHANNEL, ESPNOW_WIFI_IF);


/********************************* 灯效相关函数 *********************************/
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

void fadeToBlack(CRGB *leds, int ledNo, byte fadeValue) {
  leds[ledNo].fadeToBlackBy(fadeValue);
}

// PIR 流水灯效
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

// 作为不倒翁击倒灯效
void LightUpLeds(CRGB *leds, int numLeds) {
  for (int i = 0; i < numLeds; i++) {
    leds[i] = CRGB(sin(i + millis() / 100.0) * 127 + 128, random(0, 255), random(0, 255));
  }
  // 确保 numLeds 之后的灯珠全部熄灭
  for (int i = numLeds; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
  showStrip();
  delay(500);
}

// 逐颗熄灭LED
void fadeOutLeds(CRGB *leds, int numLeds) {
  for (int i = numLeds - 1; i >= 0; i--) {
    leds[i] = CRGB::Black;
    showStrip();
    delay(500);
  }
}

// 不倒翁持续一会的灯效
// void RunningLights(byte red, byte green, byte blue, int numLeds) {
void RunningLights(int numLeds) {
  int Position=0;
  for(int j = 0; j < numLeds * 2; j++)
  {
    Position++; // = 0; //Position + Rate;
    for(int i=0; i < numLeds; i++) {
      // sine wave, 3 offset waves make a rainbow!
      float level = sin(i+Position) * 127 + 128;
      setPixel(leds, i,level,0,0);
      // setPixel(i,((sin(i+Position) * 127 + 128)/255)*red,
      //             ((sin(i+Position) * 127 + 128)/255)*green,
      //             ((sin(i+Position) * 127 + 128)/255)*blue);
    }
    showStrip();
    delay(500);
  }
}


/********************************* PIR函数 *********************************/
int motionCount = 0;                    // 用于一直有人时持续触发的灯光效果
unsigned long lastMotionTime = 0;       // 用于避免同一次PIR反复触发灯光效果

void handlePIR(int pirSensor, CRGB *leds, int numLeds) {
  bool state = digitalRead(pirSensor); // read from PIR sensor
  if (state == 1) {
    Serial.println("A Motion has occured on PIR sensor.");
    motionCount++;
    if (motionCount >= motionCountThreshold) {
      meteorRain(leds, numLeds, 10, 64, true, 30, meteorDirection);
      meteorDirection = !meteorDirection;
    } else {
        meteorDirection = true;
        meteorRain(leds, numLeds, 10, 64, true, 30, meteorDirection);
      }
    
    lastMotionTime = millis();
  }
  else if (millis() - lastMotionTime > motionTimeout) {
    Serial.println("Nothing Happened on PIR sensor.");
    motionCount = 0;
    // setAll(leds, numLeds, 0, 0, 0); // Turn off all LEDs when no motion is detected
  }
}


/********************************* Arduino *********************************/
void setup()
{
  Serial.begin(115200);               // set baud rate as 115200

  pinMode(digital_pir_sensor, INPUT); // set Pin mode as input

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
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
  NowSerial_2.begin(115200);
  NowSerial_4.begin(115200);
  Serial.println("XIAO NUM 3 Ready.\n");
}


void loop()
{
  // XIAO 2 的消息到达（优先级最高）
  if (NowSerial_2.available() >= 0 && !tumbler_executed) {
    int bytesRead = NowSerial_2.readBytes(&need_num_xiao, 1);
    bytesRead += NowSerial_2.readBytes(&need_extra_leds, 1);
    if (bytesRead == 2) {
      Serial.print("Received array from XIAO 2: ");

      // 获得灯条相关的信息
      Serial.print("Need Num of XIAO: "); Serial.println(need_num_xiao);
      Serial.print("Need Num of extra LEDS: "); Serial.println(need_extra_leds);
      taskflow = 2; // 切换到不倒翁工作流
      tumbler_done = false; // 标记不倒翁效果尚未完成
    }
  }

  // 不倒翁效果结束的消息传来
  int receivedData_4 = 1;
  if (NowSerial_4.available()) {
    receivedData_4 = NowSerial_4.read();
    Serial.print("Received data from XIAO 4: ");
    Serial.println(receivedData_4);
  }
  
  // 如果收到来自 XIAO 4 的消息,说明不倒翁效果已完成
  if (!receivedData_4) {
    tumbler_done = true; // 标记不倒翁效果已完成
    // 发送消息给XIAO 2,通知不倒翁效果已完成
    if (NowSerial_2.availableForWrite()) {
      NowSerial_2.write(0);
      Serial.println("Sent tumbler done message to XIAO 2");
    }
    fadeOutLeds(leds, need_extra_leds); // 熄灭灯光
  }

  switch(taskflow) {
    case 1:                    // PIR工作流
      handlePIR(digital_pir_sensor, leds, NUM_LEDS);
      break;
    case 2:                    // 不倒翁工作流
      if (!tumbler_executed) { // 如果不倒翁效果尚未执行过
        need_num_xiao -= 1;
        if (need_num_xiao > 0) {
          // 先发送 ESP-NOW 消息给下一个设备
          if (NowSerial_4.availableForWrite()) {
            int bytesWritten = NowSerial_4.write(need_num_xiao);
            bytesWritten += NowSerial_4.write(need_extra_leds);
            if (bytesWritten == 2) {
              Serial.println("Send data to XIAO 4 Successfully");
            } else {
              Serial.println("Failed to send data to XIAO 4");
            }
          }
          // 再点亮灯条
          LightUpLeds(leds, NUM_LEDS);
          // delay(1000);
          // RunningLights(need_extra_leds);
        } else {
          // 如果不用点亮下个灯珠,发送0给下个设备
          if (NowSerial_4.availableForWrite()) {
            NowSerial_4.write(0);
            NowSerial_4.write(0);
            Serial.println("Send 0 to XIAO 4");
          }
          // 只亮本灯条对应的灯珠数量
          LightUpLeds(leds, need_extra_leds);
          // delay(1000);
          // RunningLights(need_extra_leds);
        }
        tumbler_executed = true; // 标记不倒翁效果已经执行过
      }
      // 如果不倒翁效果已完成,切换回PIR工作流
      if (tumbler_done) {
        taskflow = 1;
        tumbler_executed = false; // 重置不倒翁效果执行标记
      }
      break;
    default:
      taskflow = 1; // 如果没有任何任务,切换到PIR工作流
      Serial.println("Nothing Happened, switch to PIR workflow");  // Far from PIR sensor
  }
}