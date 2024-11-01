#include <esp_now.h>
#include <WiFi.h>
#include <cstring>

// =========================
// 配置部分
// 发送阶段：
// 0号设备 → 1号设备 → 2号设备 → 3号设备 → 4号设备 → 5号设备
// 0号设备 → 6号设备 → 7号设备 → 8号设备 → 9号设备 → 10号设备

// 回传阶段：
// 5号设备 → 4号设备 → 3号设备 → 2号设备 → 1号设备 → 0号设备
// 10号设备 → 9号设备 → 8号设备 → 7号设备 → 6号设备 → 0号设备
// =========================

// 设备编号（0 到 10）
#define DEVICE_ID 5  // 修改为对应设备的编号（0到10）

// 定义MAC地址数组
uint8_t device_macs[11][6] = {
  {0x8C, 0xBF, 0xEA, 0xCB, 0xC5, 0x3C}, // 0
  {0x54, 0x32, 0x04, 0x33, 0x2B, 0x28}, // 1
  {0x8C, 0xBF, 0xEA, 0xCB, 0x80, 0x60}, // 2
  {0x8C, 0xBF, 0xEA, 0xCC, 0xBD, 0x54}, // 3
  {0x8C, 0xBF, 0xEA, 0xCB, 0x80, 0xC8}, // 4
  {0x54, 0x32, 0x04, 0x33, 0x30, 0x94}, // 5
  {0x8C, 0xBF, 0xEA, 0xCB, 0x81, 0x70}, // 6
  {0x8C, 0xBF, 0xEA, 0xCC, 0xBD, 0xE4}, // 7
  {0x54, 0x32, 0x04, 0x33, 0x1F, 0xF8}, // 8
  {0x8C, 0xBF, 0xEA, 0xCC, 0xC1, 0x44}, // 9
  {0x54, 0x32, 0x04, 0x03, 0x36, 0x5C}  // 10
};

// 定义每个设备的下一个设备数量和列表
const int num_next_devices[11] = {
    2, // 0
    1, // 1
    1, // 2
    1, // 3
    1, // 4
    0, // 5
    1, // 6
    1, // 7
    1, // 8
    1, // 9
    0  // 10
};

const int next_devices[11][2] = {
    {1, 6},     // 0
    {2, -1},    // 1
    {3, -1},    // 2
    {4, -1},    // 3
    {5, -1},    // 4
    {-1, -1},   // 5
    {7, -1},    // 6
    {8, -1},    // 7
    {9, -1},    // 8
    {10, -1},   // 9
    {-1, -1}    // 10
};

// 定义每个设备的上一个设备数量和列表
const int num_prev_devices[11] = {
    0, // 0
    1, // 1
    1, // 2
    1, // 3
    1, //4
    1, //5
    1, //6
    1, //7
    1, //8
    1, //9
    1  //10
};

const int prev_devices[11][1] = {
    {-1}, // 0
    {0},  //1
    {1},  //2
    {2},  //3
    {3},  //4
    {4},  //5
    {0},  //6
    {6},  //7
    {7},  //8
    {8},  //9
    {9}   //10
};


// 定义回复计数器（仅0号设备需要）
#if DEVICE_ID == 0
volatile int reply_count = 0;
#endif

bool ready_to_send = (DEVICE_ID == 0) ? true : false; // 0号设备准备发送

// =========================
// 消息结构定义
// =========================
typedef struct struct_message {
  uint8_t type;         // 0x01: 发送, 0x02: 回传, 0x03: PIR
  uint8_t num_xiao;     // 需要的XIAO数量（仅type=0x01时有效）
  uint16_t num_leds;    // 灯珠数量（仅type=0x01时有效）
} struct_message;

// =========================
// ESP-NOW消息发送函数
// =========================
void sendMessage(uint8_t *peer_addr, struct_message msg) {
  esp_err_t result = esp_now_send(peer_addr, (uint8_t *) &msg, sizeof(msg));
  if (result == ESP_OK) {
    Serial.println("消息发送成功");
  }
  else {
    Serial.println("消息发送失败");
  }
}

// =========================
// 接收回调函数
// =========================
void onDataReceive(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  if (len != sizeof(struct_message)) {
    Serial.println("接收到的消息长度不正确");
    return;
  }

  struct_message msg;
  memcpy(&msg, incomingData, sizeof(msg));

  // 获取发送者ID
  int sender_id = -1;
  for(int i = 0; i < 11; i++) {
    if(memcmp(recv_info->src_addr, device_macs[i], 6) == 0){
      sender_id = i;
      break;
    }
  }

  if(sender_id == -1){
    Serial.println("未知发送者");
    return;
  }

  Serial.printf("接收到来自设备%d的消息: Type=0x%02X, NumESP=%d, NumLEDs=%d\n", sender_id, msg.type, msg.num_xiao, msg.num_leds);

  // 处理消息
  if(msg.type == 0x01){ // 发送消息
    // 如果有下一个设备，转发消息
    for(int i = 0; i < num_next_devices[DEVICE_ID]; i++){
      int child_id = next_devices[DEVICE_ID][i];
      if(child_id != -1){
        sendMessage(device_macs[child_id], msg);
        Serial.printf("设备%d转发消息给设备%d\n", DEVICE_ID, child_id);
      }
    }
    // 如果没有下一个设备（即最终设备），开始回传
    if(num_next_devices[DEVICE_ID] == 0){
        struct_message reply;
        reply.type = 0x02; // 回传消息
        reply.num_xiao = 0;
        reply.num_leds = 0;
        // 发送回上一个设备
        sendMessage(device_macs[prev_devices[DEVICE_ID][0]], reply);
        Serial.printf("设备%d发送回传消息给设备%d\n", DEVICE_ID, prev_devices[DEVICE_ID][0]);
    }
  }

  else if(msg.type == 0x02){ // 回传消息
    // 如果有上一个设备，回传消息
    if(prev_devices[DEVICE_ID][0] != -1){
      sendMessage(device_macs[prev_devices[DEVICE_ID][0]], msg);
      Serial.printf("设备%d回传消息给设备%d\n", DEVICE_ID, prev_devices[DEVICE_ID][0]);
    }
    else{
      // 如果是0号设备，标记收到回传消息
      #if DEVICE_ID == 0
      reply_count++;
      Serial.printf("0号设备收到第%d个回传回复\n", reply_count);
      // if(reply_count >= 2){ // 0号设备同时接收来自1和6的回传
      if(reply_count >= 1){
        ready_to_send = true;
        reply_count = 0;
        Serial.println("0号设备已收到所有回传回复，准备发送下一次消息");
      }
      #endif
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // 初始化WiFi为STA模式
  WiFi.mode(WIFI_STA);
  Serial.println("初始化WiFi为STA模式");

  // 初始化ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("初始化ESP-NOW失败");
    while(true);
  }
  Serial.println("ESP-NOW初始化成功");

  // 注册接收回调函数
  esp_now_register_recv_cb(onDataReceive);

  // 添加对等设备（仅下一设备和上一个设备用于回传）
  // 添加所有下一个设备
  for(int i = 0; i < num_next_devices[DEVICE_ID]; i++){
    int child_id = next_devices[DEVICE_ID][i];
    if(child_id != -1){
      esp_now_peer_info_t peerInfo;
      memcpy(peerInfo.peer_addr, device_macs[child_id], 6);
      peerInfo.channel = 0;  
      peerInfo.encrypt = false;
      // 添加对等设备
      if (esp_now_add_peer(&peerInfo) != ESP_OK){
        Serial.printf("添加对等设备%d失败\n", child_id);
      }
      else{
        Serial.printf("添加对等设备%d成功\n", child_id);
      }
    }
  }

  // 添加上一个设备用于回传（除0号设备外）
  for(int i=0; i<num_prev_devices[DEVICE_ID]; i++){
    int parent_id = prev_devices[DEVICE_ID][i];
    if(parent_id != -1){
      esp_now_peer_info_t peerInfo;
      memcpy(peerInfo.peer_addr, device_macs[parent_id], 6);
      peerInfo.channel = 0;  
      peerInfo.encrypt = false;

      // 添加对等设备
      // 注意：如果已经添加过对等设备，添加会失败，但这不会影响操作
      if (esp_now_add_peer(&peerInfo) != ESP_OK){
        // 可以选择忽略或打印提示
        Serial.printf("添加上一个设备%d为对等设备失败或已存在\n", parent_id);
      }
      else{
        Serial.printf("添加上一个设备%d为对等设备成功\n", parent_id);
      }
    }
  }

  // 0号设备准备发送
  if(DEVICE_ID == 0){
    ready_to_send = true;
  }

  Serial.print("XIAO "); Serial.print(DEVICE_ID); Serial.print(" "); Serial.println("Ready!");
}

void loop() {
  // 0号设备负责发送消息
  #if DEVICE_ID == 0
  if(ready_to_send){
    struct_message msg;
    msg.type = 0x01; // 发送类型
    msg.num_xiao = 11; // 总ESP32数量
    msg.num_leds = 60; // 灯珠数量（示例）

    Serial.println("0号设备发送消息");
    // 同时发送给1号和6号设备
    for(int i=0; i<num_next_devices[DEVICE_ID]; i++){
      int child_id = next_devices[DEVICE_ID][i];
      if(child_id != -1){
        sendMessage(device_macs[child_id], msg);
      }
    }
    ready_to_send = false;
  }
  #endif

  // 可以添加其他逻辑，如用户输入触发发送等

  delay(100);
}