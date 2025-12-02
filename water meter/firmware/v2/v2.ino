#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_sleep.h"

// ===== CONFIG WiFi & Server =====
const char* WIFI_SSID     = "xxxxxxxxxxxxxxx";
const char* WIFI_PASSWORD = "xxxxxxxxxxxxxxx";

// ใช้ IP ของเครื่อง AppServ (ห้ามใช้ localhost)
const char* SERVER_URL    = "http://xxxxxxxxx/meter_upload.php";
const char* DEVICE_ID     = "ESP32CAM_01";

// ตอนทดสอบ: ถ่ายทุก 60 วินาที
#define SLEEP_TIME_US (60ULL * 1000000ULL)

// ใช้งานจริงวันละครั้ง: ปลดคอมเมนต์บรรทัดนี้แทน
// #define SLEEP_TIME_US (24ULL * 60ULL * 60ULL * 1000000ULL)

// ===== PIN ESP32-CAM (AI Thinker) =====
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

#define FLASH_LED_PIN      4   // แฟลชบนบอร์ดส่วนใหญ่ใช้ GPIO4

// ===== ตั้งค่ากล้องแบบเน้นคุณภาพภาพ =====
void initCameraHighQuality() {
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

  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // ใช้ความละเอียดสูงหน่อยเพื่ออ่านเลขง่าย
  config.frame_size   = FRAMESIZE_SVGA;   // 800x600
  config.jpeg_quality = 10;              // 0-63 (ต่ำ = ชัด)
  config.fb_count     = 1;               // เราใช้แค่เฟรมเดียว

  config.fb_location  = CAMERA_FB_IN_PSRAM;
  config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }

  Serial.println("Camera init success");

  // ปรับ sensor ให้ภาพคม/สว่างขึ้นนิดนึง
  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    s->set_framesize(s, FRAMESIZE_SVGA); // ย้ำอีกที เผื่อมีค่า default แปลก ๆ
    s->set_brightness(s, 1);             // -2..2
    s->set_contrast(s, 1);               // -2..2
    s->set_sharpness(s, 1);              // -2..2
    s->set_saturation(s, 0);             // -2..2
    s->set_gainceiling(s, GAINCEILING_16X);
    // ถ้าภาพสว่างเกิน/มืดเกิน ค่อยมา fine-tune ตรงนี้ภายหลัง
  }
}

// ===== เชื่อม WiFi =====
bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected, IP: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("WiFi connection failed");
    return false;
  }
}

// ===== ส่งรูปไปหา PHP (binary) =====
void uploadImageToServer(camera_fb_t *fb) {
  if (!fb) {
    Serial.println("No frame buffer to upload!");
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, cannot upload.");
    return;
  }

  HTTPClient http;
  String url = String(SERVER_URL) + "?device_id=" + DEVICE_ID;
  Serial.print("Request URL: ");
  Serial.println(url);

  if (!http.begin(url)) {
    Serial.println("HTTP begin() failed");
    return;
  }

  http.addHeader("Content-Type", "application/octet-stream");
  http.setTimeout(15000); // 15 วินาที เผื่อเน็ตช้า

  Serial.println("Uploading image...");
  int httpResponseCode = http.POST((uint8_t*)fb->buf, fb->len);

  Serial.printf("HTTP Response code: %d\n", httpResponseCode);
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.print("Response body: ");
    Serial.println(response);
  } else {
    Serial.printf("HTTP Error: %d (%s)\n",
                  httpResponseCode,
                  http.errorToString(httpResponseCode).c_str());
  }

  http.end();
}

// ===== เข้าสู่ Deep Sleep =====
void goToSleep() {
  Serial.println("Going to deep sleep...");
  esp_sleep_enable_timer_wakeup(SLEEP_TIME_US);
  delay(100);
  esp_deep_sleep_start();
}

// ===== setup & loop =====
void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n--- ESP32-CAM Water Meter (High Quality) ---");

  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);

  // 1) Init กล้อง (High Quality)
  initCameraHighQuality();

  // ถ้า init ล้มเหลว ไม่ต้องทำต่อ
  // (ตรวจแบบหยาบด้วยการลองเรียก sensor)
  if (!esp_camera_sensor_get()) {
    Serial.println("Camera not ready, go to sleep");
    goToSleep();
  }

  // 2) ต่อ WiFi
  if (!connectWiFi()) {
    esp_camera_deinit();
    goToSleep();
  }

  // 3) เปิดแฟลช + pre-capture
  digitalWrite(FLASH_LED_PIN, HIGH);
  delay(400); // ให้ auto exposure ทำงาน

  // ดึงเฟรมแรกทิ้ง (pre-capture)
  camera_fb_t *fb = esp_camera_fb_get();
  if (fb) {
    esp_camera_fb_return(fb);
  }

  // 4) ดึงเฟรมจริง
  fb = esp_camera_fb_get();

  // ปิดแฟลช
  digitalWrite(FLASH_LED_PIN, LOW);

  if (!fb) {
    Serial.println("Camera capture failed");
  } else {
    Serial.printf("Captured image: %d bytes, %dx%d\n",
                  fb->len, fb->width, fb->height);
    uploadImageToServer(fb);
    esp_camera_fb_return(fb);
  }

  // 5) ปิด WiFi + กล้อง แล้วหลับ
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  esp_camera_deinit();

  goToSleep();
}

void loop() {
  // ไม่ใช้ เพราะทำงานทั้งหมดใน setup แล้วก็ deep sleep
}
