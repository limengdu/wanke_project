// 导入必要的库
#include "LIS3DHTR.h"
#include <Wire.h>
LIS3DHTR<TwoWire> LIS; //IIC
#define WIRE Wire

/********************************* Arduino *********************************/
void setup()
{
  Serial.begin(115200);
  while(!Serial);
  delay(500);
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

  // float minX = LIS.getAccelerationX();
  // float maxX = LIS.getAccelerationX();
  // float minY = LIS.getAccelerationY();
  // float maxY = LIS.getAccelerationY();
  // float minZ = LIS.getAccelerationZ();
  // float maxZ = LIS.getAccelerationZ();
  float max_magnitude = sqrt(x * x + y * y + z * z);

  unsigned long startTime = millis();
  while (millis() - startTime < 10000) {
    float x = LIS.getAccelerationX();
    float y = LIS.getAccelerationY(); 
    float z = LIS.getAccelerationZ();

    float magnitude = sqrt(x * x + y * y + z * z);

    max_magnitude = max(max_magnitude, magnitude);

    // minX = min(minX, x);
    // maxX = max(maxX, x);
    // minY = min(minY, y);
    // maxY = max(maxY, y);
    // minZ = min(minZ, z);
    // maxZ = max(maxZ, z);

    delay(50); // 每50ms读取一次数据
  }

  // 打印5秒内的最大最小值
  // Serial.print("x min:"); Serial.print(minX); Serial.print("  ");
  // Serial.print("x max:"); Serial.println(maxX);
  // Serial.print("y min:"); Serial.print(minY); Serial.print("  ");
  // Serial.print("y max:"); Serial.println(maxY);
  // Serial.print("z min:"); Serial.print(minZ); Serial.print("  ");
  // Serial.print("z max:"); Serial.println(maxZ);
  Serial.print("max magnitude: "); Serial.println(max_magnitude);
  delay(2000);
  Serial.println("Next begin!");
}