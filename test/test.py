import re
import pandas as pd

# Sample data string from the sensor output
data = 

# Regular expression pattern to extract the timestamp and sensor values
pattern = r'(\d{2}:\d{2}:\d{2}\.\d{3}) -> ([\d\.\-]+),([\d\.\-]+),([\d\.\-]+),([\d\.\-]+),([\d\.\-]+),([\d\.\-]+),([\d\.\-]+)'

# Lists to hold each field's data
timestamps = []
accelx = []
accely = []
accelz = []
gyrox = []
gyroy = []
gyroz = []
temp = []

# Process each line of the data
for line in data.splitlines():
    match = re.match(pattern, line)
    if match:
        timestamps.append(match.group(1))
        accelx.append(match.group(2))
        accely.append(match.group(3))
        accelz.append(match.group(4))
        gyrox.append(match.group(5))
        gyroy.append(match.group(6))
        gyroz.append(match.group(7))
        temp.append(match.group(8))

# Create a DataFrame from the parsed data
df = pd.DataFrame({
    "Timestamp": timestamps,
    "AccelX": accelx,
    "AccelY": accely,
    "AccelZ": accelz,
    "GyroX": gyrox,
    "GyroY": gyroy,
    "GyroZ": gyroz,
    "Temp": temp
})

# Convert numeric columns from strings to floats
numeric_cols = ["AccelX", "AccelY", "AccelZ", "GyroX", "GyroY", "GyroZ", "Temp"]
df[numeric_cols] = df[numeric_cols].astype(float)

# Print the DataFrame (or export it as a CSV file for Excel)
print(df)
# To export to CSV, uncomment the next line:
# df.to_csv("sensor_data.csv", index=False)