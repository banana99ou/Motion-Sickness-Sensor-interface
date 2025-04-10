import pandas as pd
import numpy as np

file_path = "sensor_data.xlsx"

# 1) Read the Timestamp as plain text
df = pd.read_excel(
    file_path,
    dtype={"Timestamp": str},  # Force 'Timestamp' to be read as string
    engine="openpyxl"
)

# 2) Convert each string timestamp (e.g. "13:46:00.915") into a proper datetime
df["Timestamp"] = pd.to_datetime(df["Timestamp"], format="%H:%M:%S.%f")

# 3) Calculate time deltas in seconds
df["DeltaT"] = df["Timestamp"].diff().dt.total_seconds()

# Replace zero DeltaT with NaN, so we can interpolate
df["DeltaT"] = df["DeltaT"].replace(0, np.nan)

# Interpolate missing DeltaT values:
#  - linear method fills in any span of NaNs by distributing them evenly.
#  - limit_direction='both' fills from either side if possible
df["DeltaT"] = df["DeltaT"].interpolate(method="linear", limit_direction="both")

# 4) Convert angles (Gyro*) to angular speed (degrees/sec)
df["AngSpeedX"] = df["GyroX"].diff() / df["DeltaT"]
df["AngSpeedY"] = df["GyroY"].diff() / df["DeltaT"]
df["AngSpeedZ"] = df["GyroZ"].diff() / df["DeltaT"]

# Fill the first row’s NaN with 0 or handle as needed
df.fillna(0, inplace=True)

# 5) Save to a new file (DeltaT column is retained this time)
output_path = "sensor_data_processed-1.xlsx"
df.to_excel(output_path, index=False)
print(f"Saved to {output_path}")
