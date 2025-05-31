# Sensor Interface repo
### this repository develops Sensor Interface that...
1. collect data from MPU-6050 IMU and OV2640 CAM
2. apply appropriate filter to sensor data
3. send this sensor data to Python on PC, with correct timestamp

## Folders
1. Intergrated code  
   intergrated code for ESP32
2. RecievePY  
   Python code that Recieve both sensor data. and saves it  
   * Recieve both sensor data
   [ ] Resampling while Recieving
   [ ] sending to Simulink while saving
   * Save as .csv and .avi or .jpg(-folder)
3. MATLAB model  
   Matlab file of SVC model (outdated)  
   - [ ] fix G block to use orientation data
4. Ref  
   Reference material
5. test  
   most recent dev testing code
