#include "esp_camera.h"
#include <WiFi.h>

#define CAMERA_MODEL_AI_THINKER // Has PSRAM

#include <Wire.h>
#include <MPU6050_tockn.h>
#include "camera_pins.h"

// Wi-Fi credentials
const char* ssid = "FMCL"; // "HYJs_iPhone"; // "piAP"; //
const char* password = "66955144"; // "12344321"; // "12344321"; //

// Global variable to set the camera resolution.
// Initially set to QVGA (320×240). To change the resolution later, change this variable
// (and reinitialize the camera if needed).
framesize_t cameraResolution = FRAMESIZE_HVGA;

// Create the MPU6050 object using the default Wire instance.
MPU6050 mpu(Wire);

// Function prototypes for our tasks.
void cameraStreamTask(void *pvParameters);
void imuStreamTask(void *pvParameters);

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  delay(1000); // Give time for the Serial Monitor to start.

  // --- Camera Initialization ---
  camera_config_t config;
  config.ledc_channel    = LEDC_CHANNEL_0;
  config.ledc_timer      = LEDC_TIMER_0;
  config.pin_d0          = Y2_GPIO_NUM;
  config.pin_d1          = Y3_GPIO_NUM;
  config.pin_d2          = Y4_GPIO_NUM;
  config.pin_d3          = Y5_GPIO_NUM;
  config.pin_d4          = Y6_GPIO_NUM;
  config.pin_d5          = Y7_GPIO_NUM;
  config.pin_d6          = Y8_GPIO_NUM;
  config.pin_d7          = Y9_GPIO_NUM;
  config.pin_xclk        = XCLK_GPIO_NUM;
  config.pin_pclk        = PCLK_GPIO_NUM;
  config.pin_vsync       = VSYNC_GPIO_NUM;
  config.pin_href        = HREF_GPIO_NUM;
  config.pin_sccb_sda    = SIOD_GPIO_NUM;
  config.pin_sccb_scl    = SIOC_GPIO_NUM;
  config.pin_pwdn        = PWDN_GPIO_NUM;
  config.pin_reset       = RESET_GPIO_NUM;
  config.xclk_freq_hz    = 20000000;
  config.pixel_format    = PIXFORMAT_JPEG;
  config.frame_size      = cameraResolution; // QVGA for now (modifiable later)
  config.jpeg_quality    = 12;
  config.fb_count        = 2;
  config.grab_mode       = CAMERA_GRAB_LATEST;

  // Check for PSRAM
  if (!psramFound()){
    Serial.println("PSRAM not found");
    return;
  }

  // Initialize the camera.
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }
  Serial.println("Camera initialized");

  // --- Wi-Fi Connection ---
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi connected. IP: ");
  Serial.println(WiFi.localIP());

  // --- MPU6050 Initialization ---
  // The MPU6050 is soldered to pins 14 (SCL) and 15 (SDA).
  // Using Wire.begin with a frequency of 100kHz.
  Wire.begin(15, 14, 100000);
  mpu.begin();
  Serial.println("MPU6050 initialized");

  // --- Start the Server Tasks ---
  // Create a task to run the camera stream on port 81.
  xTaskCreatePinnedToCore(cameraStreamTask, "CameraStreamTask", 8192, NULL, 1, NULL, 1);
  // Create a task to run the IMU stream on port 8888.
  xTaskCreatePinnedToCore(imuStreamTask, "IMUStreamTask", 4096, NULL, 1, NULL, 0);
}

void loop() {
  // Nothing to do in loop since tasks are handling everything.
  delay(1000);
}

///////////////////////////////////////////////////////////////////////////////
// CAMERA STREAM TASK
// This task creates a blocking WiFiServer on port 81 that waits for a client.
// When connected, it continuously captures frames from the camera, adds a multipart
// header, and sends the JPEG data, targeting around 30 frames per second.
///////////////////////////////////////////////////////////////////////////////
void cameraStreamTask(void *pvParameters) {
  WiFiServer server(8000);
  server.begin();
  Serial.println("Camera stream server started on port 8000");

  while (true) {
    WiFiClient client = server.available();
    if (client) {
      Serial.println("Camera client connected");
      client.setNoDelay(true);
      // Send initial HTTP header for MJPEG streaming.
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
      client.println("Access-Control-Allow-Origin: *");
      client.println();

      while (client.connected()) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
          Serial.println("Camera capture failed");
          break;
        }
        // Prepare the multipart boundary and header.
        String header = "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: " + String(fb->len) + "\r\n\r\n";
        client.print(header);
        // Send the image data.
        client.write(fb->buf, fb->len);
        client.print("\r\n");
        esp_camera_fb_return(fb);
        // Delay to yield ~30 FPS (approximately 33 ms delay)
        delay(33);
      }
      client.stop();
      Serial.println("Camera client disconnected");
    }
    delay(1);
  }
}

///////////////////////////////////////////////////////////////////////////////
// IMU STREAM TASK
// This task creates a blocking WiFiServer on port 8888 that waits for a client.
// When a client connects, it sends JSON strings containing timestamped MPU6050 data
// at roughly 300 Hz.
///////////////////////////////////////////////////////////////////////////////
void imuStreamTask(void *pvParameters) {
  WiFiServer server(8888);
  server.begin();
  Serial.println("IMU stream server started on port 8888");

  while (true) {
    WiFiClient client = server.available();
    if (client) {
      Serial.println("IMU client connected");
      client.setNoDelay(true);
      // Send HTTP header for a plain text / JSON stream.
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Access-Control-Allow-Origin: *");
      client.println();

      while (client.connected()) {
        // Update and read MPU6050 data.
        mpu.update();
        float ax = mpu.getAccX();
        float ay = mpu.getAccY();
        float az = mpu.getAccZ();
        float gx = mpu.getGyroX();
        float gy = mpu.getGyroY();
        float gz = mpu.getGyroZ();
        // Get a timestamp in microseconds.
        uint64_t timestamp = esp_timer_get_time();

        // Format the data into a JSON string.
        char jsonBuffer[128];
        snprintf(jsonBuffer, sizeof(jsonBuffer),
                 "{\"t_us\":%llu,\"ax\":%.2f,\"ay\":%.2f,\"az\":%.2f,\"gx\":%.2f,\"gy\":%.2f,\"gz\":%.2f}\n",
                 timestamp, ax, ay, az, gx, gy, gz);
        client.print(jsonBuffer);

        // Delay to yield ~300 Hz (approximately 3 ms delay)
        delay(3);
      }
      client.stop();
      Serial.println("IMU client disconnected");
    }
    delay(1);
  }
}
