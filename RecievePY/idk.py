#!/usr/bin/env python3
import requests
import cv2
import numpy as np

# ——— CONFIG ———
ESP32_IP   = "192.168.2.2"
ESP32_PORT = 80
URL = f"http://{ESP32_IP}:{ESP32_PORT}/"   # or add the exact path if different

# MJPEG boundary markers
JPEG_SOI = b'\xff\xd8'  # start of image
JPEG_EOI = b'\xff\xd9'  # end of image

def stream_and_display():
    """
    Opens a persistent HTTP connection to the ESP32 MJPEG stream,
    accumulates bytes until a full JPEG is found, then decodes and shows it.
    """
    # open the HTTP stream
    stream = requests.get(URL, stream=True)
    if stream.status_code != 200:
        print(f"Failed to connect, status code {stream.status_code}")
        return

    buffer = b''
    try:
        for chunk in stream.iter_content(chunk_size=4096):
            if not chunk:
                continue
            buffer += chunk

            # look for JPEG start and end markers in the buffer
            start = buffer.find(JPEG_SOI)
            end   = buffer.find(JPEG_EOI, start + 2)

            if start != -1 and end != -1:
                jpg = buffer[start:end+2]              # extract complete JPEG
                buffer = buffer[end+2:]                # remove processed bytes

                # decode to OpenCV image
                img = cv2.imdecode(
                    np.frombuffer(jpg, dtype=np.uint8),
                    cv2.IMREAD_COLOR
                )
                if img is not None:
                    cv2.imshow("ESP32-CAM Stream", img)
                    if cv2.waitKey(1) & 0xFF == ord('q'):
                        break
    except KeyboardInterrupt:
        pass
    finally:
        cv2.destroyAllWindows()
        stream.close()

if __name__ == "__main__":
    print(f"Connecting to ESP32 at {URL}")
    stream_and_display()
