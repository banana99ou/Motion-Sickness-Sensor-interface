#include <SPI.h>
#include <EthernetSPI2.h>
#include <Wire.h>
#include <MPU6050_tockn.h>

//LEDS
#define LED 33
#define FLED 4

// ----------------- Network Settings -----------------
// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
0x02, 0xAB, 0xCD, 0xEF, 0x00, 0x01
};
//IPAddress ip(192, 168, 89, 11);
IPAddress ip(192, 168, 89, 11);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(8000);
// ----------------- MPU6050 Settings -----------------
MPU6050 mpu(Wire);

void setup() {
  // Serial for debug
  Serial.begin(115200);

  Ethernet.init(2);
  //Ethernet.begin(mac, ip); // start the Ethernet connection and the server:
  Ethernet.begin(mac); 

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      // delay(200);
      // digitalWrite(LED, HIGH);
      // digitalWrite(FLED, HIGH);
      // delay(200);
      // digitalWrite(LED, LOW);
      // digitalWrite(FLED, LOW);
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

  Serial.println("Ethernet cable is now connected.");
  
  server.begin();
  delay(500);
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());

  // ------ I2C for MPU6050 ------
  // SDA = GPIO15, SCL = GPIO16, 100 kHz
  Wire.begin(15, 16, 100000);
  mpu.begin();
  mpu.calcGyroOffsets(true);
  Serial.println("MPU6050 initialized and calibrated.");
}

void loop() {
    boolean biocr;
    boolean biolf;
    boolean doubleblank;
    boolean singleblank;

    EthernetClient client = server.available();
    if (client) {
        Serial.println("new client");
        // an http request ends with a blank line
        doubleblank = false;
        singleblank = true;
        biocr = false;
        biolf = false;

        while (client.connected()) {
            mpu.update();
            float dataOut[6] = {
                mpu.getAccX(),
                mpu.getAccY(),
                mpu.getAccZ(),
                mpu.getGyroX(),
                mpu.getGyroY(),
                mpu.getGyroZ()
            };

            // Send raw float array over TCP
            client.write(reinterpret_cast<uint8_t*>(dataOut), sizeof(dataOut));
            // Serial.print(string(dataOut));
            Serial.println(" Sent.");
        }
        delay(10);
        client.stop();
        Serial.println("client disconnected");
    }
}
