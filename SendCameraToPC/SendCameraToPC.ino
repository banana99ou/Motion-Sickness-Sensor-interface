/****************************************************
  ESP32-CAM MJPEG Streaming @ 1280x720 Example
  --------------------------------------------------
  Streams MJPEG video at http://<ESP32_IP>/stream
  and single JPEG frames at http://<ESP32_IP>/capture

  Author: ChatGPT (Revised for standard ESP32 libraries)
 ****************************************************/

 #include "esp_camera.h"
 #include <WiFi.h>
 #include <WebServer.h>
 
 // ============ USER SETTINGS ============
 const char* SSID     = "FMCL";
 const char* PASSWORD = "66955144";
 
 // Web server on port 80
 WebServer server(80);
 
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
 void startCameraServer();
 
 void setup() {
   Serial.begin(115200);
   Serial.setDebugOutput(false);
   Serial.println();
 
   // Initialize the camera
   if (esp_camera_init(&camera_config) != ESP_OK) {
     Serial.println("Camera init failed!");
     while(true) { delay(1000); }
   } else {
     Serial.println("Camera init successful!");
   }
 
   // Optional: tweak sensor settings if you wish
   // sensor_t * s = esp_camera_sensor_get();
   // s->set_framesize(s, FRAMESIZE_HD);   // Already in config
   // s->set_brightness(s, 1);             // Example: adjust brightness
 
   // Connect to Wi-Fi
   WiFi.begin(SSID, PASSWORD);
   Serial.print("Connecting to WiFi...");
   while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.print(".");
   }
   Serial.println(" connected!");
   Serial.print("Camera Stream Ready! Go to: http://");
   Serial.print(WiFi.localIP());
   Serial.println("/stream");
 
   // Start streaming server
   startCameraServer();
 }
 
 void loop() {
   // Handle incoming HTTP requests
   server.handleClient();
 }
 
 // ============ MJPEG STREAM HANDLER ============
 // Streams a continuous MJPEG feed to the client.
 void handle_jpg_stream() {
   WiFiClient client = server.client();
   if(!client.connected()) {
     return;
   }
 
   // Write standard HTTP headers for an MJPEG stream
   client.println("HTTP/1.1 200 OK");
   client.print("Content-Type: ");
   client.println(_STREAM_CONTENT_TYPE);
   client.println("Connection: close");
   client.println();
 
   while (true) {
     if(!client.connected()) {
       break;
     }
 
     camera_fb_t * fb = esp_camera_fb_get();
     if(!fb) {
       Serial.println("Camera capture failed");
       break;
     }
 
     // Send boundary between frames
     client.write((const uint8_t*)_STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
 
     // Write the part header (content-type, content-length)
     char partBuf[64];
     sprintf(partBuf, _STREAM_PART, fb->len);
     client.write((const uint8_t*)partBuf, strlen(partBuf));
 
     // Write the actual JPEG frame
     client.write(fb->buf, fb->len);
 
     // Return the buffer to be reused
     esp_camera_fb_return(fb);
 
     // Tiny delay can help with stability
     delay(1);
   }
 }
 
 // ============ SINGLE JPEG CAPTURE HANDLER ============
 // Captures a single frame and sends it once, then closes.
 void handle_jpg_capture() {
   camera_fb_t * fb = esp_camera_fb_get();
   if(!fb) {
     server.send(503, "text/plain", "Camera capture failed");
     return;
   }
 
   // We send headers, then status code, then manually write the data.
   server.sendHeader("Content-Type", "image/jpeg");
   server.sendHeader("Content-Length", String(fb->len));
   server.send(200, "image/jpeg", ""); // empty body for now
   server.client().write((const char *)fb->buf, fb->len);
 
   esp_camera_fb_return(fb);
 }
 
 // ============ START CAMERA SERVER ============
 void startCameraServer() {
   // MJPEG stream at /stream
   server.on("/stream", HTTP_GET, handle_jpg_stream);
 
   // Single capture at /capture
   server.on("/capture", HTTP_GET, handle_jpg_capture);
 
   server.begin();
   Serial.println("HTTP server started");
 }
 