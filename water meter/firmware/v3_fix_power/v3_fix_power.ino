#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_sleep.h"

// เพิ่ม 2 library นี้เพื่อแก้เรื่อง Brownout (ไฟตกแล้วรีบูต)
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// ====== CONFIG WiFi & Server ======
const char* WIFI_SSID     = "CJnaJA";
const char* WIFI_PASSWORD = "11223344";

// PHP endpoint บน AppServ/XAMPP
const char* SERVER_URL    = "http://172.20.10.8/meter_upload.php";
const char* DEVICE_ID     = "ESP32CAM_01";

// นาทีที่อยากให้ตื่นมาถ่าย (ตอนนี้ตั้งไว้ 1 นาที)
#define SLEEP_INTERVAL_MIN   1ULL
#define SLEEP_TIME_US  (SLEEP_INTERVAL_MIN * 60ULL * 1000000ULL)

// ====== PIN ESP32-CAM (AI Thinker) ======
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define FLASH_LED_PIN      4

// ---------- helper prototypes ----------
void printWakeupReason();
bool initCamera();
bool connectWiFi();
bool uploadImageToServer(camera_fb_t *fb);
void goToSleep(const char* reason);

// =======================================

void setup() {
  // 1. ปิด Brownout Detector ทันทีเพื่อกันรีเซ็ตเมื่อไฟกระชาก
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 

  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println(F("--- ESP32-CAM Water Meter (Stable Version) ---"));

  printWakeupReason();

  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);

  // -----------------------------------------------------------
  // [FIX 1] ต่อ WiFi ก่อน! เพื่อให้ระบบ Network เสถียรก่อนเริ่มกล้อง
  // -----------------------------------------------------------
  if (!connectWiFi()) {
    Serial.println(F("[setup] WiFi connect failed -> sleep"));
    goToSleep("wifi_fail");
  }
  
  // [FIX 2] ปิดโหมดประหยัดพลังงานของ WiFi (ช่วยลดสัญญาณกวนและไฟกระชาก)
  WiFi.setSleep(false);
  delay(100); 

  // -----------------------------------------------------------
  // [FIX 3] Init กล้องทีหลัง
  // -----------------------------------------------------------
  if (!initCamera()) {
    Serial.println(F("[setup] Camera init failed -> sleep"));
    goToSleep("camera_fail");
  }

  // --- เริ่มขั้นตอนถ่ายรูป ---
  Serial.println(F("[setup] Capturing image..."));
  
  digitalWrite(FLASH_LED_PIN, HIGH);
  delay(500); // รอแสงเข้า + โฟกัส
  
  // เนื่องจากเราเปลี่ยนโหมดเป็น GRAB_LATEST บางทีเฟรมแรกอาจจะดำหรือเสีย
  // แนะนำให้ถ่ายทิ้ง 1 ครั้งถ้าจำเป็น แต่ลองถ่ายจริงเลยก่อนก็ได้
  camera_fb_t *fb = esp_camera_fb_get();
  
  digitalWrite(FLASH_LED_PIN, LOW); // ปิดแฟลชทันที

  if (!fb) {
    Serial.println(F("[setup] Camera capture failed -> sleep"));
    goToSleep("capture_fail");
  }

  Serial.printf("[setup] Captured %d bytes, %dx%d\n", fb->len, fb->width, fb->height);

  // --- อัปโหลด ---
  bool ok = uploadImageToServer(fb);

  // คืนหน่วยความจำ
  esp_camera_fb_return(fb);

  // --- ปิดระบบเพื่อประหยัดไฟ ---
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  esp_camera_deinit();

  if (ok) {
    Serial.println(F("[setup] Upload OK -> sleep"));
    goToSleep("ok");
  } else {
    Serial.println(F("[setup] Upload failed -> sleep anyway"));
    goToSleep("upload_fail");
  }
}

void loop() {
  // ไม่ได้ใช้เพราะ Deep Sleep ใน setup
}

// =======================================
//             Helper functions
// =======================================

void printWakeupReason() {
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  Serial.print(F("[wakeup] cause = "));
  switch (cause) {
    case ESP_SLEEP_WAKEUP_TIMER:      Serial.println(F("TIMER")); break;
    case ESP_SLEEP_WAKEUP_UNDEFINED:  Serial.println(F("UNDEFINED (power on)")); break;
    default:                          Serial.println((int)cause); break;
  }
}

bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;

  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;

  // [FIX 4] ลดความเร็ว XCLK เหลือ 10MHz (ลดปัญหาภาพลาย/DMA Overflow)
  config.xclk_freq_hz = 10000000; 
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size   = FRAMESIZE_VGA; 
    // [FIX 5] ลดคุณภาพ JPEG เป็น 20-25 (เลขเยอะ=ไฟล์เล็ก) เพื่อลดภาระการส่งข้อมูล
    config.jpeg_quality = 20; 
    config.fb_count     = 2;
    config.fb_location  = CAMERA_FB_IN_PSRAM;
  } else {
    config.frame_size   = FRAMESIZE_VGA;
    config.jpeg_quality = 20;
    config.fb_count     = 1;
    config.fb_location  = CAMERA_FB_IN_DRAM;
  }

  // [FIX 6] เปลี่ยนโหมดจับภาพเป็น LATEST (แก้ปัญหา Buffer เต็ม/DMA Overflow)
  config.grab_mode = CAMERA_GRAB_LATEST;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[camera] Init failed, error=0x%x\n", err);
    return false;
  }
  Serial.println(F("Camera init success"));
  return true;
}

bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print(F("Connecting to WiFi"));
  int attempts = 0;
  // รอสูงสุดประมาณ 20-30 วินาที
  while (WiFi.status() != WL_CONNECTED && attempts < 60) { 
    delay(500);
    Serial.print(F("."));
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(F("WiFi connected, IP: "));
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println(F("WiFi connection failed"));
    return false;
  }
}

bool uploadImageToServer(camera_fb_t *fb) {
  if (!fb || !fb->buf || fb->len == 0) {
    Serial.println(F("[upload] invalid frame buffer"));
    return false;
  }

  HTTPClient http;

  String url = String(SERVER_URL) + "?device_id=" + DEVICE_ID;
  Serial.print(F("[upload] URL: "));
  Serial.println(url);

  if (!http.begin(url)) {
    Serial.println(F("[upload] http.begin() failed"));
    return false;
  }

  http.addHeader("Content-Type", "image/jpeg");
  http.setTimeout(20000);  // Timeout 20 วินาที

  Serial.printf("[upload] POST %d bytes...\n", fb->len);
  int httpResponseCode = http.POST(fb->buf, fb->len);
  Serial.printf("[upload] HTTP code: %d\n", httpResponseCode);

  if (httpResponseCode > 0) {
    String body = http.getString();
    Serial.print(F("[upload] Response: "));
    Serial.println(body);
  }

  http.end();
  return (httpResponseCode == 200);
}

void goToSleep(const char* reason) {
  Serial.print(F("[sleep] Reason: "));
  Serial.println(reason);
  Serial.printf("[sleep] Sleep for %llu minute(s)\n",
                (unsigned long long)SLEEP_INTERVAL_MIN);

  esp_sleep_enable_timer_wakeup(SLEEP_TIME_US);
  Serial.flush();
  
  // ปิดไฟ LED ก่อนหลับให้ชัวร์
  digitalWrite(FLASH_LED_PIN, LOW);
  
  delay(100);
  esp_deep_sleep_start();
}