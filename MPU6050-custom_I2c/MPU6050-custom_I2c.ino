/*
   Read MPU6050 data (acceleration, gyro, temperature)
   using ESP32-CAM pins 14 (SCL) and 15 (SDA).

   Library used: MPU6050_tockn
   Install via Arduino Library Manager:
     Sketch -> Include Library -> Manage Libraries -> Search "MPU6050_tockn"
*/

#include <Wire.h>
#include <MPU6050_tockn.h>

MPU6050 mpu(Wire);

void setup() {
  Serial.begin(115200);
  delay(100);

  // Initialize I2C using SDA = 15, SCL = 14
  // The third parameter is the I2C frequency (100kHz here).
  Wire.begin(15, 14, 100000);

  // Initialize MPU6050
  mpu.begin();
  // Optional: calibrate gyro offsets while printing details
  mpu.calcGyroOffsets(true);

  Serial.println("MPU6050 initialized and calibrated.");
}

void loop() {
  // Update MPU6050 data
  mpu.update();

  // Print the raw data
  Serial.print("Temp = ");
  Serial.print(mpu.getTemp());
  Serial.print("°C, ");

  Serial.print("AccX = ");
  Serial.print(mpu.getAccX());
  Serial.print(", AccY = ");
  Serial.print(mpu.getAccY());
  Serial.print(", AccZ = ");
  Serial.print(mpu.getAccZ());
  Serial.print(", ");

  Serial.print("GyroX = ");
  Serial.print(mpu.getGyroX());
  Serial.print(", GyroY = ");
  Serial.print(mpu.getGyroY());
  Serial.print(", GyroZ = ");
  Serial.print(mpu.getGyroZ());
  Serial.println();

  delay(100);  // Adjust delay as needed
}
