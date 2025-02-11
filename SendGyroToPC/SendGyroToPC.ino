#include <WiFi.h>
#include <Wire.h>
#include <MPU6050_tockn.h>

MPU6050 mpu(Wire);

WiFiServer server(8888);

const char* ssid     = "SK_7458_2.4G";
const char* password = "HYV21@0527";

int counter = 0;

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  server.begin();  // Start listening on port 8888
  Serial.println("Server started, waiting for client...");

  Wire.begin(15, 14, 100000);
  mpu.begin();
  mpu.calcGyroOffsets(true);
  Serial.println("MPU6050 initialized and calibrated.");
}

void loop() {
    WiFiClient client = server.available();
    if (client) {
        while(client.connected()){
            // Update the MPU6050 data
            mpu.update();
            String dataString = String(mpu.getAccX()) + "," +
                                String(mpu.getAccY()) + "," +
                                String(mpu.getAccZ()) + "," +
                                String(mpu.getGyroX()) + "," +
                                String(mpu.getGyroY()) + "," +
                                String(mpu.getGyroZ()) + "," +
                                String(mpu.getTemp());


            Serial.print("sending");
            Serial.println(dataString);
            // Once connected, send data
            client.println(dataString);
            counter++;
            delay(20);
        }
    }
}
