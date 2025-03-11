typedef union{
  float number;
  uint8_t bytes[4];
} FLOATUNION_t;

FLOATUNION_t myValue;

#include "esp_camera.h"
#include <WiFi.h>

// ============ USER SETTINGS ============
const char* SSID     = "FMCL";
const char* PASSWORD = "66955144";

int counter1 = 0;
float counter2 = 0;

// Web server on port 80WiFiServer server(8888);
WiFiServer server(8888);

// ============ CAMERA CONFIGURATION ============
static camera_config_t camera_config = {
  .pin_pwdn  = 32,     // power down is not used
  .pin_reset = -1,     // software reset will be performed
  .pin_xclk = 0,
  .pin_sscb_sda = 26,
  .pin_sscb_scl = 27,

  .pin_d7 = 35,
  .pin_d6 = 34,
  .pin_d5 = 39,
  .pin_d4 = 36,
  .pin_d3 = 21,
  .pin_d2 = 19,
  .pin_d1 = 18,
  .pin_d0 = 5,
  .pin_vsync = 25,
  .pin_href = 23,
  .pin_pclk = 22,

  // XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
  .xclk_freq_hz = 20000000,
  .ledc_timer = LEDC_TIMER_0,
  .ledc_channel = LEDC_CHANNEL_0,

  .pixel_format = PIXFORMAT_JPEG,
  // Check whether your esp_camera library defines FRAMESIZE_HD = 1280x720
  // Some libraries use FRAMESIZE_P_HD for 720p. Adjust if needed.
  .frame_size = FRAMESIZE_HD,  
  .jpeg_quality = 12,         // Lower = better quality, bigger file
  .fb_count = 2,              // Use 2 frame-buffers
  .grab_mode = CAMERA_GRAB_LATEST
};

// Content definitions for MJPEG streaming
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=frame";
static const char* _STREAM_BOUNDARY = "\r\n--frame\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

// Forward declaration
// void startCameraServer();

void setup() {
  Serial.begin(115200);

  // Initialize the camera
  if (esp_camera_init(&camera_config) != ESP_OK) {
    Serial.println("Camera init failed!");
    while(true) { delay(1000); }
  } else {
    Serial.println("Camera init successful!");
  }

  // Optional: tweak sensor settings if you wish
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA);   // Already in config
  s->set_brightness(s, 1);             // Example: adjust brightness

  // Connect to Wi-Fi
  WiFi.begin(SSID, PASSWORD);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected!");
  server.begin();
  Serial.print("Camera Stream Ready! Go to: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/stream");
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    while(client.connected()){

      // take a camera frame
      camera_fb_t * fb = esp_camera_fb_get();
      if (!fb) {
        Serial.println("Camera capture failed");
        //break;
      }

      // Send a 4-byte header with the frame size (big-endian)
      uint32_t frameSize = fb->len;
      float header[4];
      header[0] = (frameSize >> 24) & 0xFF;
      header[1] = (frameSize >> 16) & 0xFF;
      header[2] = (frameSize >> 8) & 0xFF;
      header[3] = frameSize & 0xFF;
      for(int j=0; j<4; j++){
        myValue.number = header[j];
        for (int i=0; i<4; i++){
          client.write(myValue.bytes[i]); 
        }
      }
      for (size_t i = 0; i < fb->len; i++) {
        client.write(fb->buf[i]);
      }
    }
  }
}