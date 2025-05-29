#include "esp_camera.h"
#include <WiFi.h>
#include <Wire.h>
#include <MPU6050_tockn.h>
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
#include "camera_pins.h"


#define WiFi_Quanser
// #define WiFi_FMCL
// #define WiFi_HYJ
// #define WiFi_piAP

// Wi-Fi credentials
#if defined(WiFi_Quanser)
  const char* ssid     = "Quanser_UVS";
  const char* password = "UVS_wifi";
#endif

#if defined(WiFi_FMCL)
  const char* ssid     = "FMCL";
  const char* password = "66955144";
#endif

#if defined(WiFi_HYJ)
  const char* ssid     = "HYJs_iPhone";
  const char* password = "12344321";
#endif

#if defined(WiFi_piAP)
  const char* ssid     = "piAP";
  const char* password = "12344321";
#endif

static const framesize_t CAM_RES = FRAMESIZE_VGA;   // camera resolution
static const int        CAM_PORT = 8000;
static const int        IMU_PORT = 8888;

// ──────────────────────────────────────────────────────────
// GLOBALS & QUEUES
// ──────────────────────────────────────────────────────────
MPU6050 mpu(Wire);

struct ImuPacket {
  uint64_t t_us;
  float ax, ay, az;
  float gx, gy, gz;
};

QueueHandle_t camQ;   // holds camera_fb_t*
QueueHandle_t imuQ;   // holds ImuPacket

// ──────────────────────────────────────────────────────────
// TASK PROTOTYPES
// ──────────────────────────────────────────────────────────
void camProducerTask(void*);
void camNetTask(void*);
void imuProducerTask(void*);
void imuNetTask(void*);

// ──────────────────────────────────────────────────────────
// SET‑UP
// ──────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  delay(400);

  // ── Camera init ────────────────────────────────────────
  camera_config_t cfg = {};
  cfg.ledc_channel = LEDC_CHANNEL_0;
  cfg.ledc_timer   = LEDC_TIMER_0;
  cfg.pin_d0 = Y2_GPIO_NUM;  cfg.pin_d1 = Y3_GPIO_NUM;
  cfg.pin_d2 = Y4_GPIO_NUM;  cfg.pin_d3 = Y5_GPIO_NUM;
  cfg.pin_d4 = Y6_GPIO_NUM;  cfg.pin_d5 = Y7_GPIO_NUM;
  cfg.pin_d6 = Y8_GPIO_NUM;  cfg.pin_d7 = Y9_GPIO_NUM;
  cfg.pin_xclk = XCLK_GPIO_NUM;
  cfg.pin_pclk = PCLK_GPIO_NUM;
  cfg.pin_vsync = VSYNC_GPIO_NUM;
  cfg.pin_href  = HREF_GPIO_NUM;
  cfg.pin_sccb_sda = SIOD_GPIO_NUM;
  cfg.pin_sccb_scl = SIOC_GPIO_NUM;
  cfg.pin_pwdn  = PWDN_GPIO_NUM;
  cfg.pin_reset = RESET_GPIO_NUM;
  cfg.xclk_freq_hz = 20000000;
  cfg.pixel_format = PIXFORMAT_JPEG;
  cfg.frame_size   = CAM_RES;
  cfg.jpeg_quality = 20;                  //! image quality lower the better -> !!validate!!
  cfg.fb_count     = 2;
  cfg.grab_mode    = CAMERA_GRAB_LATEST;  //! what does this do?

  if (!psramFound()) { Serial.println("No PSRAM!"); while (1) {} }
  ESP_ERROR_CHECK( esp_camera_init(&cfg) );
  Serial.println("Camera ready");

  // flip frames vertically
  sensor_t *sensor = esp_camera_sensor_get();
  sensor->set_vflip(sensor, 1);
  sensor->set_hmirror(sensor, 1);

  // ── Wi‑Fi ───────────────────────────────────────────────
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  Serial.print("Wi‑Fi ");
  while (WiFi.status() != WL_CONNECTED) { Serial.print('.'); delay(300); }
  Serial.printf("\nIP: %s\n", WiFi.localIP().toString().c_str());

  // ── MPU‑6050 @400 kHz I²C ──────────────────────────────
  Wire.begin(15, 14, 400000);        // SDA=15, SCL=14, 400 kHz
  Wire.setClock(400000);
  mpu.begin();
  Serial.println("MPU6050 ready");

  // ── Queues (pointers only → tiny) ──────────────────────
  camQ = xQueueCreate(3,  sizeof(camera_fb_t*));
  imuQ = xQueueCreate(32, sizeof(ImuPacket));

  // ── Tasks (all on core 1) ──────────────────────────────
  xTaskCreatePinnedToCore(camProducerTask, "camProd", 4096, nullptr, 3, nullptr, 1);
  xTaskCreatePinnedToCore(camNetTask,      "camNet",  8192, nullptr, 2, nullptr, 1);
  xTaskCreatePinnedToCore(imuProducerTask, "imuProd", 4096, nullptr, 2, nullptr, 1);
  xTaskCreatePinnedToCore(imuNetTask,      "imuNet",  4096, nullptr, 1, nullptr, 1);
}

void loop() { vTaskDelay(portMAX_DELAY); }   // nothing here

// ──────────────────────────────────────────────────────────
// CAMERA PRODUCER  (grab frames → queue)
// ──────────────────────────────────────────────────────────
void camProducerTask(void*) {
  for (;;) {
    camera_fb_t* fb = esp_camera_fb_get();           // blocking until frame ready
    if (!fb) continue;
    if (xQueueSend(camQ, &fb, 0) != pdTRUE) {        // queue full → drop oldest
      esp_camera_fb_return(fb);
    }
  }
}

// ──────────────────────────────────────────────────────────
// CAMERA NETWORK  (HTTP MJPEG, pops queue)
// ──────────────────────────────────────────────────────────
void camNetTask(void*) {
  WiFiServer server(CAM_PORT);
  server.begin();
  Serial.printf("MJPEG on: %d\n", CAM_PORT);

  for (;;) {
    WiFiClient client = server.available();
    if (!client) { vTaskDelay(pdMS_TO_TICKS(20)); continue; }

    // ─ HTTP header ─
    client.setNoDelay(true);
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
    client.println("Access-Control-Allow-Origin: *\r\n");

    Serial.println("MJPEG client connected");
    while (client.connected()) {
      camera_fb_t* fb = nullptr;
      if (xQueueReceive(camQ, &fb, pdMS_TO_TICKS(200)) == pdTRUE) {
        String head = "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: "
                      + String(fb->len) + "\r\n\r\n";
        client.print(head);
        client.write(fb->buf, fb->len);
        client.print("\r\n");
        esp_camera_fb_return(fb);
      }
    }
    client.stop();
    Serial.println("MJPEG client disconnected");
  }
}

// ──────────────────────────────────────────────────────────
// IMU PRODUCER  (300 Hz precise)
// ──────────────────────────────────────────────────────────
void imuProducerTask(void*) {
  const TickType_t period = pdMS_TO_TICKS(3);      // ≈300 Hz
  TickType_t last = xTaskGetTickCount();

  for (;;) {
    ImuPacket pkt;
    mpu.update();
    pkt.t_us = esp_timer_get_time();
    pkt.ax = mpu.getAccX(); pkt.ay = mpu.getAccY(); pkt.az = mpu.getAccZ();
    pkt.gx = mpu.getGyroX(); pkt.gy = mpu.getGyroY(); pkt.gz = mpu.getGyroZ();

    xQueueSend(imuQ, &pkt, 0);     // drop if queue full
    vTaskDelayUntil(&last, period);
  }
}

// ──────────────────────────────────────────────────────────
// IMU NETWORK  (JSON line stream)
// ──────────────────────────────────────────────────────────
void imuNetTask(void*) {
  WiFiServer server(IMU_PORT);
  server.begin();
  Serial.printf("IMU JSON: %d\n", IMU_PORT);

  for (;;) {
    WiFiClient client = server.available();
    if (!client) { vTaskDelay(pdMS_TO_TICKS(50)); continue; }

    client.setNoDelay(true);
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Access-Control-Allow-Origin: *\r\n");

    Serial.println("IMU client connected");
    while (client.connected()) {
      ImuPacket pkt;
      if (xQueueReceive(imuQ, &pkt, pdMS_TO_TICKS(100)) == pdTRUE) {
        char line[160];
        snprintf(line, sizeof(line),
                 "{\"t_us\":%llu,\"ax\":%.3f,\"ay\":%.3f,\"az\":%.3f,"
                 "\"gx\":%.3f,\"gy\":%.3f,\"gz\":%.3f}\n",
                 pkt.t_us, pkt.ax, pkt.ay, pkt.az, pkt.gx, pkt.gy, pkt.gz);
        client.print(line);
      }
    }
    client.stop();
    Serial.println("IMU client disconnected");
  }
}
