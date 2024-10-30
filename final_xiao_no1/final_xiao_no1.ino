// 导入必要的库
#include <FastLED.h>
#include "ESP32_NOW_Serial.h"
#include "MacAddress.h"
#include "WiFi.h"
#include "esp_wifi.h"


/********************************* 灯相关 *********************************/
#define NUM_LEDS 59
#define LED_PIN_1 D1
#define LED_PIN_2 D5
#define BRIGHTNESS 255                    // FastLED brightness, 0 (min) to 255 (max)
CRGB leds_1[NUM_LEDS];
CRGB leds_2[NUM_LEDS];


/********************************* PIR相关 *********************************/
#define digital_pir_sensor_1 D0
#define digital_pir_sensor_2 D2
const unsigned long motionTimeout = 5000; // 5 seconds

const int motionCountThreshold = 2;       // PIR触发持续有人的阈值
bool meteorDirection = true;              // 控制流水灯的流转方向


/********************************* 不倒翁控制相关 *********************************/
int taskflow = 0;                         // 控制当前任务流
uint8_t need_num_xiao, need_extra_leds;   // 0号位上报的灯相关信息


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
const MacAddress peer_mac_0({0x8C, 0xBF, 0xEA, 0xCB, 0xC5, 0x3C});
const MacAddress peer_mac_2({0x8C, 0xBF, 0xEA, 0xCB, 0x80, 0x60});
const MacAddress peer_mac_6({0x8C, 0xBF, 0xEA, 0xCB, 0x81, 0x70});

ESP_NOW_Serial_Class NowSerial_0(peer_mac_0, ESPNOW_WIFI_CHANNEL, ESPNOW_WIFI_IF);
ESP_NOW_Serial_Class NowSerial_2(peer_mac_2, ESPNOW_WIFI_CHANNEL, ESPNOW_WIFI_IF);
ESP_NOW_Serial_Class NowSerial_6(peer_mac_6, ESPNOW_WIFI_CHANNEL, ESPNOW_WIFI_IF);


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
  for(int i = 0; i < numLeds; i++) {
    leds[i] = CRGB(sin(i + millis() / 100.0) * 127 + 128, random(0, 255), random(0, 255));
  }
  showStrip();
  delay(500);
}


/********************************* PIR函数 *********************************/
int motionCount = 0;                    // 用于一直有人时持续触发的灯光效果
unsigned long lastMotionTime = 0;       // 用于避免同一次PIR反复触发灯光效果

void handlePIR(int pirSensor, CRGB *leds, int numLeds) {
  bool state = digitalRead(pirSensor); // read from PIR sensor
  if (state == 1) {
    Serial.print("A Motion has occured on sensor ");
    Serial.println(pirSensor == digital_pir_sensor_1 ? "1" : "2");  // When there is a response
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
    Serial.print("Nothing Happened on sensor ");
    Serial.println(pirSensor == digital_pir_sensor_1 ? "1" : "2");  // Far from PIR sensor
    motionCount = 0;
    // setAll(leds, numLeds, 0, 0, 0); // Turn off all LEDs when no motion is detected
  }
}

/********************************* Arduino *********************************/
void setup()
{
  Serial.begin(115200);  // set baud rate as 9600

  pinMode(digital_pir_sensor_1, INPUT); // set Pin mode as input
  pinMode(digital_pir_sensor_2, INPUT); // set Pin mode as input

  FastLED.addLeds<WS2812B, LED_PIN_1, GRB>(leds_1, NUM_LEDS);
  FastLED.addLeds<WS2812B, LED_PIN_2, GRB>(leds_2, NUM_LEDS);
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
  NowSerial_0.begin(115200);
  NowSerial_2.begin(115200);
  NowSerial_6.begin(115200);
  Serial.println("You can now send data to the peer device using the Serial Monitor.\n");
}


void loop()
{
  // IMU XIAO 的消息到达（优先级最高）
  if (NowSerial_0.available() >= 0) {
    int bytesRead = NowSerial_0.readBytes(&need_num_xiao, 1);
    bytesRead += NowSerial_0.readBytes(&need_extra_leds, 1);
    if (bytesRead == 2) {
      Serial.print("Received array from device 0: ");

      // 获得灯条相关的信息
      Serial.print("Need Num of XIAO: "); Serial.println(need_num_xiao);
      Serial.print("Need Num of extra LEDS: "); Serial.println(need_extra_leds);
      taskflow = 2;
    }
  }

  // 不倒翁效果结束的消息传来
  int receivedData_2, receivedData_6 = 0;
  if (NowSerial_2.available()) {
    receivedData_2 = NowSerial_2.read();
    Serial.print("Received data from XIAO 2: ");
    Serial.println(receivedData_2);
  }
  if (NowSerial_6.available()) {
    receivedData_6 = NowSerial_6.read();
    Serial.print("Received data from XIAO 6: ");
    Serial.println(receivedData_6);
  }
  // 重新开启 PIR
  if (receivedData_2 && receivedData_6) {
    taskflow = 1;
  }

  switch(taskflow) {
    case 1:                    // PIR工作流
      handlePIR(digital_pir_sensor_1, leds_1, NUM_LEDS);
      handlePIR(digital_pir_sensor_2, leds_2, NUM_LEDS);
      break;
    case 2:                    // 不倒翁工作流
      need_num_xiao -= 1;
      if (need_num_xiao) {
        // 先发送 ESP-NOW 消息给下一个设备
        if (NowSerial_2.availableForWrite()) {
          int bytesWritten = NowSerial_2.write(need_num_xiao);
          bytesWritten += NowSerial_2.write(need_extra_leds);
          if (bytesWritten == 2) {
            Serial.println("Send data to XIAO 2 Successfully");
          } else {
            Serial.println("Failed to send data to XIAO 2");
          }
        }
      
        if (NowSerial_6.availableForWrite()) {
          int bytesWritten = NowSerial_6.write(need_num_xiao);
          bytesWritten += NowSerial_6.write(need_extra_leds);
          if (bytesWritten == 2) {
            Serial.println("Send data to XIAO 6 Successfully");
          } else {
            Serial.println("Failed to send data to XIAO 6");
          }
        }

        // 再同时点亮两个灯条
        LightUpLeds(leds_1, NUM_LEDS);
        LightUpLeds(leds_2, NUM_LEDS);
      }

      // 再同时点亮两个灯条
      LightUpLeds(leds_1, need_extra_leds);
      LightUpLeds(leds_2, need_extra_leds);
      break;
    defalut:
      Serial.println("Nothing Happened");  // Far from PIR sensor
  }

  taskflow = 0;
}