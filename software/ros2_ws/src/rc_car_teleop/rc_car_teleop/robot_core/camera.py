import numpy
import cv2
import rclpy
from rclpy.node import Node
from Electronics import Electronics

class Electronics:
    def __init__(self, name, status):
        self.name = name
        self.status = status

    def ShowStatus(self):
        print(f"{self.name} initialized.")

    def CheckStatus(self):
        print(f"{self.name} disconnected.")


class USBCamera(Electronics):
    def __init__(self, name, status, current=55e-3, voltage=1.8, height=720, width=1280, array_size=921600, fps=30, output_format="10-bit_RAW_RGB"):
        super().__init__(name, status, current, voltage)
        self.current = current              # 0.055A
        self.voltage = voltage              # 1.8V
        self.height = height                # 720 pixels
        self.width = width                  # 1080 pixels
        self.array_size = array_size        # 720*1080 = 921600 pixels
        self.fps = fps                      # 60 fps
        self.output_format = output_format  # 10-bit RAW RGB ---> (1280, 720, 3)
    
    # def GetFrames(self, array_size):



cap = cv2.VideoCapture(0)
while True:
    ret, frame = cap.read()         # grabs every next frame and passes it, ret (a boolean) --> gives True or False as to whether we manage to successfully receive the image
    if not ret:
        print("Frame capture failed. Exiting.")
        break
    cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    cv2.imshow('frame', frame)
    if cv2.waitKey(1) == ord('q'):
        break
    
cap.release()
cv2.destroyAllWindows()