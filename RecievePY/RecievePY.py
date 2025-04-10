import requests
import cv2
import numpy as np

ESP32_IP = "192.168.0.71"  # <-- Replace with your ESP32 Camera IP
STREAM_URL = f"http://{ESP32_IP}:81/stream"

def mjpeg_stream():
    # Open a single HTTP connection in streaming mode
    r = requests.get(STREAM_URL, stream=True)
    if r.status_code != 200:
        print(f"Failed to connect: status={r.status_code}")
        return
    
    # The boundary used by the ESP32's MJPEG stream:
    boundary = b"\r\n--123456789000000000000987654321\r\n"

    img_buffer = b""
    
    try:
        # Read the response in chunks
        for chunk in r.iter_content(chunk_size=1024):
            if not chunk:
                print("Received empty chunk.")
                continue
            print(f"Received chunk of size {len(chunk)}")
            img_buffer += chunk
            
            # Search for the boundary in the buffer
            while True:
                boundary_index = img_buffer.find(boundary)
                if boundary_index < 0:
                    # No boundary found yet; continue reading chunks
                    break
                
                # We found a boundary => everything before it is one frame (plus header info)
                frame_data = img_buffer[:boundary_index]
                # Remove that part from the buffer
                img_buffer = img_buffer[boundary_index + len(boundary):]
                
                # The frame_data includes headers like:
                #   \r\nContent-Type: image/jpeg\r\nContent-Length: ...
                #   ...\r\n\r\n<JPEG BINARY DATA>
                # We need to skip those headers to extract the raw JPEG.
                
                # Find the header/content separator: \r\n\r\n
                header_end = frame_data.find(b"\r\n\r\n")
                if header_end == -1:
                    continue  # incomplete header
                
                # The JPEG data starts right after the blank line
                jpeg_data = frame_data[header_end+4:]
                
                # Decode the JPEG to a numpy image (BGR in OpenCV)
                frame = cv2.imdecode(np.frombuffer(jpeg_data, dtype=np.uint8), cv2.IMREAD_COLOR)
                if frame is not None:
                    cv2.imshow("ESP32 MJPEG Stream", frame)
                    # If user presses 'q', exit
                    if cv2.waitKey(1) & 0xFF == ord('q'):
                        return
                else:
                    print("Warning: failed to decode JPEG frame.")
    finally:
        cv2.destroyAllWindows()

if __name__ == "__main__":
    print("Hello World")
    mjpeg_stream()
