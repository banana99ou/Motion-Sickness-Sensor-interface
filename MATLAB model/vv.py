import cv2

class StreamProcessor:
    def __init__(self, source):
        # source can be a filename, HTTP/RTSP URL, etc.
        self.cap = cv2.VideoCapture(source)

    def step(self):
        ret, frame = self.cap.read()
        if not ret:
            # handle end-of-stream or reconnect
            self.cap.release()
            self.cap = cv2.VideoCapture(source)
            ret, frame = self.cap.read()
        # now do any OpenCV processing on frame
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        # return whatever you need back to Simulink
        return gray

