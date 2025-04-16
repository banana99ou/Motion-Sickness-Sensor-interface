/*
   Read MPU6050 data (acceleration, gyro, temperature)
   using ESP32-CAM pins 14 (SCL) and 15 (SDA).

   Library used: MPU6050_tockn
   Install via Arduino Library Manager:
     Sketch -> Include Library -> Manage Libraries -> Search "MPU6050_tockn"
*/
#define LED_BUILTIN 4

#include <Wire.h>
#include <MPU6050_tockn.h>

MPU6050 mpu(Wire);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  delay(100);

  // Initialize I2C using SDA = 15, SCL = 14
  // The third parameter is the I2C frequency (100kHz here).
  Wire.begin(15, 14, 100000);

  // Initialize MPU6050
  analogWrite(LED_BUILTIN, 10);
  mpu.begin();
  // Optional: calibrate gyro offsets while printing details
  mpu.calcGyroOffsets(true);
  analogWrite(LED_BUILTIN, 0);

  Serial.println("MPU6050 initialized and calibrated.");
}

void loop() {
  // Update the MPU6050 data
  mpu.update();

  // Prepare CSV line in the format:
  // AccX,AccY,AccZ,GyroX,GyroY,GyroZ,Temp
  String dataString = String(mpu.getAccX()) + "," +
                      String(mpu.getAccY()) + "," +
                      String(mpu.getAccZ()) + "," +
                      String(mpu.getGyroX()) + "," +
                      String(mpu.getGyroY()) + "," +
                      String(mpu.getGyroZ()) + "," +
                      String(mpu.getTemp());

  Serial.println(dataString);

  delay(20); // ~50Hz data rate (adjust as needed)
}
