#!/usr/bin/env python3
import cv2
import numpy as np
import requests
import json
import threading
import time
import datetime
import os

# ----- Configuration -----
# ESP32_IP = "192.168.2.10"  # Replace with your ESP32's actual IP
ESP32_IP = "192.168.0.71"
IMU_WINDOW_SEC = 2.0       # Show last 2 seconds of IMU data
save_data = True           # Set to True to record data

# Global containers for saving files and recording
imu_data = []              # List holding latest IMU samples (used for graphing)
imu_lock = threading.Lock()
imu_file = None            # File handle for saving IMU data (CSV)
stop_event = threading.Event()  # Event used to signal threads to end

# Global recording folder paths (for IMU CSV and frame images)
recording_folder = ""
frames_folder = ""
imu_filename = ""

# ----- IMU Stream Thread -----
imu_t0 = None  # First sample's absolute timestamp, to compute relative times

def imu_stream_thread():
    """
    Connects to the ESP32's IMU data stream (port 8888) and continuously processes JSON lines.
    Converts the timestamp from microseconds to seconds and calculates a relative timestamp.
    Also, writes each sample to the CSV file if saving is enabled.
    """
    global imu_t0, imu_data, imu_file
    stream_url = f"http://{ESP32_IP}:8888"
    print(f"[IMU] Connecting to IMU stream at {stream_url}...")
    
    try:
        with requests.get(stream_url, stream=True, timeout=10) as response:
            if response.status_code != 200:
                print(f"[IMU] Error: Received status code {response.status_code} from IMU stream.")
                return
            for line in response.iter_lines():
                if stop_event.is_set():
                    break
                if line:
                    try:
                        sample = json.loads(line.decode("utf-8"))
                        # Convert the timestamp from microseconds to seconds.
                        sample["t_s"] = sample["t_us"] / 1e6  
                        if imu_t0 is None:
                            imu_t0 = sample["t_s"]
                        # Relative time since first sample.
                        sample["t_rel"] = sample["t_s"] - imu_t0
                        
                        # Append to the global list (under lock) for live plotting.
                        with imu_lock:
                            imu_data.append(sample)
                            # Keep only samples within the IMU_WINDOW_SEC sliding window.
                            latest = imu_data[-1]["t_rel"]
                            imu_data[:] = [d for d in imu_data if (latest - d["t_rel"]) < IMU_WINDOW_SEC]
                        
                        # Also, if we're recording, write this sample to the CSV.
                        if save_data and imu_file:
                            # CSV header is: t_us,t_s,t_rel,ax,ay,az,gx,gy,gz
                            csv_line = f"{sample['t_us']},{sample['t_s']:.6f},{sample['t_rel']:.6f}," \
                                       f"{sample.get('ax',0):.3f},{sample.get('ay',0):.3f},{sample.get('az',0):.3f}," \
                                       f"{sample.get('gx',0):.3f},{sample.get('gy',0):.3f},{sample.get('gz',0):.3f}\n"
                            imu_file.write(csv_line)
                            imu_file.flush()
                    except Exception as e:
                        print(f"[IMU] Error decoding JSON: {e}")
    except Exception as e:
        print(f"[IMU] Exception in connecting to IMU stream: {e}")

# ----- Camera and Display Thread -----
def camera_and_display_thread():
    """
    Opens the MJPEG camera stream from ESP32 (port 8000 in this version),
    overlays FPS, displays the image with a live IMU graph beneath,
    and saves each frame as an individual JPEG with a timestamp in its filename.
    """
    stream_url = f"http://{ESP32_IP}:8000/stream"
    cap = cv2.VideoCapture(stream_url)
    if not cap.isOpened():
        print(f"[Camera] Error: Unable to open camera stream at {stream_url}")
        return

    # Attempt to grab one frame to get dimensions.
    ret, frame = cap.read()
    if not ret:
        print("[Camera] Error: Couldn't read frame for dimensions.")
        cap.release()
        return
    frame_height, frame_width = frame.shape[:2]
    
    frame_count = 0
    fps = 0.0
    start_time = time.time()
    
    while True:
        if stop_event.is_set():
            break

        ret, frame = cap.read()
        if not ret:
            continue

        # Save each frame as a JPEG image if recording is enabled.
        if save_data:
            timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S_%f")
            frame_filename = os.path.join(frames_folder, f"frame_{timestamp}.jpg")
            cv2.imwrite(frame_filename, frame)

        frame_count += 1
        elapsed = time.time() - start_time
        if elapsed >= 1.0:
            fps = frame_count / elapsed
            frame_count = 0
            start_time = time.time()

        # Overlay the FPS value on the frame.
        cv2.putText(frame, f"FPS: {fps:.2f}", (10, 25),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.8, (0, 255, 0), 2)

        # Create an empty graph image for IMU data.
        graph_height = 200
        graph_img = np.zeros((graph_height, frame_width, 3), dtype=np.uint8)

        # Copy the latest IMU data (thread-safe).
        with imu_lock:
            data_copy = imu_data.copy()

        if len(data_copy) >= 2:
            t_latest = data_copy[-1]["t_rel"]
            points_ax = []
            points_ay = []
            points_az = []
            points_gx = []
            points_gy = []
            points_gz = []
            for sample in data_copy:
                dt = t_latest - sample["t_rel"]
                # Map dt to an x coordinate: dt=0 -> right edge; dt=IMU_WINDOW_SEC -> left edge.
                x = int(frame_width - (dt / IMU_WINDOW_SEC) * frame_width)
                # Map sensor values to y. Assume a sensor range (for both acc and gyro) of roughly [-5,5].
                scale = (graph_height / 2) / 5.0  
                y_ax = int(graph_height / 2 - sample.get("ax", 0) * scale)
                y_ay = int(graph_height / 2 - sample.get("ay", 0) * scale)
                y_az = int(graph_height / 2 - sample.get("az", 0) * scale)
                y_gx = int(graph_height / 2 - sample.get("gx", 0) * scale)
                y_gy = int(graph_height / 2 - sample.get("gy", 0) * scale)
                y_gz = int(graph_height / 2 - sample.get("gz", 0) * scale)
                points_ax.append((x, y_ax))
                points_ay.append((x, y_ay))
                points_az.append((x, y_az))
                points_gx.append((x, y_gx))
                points_gy.append((x, y_gy))
                points_gz.append((x, y_gz))
            # Draw accelerometer curves.
            if len(points_ax) >= 2:
                cv2.polylines(graph_img, [np.array(points_ax, np.int32)], False, (0, 0, 255), 2)       # ax: Red
                cv2.polylines(graph_img, [np.array(points_ay, np.int32)], False, (0, 255, 0), 2)       # ay: Green
                cv2.polylines(graph_img, [np.array(points_az, np.int32)], False, (255, 0, 0), 2)       # az: Blue
                # Draw gyroscope curves.
                cv2.polylines(graph_img, [np.array(points_gx, np.int32)], False, (0, 255, 255), 2)     # gx: Yellow
                cv2.polylines(graph_img, [np.array(points_gy, np.int32)], False, (255, 0, 255), 2)     # gy: Magenta
                cv2.polylines(graph_img, [np.array(points_gz, np.int32)], False, (255, 255, 0), 2)     # gz: Cyan

        # Draw a horizontal line at the center of the graph.
        cv2.line(graph_img, (0, graph_height // 2), (frame_width, graph_height // 2), (255, 255, 255), 1)

        # Create a composite display: camera frame on top, IMU graph below.
        composite = np.vstack([frame, graph_img])
        cv2.imshow("ESP32 Sensor Data", composite)

        # Exit the loop if the ESC key is pressed.
        if cv2.waitKey(1) & 0xFF == 27:
            stop_event.set()
            break

    cap.release()
    cv2.destroyAllWindows()

# ----- Main -----
if __name__ == '__main__':
    # Create a recording folder with the current date and time to avoid overwriting previous files.
    dt_str = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    if save_data:
        recording_folder = f"recording_{dt_str}"
        os.makedirs(recording_folder, exist_ok=True)
        # Create a subfolder for frames.
        frames_folder = os.path.join(recording_folder, "frames")
        os.makedirs(frames_folder, exist_ok=True)
        # Prepare the IMU CSV file.
        imu_filename = os.path.join(recording_folder, f"imu_data_{dt_str}.csv")
        imu_file = open(imu_filename, "w")
        imu_file.write("t_us,t_s,t_rel,ax,ay,az,gx,gy,gz\n")
        print(f"[Main] Saving IMU data to {imu_filename}")
        print(f"[Main] Saving individual frames into {frames_folder}")

    # Start the IMU stream thread.
    imu_thread = threading.Thread(target=imu_stream_thread, daemon=True)
    imu_thread.start()

    # Run the camera display/recording function (main thread).
    camera_and_display_thread()

    # After camera loop exits, close the IMU file if open.
    if imu_file:
        imu_file.close()

    print("[Main] Data recording complete.")
