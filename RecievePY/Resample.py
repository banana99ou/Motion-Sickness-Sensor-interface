#!/usr/bin/env python3
import glob
import re
import datetime
import numpy as np
import pandas as pd
import os
import cv2
from tqdm import tqdm

fps = 100
Target_Hz = 100

# 1) Load IMU CSV and show t_rel
# imu_csv = glob.glob("recording_20250508_173409/imu_data_*.csv")[0]
path = "recording_20250603_165027"
# imu_csv = glob.glob(f"{path}/imu_data_*.csv")[0]
# imu_df = pd.read_csv(imu_csv, sep=",")
print("=== IMU t_rel (s) ===")
# print(imu_df["t_rel"].head().to_list(), "...", imu_df["t_rel"].tail().to_list())

# 2) Load frames and parse timestamps
# frame_files = sorted(glob.glob("recording_20250508_173455/frames/*.jpg"))
frame_files = sorted(glob.glob(f"{path}/frames/*.jpg"))

def parse_frame_ts(fname):
    """
    Try old format: frame_YYYYMMDD_HHMMSS_microsec.jpg
    Fallback to new format:       frame_<microsecond_timestamp>.jpg
    Returns a float timestamp in seconds.
    """
    base = os.path.basename(fname)

    # Old‐firmware pattern: date + usec
    m = re.match(r"frame_(\d{8}_\d{6})_(\d{6})\.jpg", base)
    if m:
        timestr, usec = m.groups()
        dt = datetime.datetime.strptime(timestr, "%Y%m%d_%H%M%S")
        return dt.timestamp() + int(usec) * 1e-6

    # New‐firmware pattern: a single integer == microseconds
    m = re.match(r"frame_(\d+)\.jpg", base)
    if m:
        ts_us = int(m.group(1))
        # assume this is an absolute epoch‐microsecond;
        # convert to seconds
        return ts_us * 1e-6

    raise ValueError(f"Unrecognized frame filename: {base}")

frame_times = np.array([parse_frame_ts(f) for f in frame_files])
frame_raw   = frame_times - frame_times[0]
print("\n=== Frame raw times (s) ===")
print(frame_raw[:5].tolist(), "...", frame_raw[-5:].tolist())

# 3) Duration check
# duration_imu   = imu_df["t_rel"].max()
duration_frame = frame_raw.max()
# print(f"\nDuration → IMU: {duration_imu:.3f}s, Frames: {duration_frame:.3f}s")

# 4) Generate uniform grids up to the shorter duration (frames)
t_end     = frame_raw.max()      # ≈1819.645 s
video_dt  = 1.0 / fps           # 30 Hz → one timestamp every ~0.0333 s
imu_dt    = 1.0 / Target_Hz          # 300 Hz → one timestamp every ~0.00333 s

video_times = np.arange(0.0, t_end, video_dt)
imu_times   = np.arange(0.0, t_end, imu_dt)

# 5) Test IMU interpolation on the first channel (e.g. 'ax')
imu_channels = ['ax','ay','az','gx','gy','gz']
# use relative IMU times:
# imu_raw = imu_df['t_rel'].to_numpy()

frame_raw = frame_times - frame_times[0]

# 7) Build and save the full resampled IMU CSV

# collect into a dict, then DataFrame
# imu_resampled = {'t_sec': imu_times}
# for ch in imu_channels:
    # imu_resampled[ch] = np.interp(imu_times, imu_raw, imu_df[ch].to_numpy())

# imu_out = pd.DataFrame(imu_resampled)

# save
# out_csv = f"{path}_imu_resampled.csv"
# imu_out.to_csv(out_csv, index=False)
# print(f"\nSaved {len(imu_out)} rows of resampled IMU data to {out_csv}")

# 8) Write out the 30 FPS video

# read one frame to get dimensions
first_img = cv2.imread(frame_files[0])
h, w      = first_img.shape[:2]

# set up writer (MP4, 30 FPS)
fourcc    = cv2.VideoWriter_fourcc(*'mp4v')
out_video = f"{path}_output.mp4"
writer    = cv2.VideoWriter(out_video, fourcc, fps, (w, h))

# loop through every target timestamp
for t in tqdm(video_times, desc="Writing frames", unit="frame"):
    idx = np.abs(frame_raw - t).argmin()
    img = cv2.imread(frame_files[idx])
    if img is None:
        raise RuntimeError(f"Failed to load {frame_files[idx]}")
    writer.write(img)


writer.release()
print(f"\nSaved {fps} FPS video to {out_video}")
