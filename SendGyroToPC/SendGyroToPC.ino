#include <WiFi.h>
#include <Wire.h>
#include <MPU6050_tockn.h>

MPU6050 mpu(Wire);

WiFiServer server(8888);

const char* ssid     = "FMCL";
const char* password = "66955144";

int counter = 0;

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");

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
            float dataOut [6];
            dataOut[0] = mpu.getAccX();
            dataOut[1] = mpu.getAccY();
            dataOut[2] = mpu.getAccZ();
            dataOut[3] = mpu.getGyroX();
            dataOut[4] = mpu.getGyroY();
            dataOut[5] = mpu.getGyroZ();
            //dataOut[6] = mpu.getTemp();


            Serial.println("sending");
            Serial.println(String(dataOut[0]));
            // Once connected, send data
            client.write(reinterpret_cast<uint8_t*>(dataOut), sizeof(dataOut));
            delay(20);
        }
    }
}
