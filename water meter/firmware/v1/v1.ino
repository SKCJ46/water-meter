#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_sleep.h"

// ====== CONFIG WiFi & Server ======
const char* WIFI_SSID     = "Devonix_2.4G";
const char* WIFI_PASSWORD = "Devonix9978168";

// URL ของ PHP endpoint ในเซิร์ฟเวอร์เราเอง (ไม่ใช่ HTTPS จะง่ายสุด)
// สมมติพี่รัน Apache ที่ 192.168.1.10 และไฟล์อยู่ที่ /var/www/html/meter_upload.php
const char* SERVER_URL    = "http://192.168.1.81/meter_upload.php";

const char* DEVICE_ID     = "ESP32CAM_01";  // เอาไว้แยกหลายตัวในอนาคต

// ถ่ายทุก ๆ 60 วินาที (ใช้ deep sleep)
#define SLEEP_TIME_US (60ULL * 1000000ULL)

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

void initCamera() {
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

  if (psramFound()) {
    config.frame_size   = FRAMESIZE_VGA;  // 640x480
    config.jpeg_quality = 12;
    config.fb_count     = 2;
  } else {
    config.frame_size   = FRAMESIZE_VGA;
    config.jpeg_quality = 15;
    config.fb_count     = 1;
  }

  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode   = CAMERA_GRAB_WHEN_EMPTY;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
  } else {
    Serial.println("Camera init success");
  }
}

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

// ส่งภาพแบบ raw JPEG ใน body (ไม่ใช้ multipart ให้ PHP อ่านจาก php://input)
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

  // เพิ่ม device_id เป็น query string
  String url = String(SERVER_URL) + "?device_id=" + DEVICE_ID;
  Serial.print("Request URL: ");
  Serial.println(url);

  if (!http.begin(url)) {
    Serial.println("HTTP begin() failed");
    return;
  }

  // เราจะส่ง binary ตรง ๆ ใช้ content-type แบบ generic
  http.addHeader("Content-Type", "application/octet-stream");
  http.setTimeout(10000); // 10 วินาที

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

void goToSleep() {
  Serial.println("Going to deep sleep for 60 seconds...");
  esp_sleep_enable_timer_wakeup(SLEEP_TIME_US);
  delay(100);
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n--- ESP32-CAM Water Meter Capture Every 1 Minute ---");

  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);

  initCamera();

  if (!connectWiFi()) {
    goToSleep();
  }

  // เปิดแฟลชถ้าจำเป็น (แล้วแต่ที่หน้างาน)
  digitalWrite(FLASH_LED_PIN, HIGH);
  delay(500);

  camera_fb_t *fb = esp_camera_fb_get();
  digitalWrite(FLASH_LED_PIN, LOW);

  if (!fb) {
    Serial.println("Camera capture failed");
  } else {
    Serial.printf("Captured image: %d bytes\n", fb->len);
    uploadImageToServer(fb);
    esp_camera_fb_return(fb);
  }

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  goToSleep();
}

void loop() {
  // ไม่ใช้ เพราะเราทำงานทั้งหมดใน setup() แล้วก็ deep sleep
}
