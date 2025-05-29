#include <Wire.h>
#include <MPU6050_tockn.h>

// Create a union to easily convert float to 4 bytes
typedef union {
  float number;
  uint8_t bytes[4];
} FLOATUNION_t;

MPU6050 mpu(Wire);

// Array of 6 union objects for acc and gyro data
FLOATUNION_t sensorData[6];

void setup() {
  Serial.begin(115200);  // Must match baud rate in Simulink
  Wire.begin(15, 14, 100000);  // SDA, SCL, freq
  mpu.begin();
  mpu.calcGyroOffsets(true);
  Serial.println("MPU6050 initialized and calibrated.");
}

void loop() {
  // Update the MPU6050 data
  mpu.update();

  // Read the sensor values
  float accX = mpu.getAccX();
  float accY = mpu.getAccY();
  float accZ = mpu.getAccZ();
  float gyroX = mpu.getGyroX();
  float gyroY = mpu.getGyroY();
  float gyroZ = mpu.getGyroZ();

  // Store them into our union array
  sensorData[0].number = accX;
  sensorData[1].number = accY;
  sensorData[2].number = accZ;
  sensorData[3].number = gyroX;
  sensorData[4].number = gyroY;
  sensorData[5].number = gyroZ;

  // Print header (1 byte)
  Serial.write('A');

  // Print 6 floats (6 * 4 = 24 bytes)
  for (int i = 0; i < 6; i++) {
    for (int b = 0; b < 4; b++) {
      Serial.write(sensorData[i].bytes[b]);
    }
  }

  // Print terminator
  Serial.print('\n');

  delay(50);  // Adjust to match the Simulink Serial Receive block sampling rate
}
