#include <SPI.h>
#include <EthernetSPI2.h>
#include <Wire.h>
#include <MPU6050_tockn.h>

// ───── LEDS ─────────────────────────────────────────────────────────
#define LED 33
#define FLED 4

// ───── I²C (MPU-6050) ───────────────────────────────────────────────
#define I2C_SDA   15
#define I2C_SCL   16
MPU6050 mpu(Wire);

// ───── Ethernet / W5500 ─────────────────────────────────────────────
// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[]      = { 0xAE, 0x4B, 0x92, 0xE1, 0x35, 0x7C };
IPAddress ip(192, 168, 2, 3);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer  server(8888);

// ───── Timing ──────────────────────────────────────────────────────
const uint32_t  PERIOD_US = 10'000;   // 100 Hz
uint32_t        lastSend  = 0;

void setup()
{
  pinMode(LED, OUTPUT);
  pinMode(FLED, OUTPUT);
  Serial.begin(115200);

  // ── IMU ──────────────────────────────────────────────────────────
  Wire.begin(I2C_SDA, I2C_SCL, 400000);           // 400 kHz fast-mode
  mpu.begin();
  Serial.println("cal");
  mpu.calcGyroOffsets(true);                      // quick calibration
  Serial.println("fin");

  // ── Ethernet / W5500 ────────────────────────────────────────────
  Ethernet.init(2);
  //Ethernet.begin(mac, ip); // start the Ethernet connection and the server:
  Ethernet.begin(mac); 

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
    //   delay(200);
    //   digitalWrite(LED, HIGH);
    //   digitalWrite(FLED, HIGH);
    //   delay(200);
    //   digitalWrite(LED, LOW);
    //   digitalWrite(FLED, LOW);
    }
  }
  while (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is NOT connected.");
      // delay(500);
      // digitalWrite(LED, HIGH);
      // digitalWrite(FLED, HIGH);
      // delay(500);
      // digitalWrite(LED, LOW);
      // digitalWrite(FLED, LOW);
  }
  
  Serial.println("\nEthernet cable is now connected.");
  
  server.begin();
  delay(500);
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  LEDFLASHING();
}

void LEDFLASHING(){
  delay(500);
  digitalWrite(LED, HIGH);
  digitalWrite(FLED, HIGH);
  delay(500);
  digitalWrite(LED, LOW);
  digitalWrite(FLED, LOW);
  delay(500);
  digitalWrite(LED, HIGH);
  digitalWrite(FLED, HIGH);
  delay(500);
  digitalWrite(LED, LOW);
  digitalWrite(FLED, LOW);
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
