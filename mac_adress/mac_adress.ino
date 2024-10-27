// IMU：8c:bf:ea:cb:c5:3c
// 1：54:32:04:33:2b:28
// 2：8c:bf:ea:cb:80:60
// 3：8c:bf:ea:cc:bd:54
// 4：8c:bf:ea:cb:80:c8
// 5：54:32:04:33:30:94
// 6：8c:bf:ea:cb:81:70
// 7：8c:bf:ea:cc:bd:e4
// 8：54:32:04:33:1f:f8
// 9：8c:bf:ea:cc:c1:44

#include <WiFi.h>
#include <esp_wifi.h>

void readMacAddress(){
  uint8_t baseMac[6];
  esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
  if (ret == ESP_OK) {
    Serial.printf("%02x:%02x:%02x:%02x:%02x:%02x\n",
                  baseMac[0], baseMac[1], baseMac[2],
                  baseMac[3], baseMac[4], baseMac[5]);
  } else {
    Serial.println("Failed to read MAC address");
  }
}

void setup(){
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.STA.begin();

  Serial.print("[DEFAULT] ESP32 Board MAC Address: ");
  readMacAddress();
}
 
void loop(){

}

