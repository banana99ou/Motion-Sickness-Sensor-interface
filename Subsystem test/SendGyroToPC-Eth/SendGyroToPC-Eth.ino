/*********************************************************************
 *  ESP32-CAM + W5500  ▸  MPU-6050 JSON streamer (no Wi-Fi / camera) *
 *********************************************************************/
#include <SPI.h>
#include <EthernetSPI2.h>
#include <Wire.h>
#include <MPU6050_tockn.h>

// ───── I²C (MPU-6050) ───────────────────────────────────────────────
#define I2C_SDA   15
#define I2C_SCL   16
MPU6050 mpu(Wire);

// ───── Ethernet / W5500 ─────────────────────────────────────────────
byte mac[]      = { 0xAE, 0x4B, 0x92, 0xE1, 0x35, 0x7C };
IPAddress ip    =  IPAddress(192, 168, 89, 11);     // adjust to your LAN
const uint16_t  IMU_PORT = 8888;
EthernetServer  server(IMU_PORT);

// ───── Timing ──────────────────────────────────────────────────────
const uint32_t  PERIOD_US = 10'000;   // 100 Hz
uint32_t        lastSend  = 0;

void setup()
{
  Serial.begin(115200);

  // ── IMU ──────────────────────────────────────────────────────────
  Wire.begin(I2C_SDA, I2C_SCL, 400000);           // 400 kHz fast-mode
  mpu.begin();
  mpu.calcGyroOffsets(true);                      // quick calibration

  // ── Ethernet / W5500 ────────────────────────────────────────────
  Ethernet.init(2);                               // CS pin for W5500
  Ethernet.begin(mac, ip);
  while (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Waiting for Ethernet link…");
    delay(500);
  }
  server.begin();
  Serial.printf("IMU JSON stream ➜  http://%s:%u/\n",
                Ethernet.localIP().toString().c_str(), IMU_PORT);
}

void loop()
{
  EthernetClient client = server.available();
  static bool headerSent = false;

  // ─── Handle an active connection ────────────────────────────────
  if (client) {
    if (!headerSent) {                       // send HTTP headers once
      client.println(F("HTTP/1.1 200 OK"));
      client.println(F("Content-Type: application/json"));
      client.println(F("Access-Control-Allow-Origin: *"));
      client.println();
      headerSent = true;
      Serial.println("Client connected");
    }

    uint32_t now = micros();
    if (now - lastSend >= PERIOD_US) {       // 100 Hz pacing
      lastSend = now;

      mpu.update();
      float ax = mpu.getAccX(),  ay = mpu.getAccY(),  az = mpu.getAccZ();
      float gx = mpu.getGyroX(), gy = mpu.getGyroY(), gz = mpu.getGyroZ();
      float gNorm = sqrtf(ax*ax + ay*ay + az*az);
      float gvx = ax / gNorm, gvy = ay / gNorm, gvz = az / gNorm;

      char buf[192];
      snprintf(buf, sizeof(buf),
        "{\"t_us\":%lu,\"ax\":%.3f,\"ay\":%.3f,\"az\":%.3f,"
        "\"gx\":%.3f,\"gy\":%.3f,\"gz\":%.3f,"
        "\"gvx\":%.3f,\"gvy\":%.3f,\"gvz\":%.3f}\n",
        (unsigned long)now, ax, ay, az, gx, gy, gz, gvx, gvy, gvz);
      client.print(buf);
    }

    if (!client.connected()) {               // clean up on disconnect
      client.stop();
      headerSent = false;
      Serial.println("Client disconnected");
    }
  }
}
