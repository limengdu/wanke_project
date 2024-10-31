// 导入必要的库
#include "LIS3DHTR.h"
#include <Wire.h>
LIS3DHTR<TwoWire> LIS; //IIC
#define WIRE Wire
#include "ESP32_NOW_Serial.h"
#include "MacAddress.h"
#include "WiFi.h"
#include "esp_wifi.h"


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
const MacAddress peer_mac_1({0x54, 0x32, 0x04, 0x33, 0x2B, 0x28});

ESP_NOW_Serial_Class NowSerial_1(peer_mac_1, ESPNOW_WIFI_CHANNEL, ESPNOW_WIFI_IF);


/********************************* 不倒翁控制相关 *********************************/
const int MAX_LEDS = 59 * 5;       // 总共59*5个灯珠
const int PER_NUM_LEDS = 59;       // 单灯条59个灯珠
const float FORCE_THRESHOLD = 2.1; // 力度阈值,可根据实际情况调整
const float MAX_THRESHOLD = 3.54;  // 最大力度,可根据实际情况调整
unsigned long hitStartTime = 0;    // 记录击打开始的时间
bool isHit = false;                // 表示当前是否处于击打状态
float magnitudeSum = 0;            // 存储击打期间所有幅度值的总和
int magnitudeCount = 0;            // 记录击打期间的幅度值数量
bool isReported = false;           // 表示是否已经上报了结果


/********************************* Arduino *********************************/
void setup()
{
  Serial.begin(115200);

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
  NowSerial_1.begin(115200);
  Serial.println("You can now send data to the peer device using the Serial Monitor.\n");

  LIS.begin(WIRE,0x19); //IIC init
  LIS.openTemp();  //If ADC3 is used, the temperature detection needs to be turned off.
  delay(100);
  LIS.setFullScaleRange(LIS3DHTR_RANGE_2G);
  LIS.setOutputDataRate(LIS3DHTR_DATARATE_50HZ);
}


void loop()
{
  if (!LIS){
    Serial.println("LIS3DHTR didn't connect.");
    return;
  }

  float x = LIS.getAccelerationX();
  float y = LIS.getAccelerationY(); 
  float z = LIS.getAccelerationZ();

  //3 axis
  // Serial.print("x:"); Serial.print(LIS.getAccelerationX()); Serial.print("  ");
  // Serial.print("y:"); Serial.print(LIS.getAccelerationY()); Serial.print("  ");
  // Serial.print("z:"); Serial.println(LIS.getAccelerationZ());

  // 如果没有在击打状态或者已经上报了结果,才进行击打检测
  if (!isHit || !isReported) {
  // 计算三轴加速度的模长
    float magnitude = sqrt(x * x + y * y + z * z);

    // 如果模长超过阈值,认为不倒翁受到了击打
    if (magnitude > FORCE_THRESHOLD) {
      if (!isHit) {
        // 如果这是一次新的击打,记录击打开始时间
        hitStartTime = millis();
        isHit = true;
        isReported = false;
      }
      
      // 在击打开始的两秒内,累加幅度值,并增加计数器
      if (millis() - hitStartTime < 2000) {
        magnitudeSum += magnitude;
        magnitudeCount++;
      }
    }
  }
  
  // 如果击打已经开始,且已经过去了两秒,且还没有上报结果
  if (isHit && millis() - hitStartTime >= 2000 && !isReported) {
    // 计算平均幅度
    float avgMagnitude = magnitudeSum / magnitudeCount;
    
    int numLedsToLight;
    // 如果平均幅度超过最大阈值,点亮所有灯珠
    if (avgMagnitude >= MAX_THRESHOLD) {
      numLedsToLight = MAX_LEDS;
    }
    else {
      // 在 FORCE_THRESHOLD 和 MAX_THRESHOLD 之间,使用线性函数计算点亮的灯珠数量
      float ratio = (avgMagnitude - FORCE_THRESHOLD) / (MAX_THRESHOLD - FORCE_THRESHOLD);
      numLedsToLight = 1 + (MAX_LEDS - 1) * ratio;
    }
    
    Serial.println("Hit the tumbler.");
    Serial.print("Average magnitude: "); Serial.println(avgMagnitude);
    Serial.print("Need Total LEDs Number: "); Serial.println(numLedsToLight);

    // int num_xiao = numLedsToLight / PER_NUM_LEDS;
    // int extraLEDs = numLedsToLight % PER_NUM_LEDS;
    // int num_xiao = 1;
    // int extraLEDs = 30;
    int num_xiao = 5;
    int extraLEDs = 30;
    Serial.print("Need Num of XIAO: "); Serial.println(num_xiao);
    Serial.print("Need Num of extra LEDS: "); Serial.println(extraLEDs);

    int bytesWritten = NowSerial_1.write(num_xiao);
    bytesWritten += NowSerial_1.write(extraLEDs);

    if (bytesWritten == 2) {
      Serial.println("Sent array to XIAO 1 successfully");
    } else {
      Serial.println("Failed to send array to XIAO 1");
    }
    // 确保下一个XIAO收到消息，不然重发
    while(!NowSerial_1.available()) {
      int frame = NowSerial_1.read();
      if (frame == 8) break;
      else {
        bytesWritten = NowSerial_1.write(num_xiao);
        bytesWritten += NowSerial_1.write(extraLEDs);
        delay(500);
      }
    }
    Serial.println("XIAO 1 received!");
    isReported = true;  // 标记已上报
  }
  
  // 如果 NowSerial_1 有可用数据
  if (NowSerial_1.available()) {
    int receivedData = NowSerial_1.read();
    
    // 如果收到0,表示不倒翁效果已完成,重置变量,准备下一次击打
    if (receivedData == 0) {
      Serial.println("Received tumbler done message from XIAO 1");
      isHit = false;
      isReported = false;
      magnitudeSum = 0;
      magnitudeCount = 0;
    }
  }
  delay(50);
}
