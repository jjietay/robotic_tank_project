class Camera(Sensor):
    def __init__(self, name, status, current=55e-3, voltage=1.8, height=720, width=1280, array_size=921600, fps=30, output_format="10-bit_RAW_RGB"):
        super().__init__(name, status, current, voltage)
        self.current = current              # 0.055A
        self.voltage = voltage              # 1.8V
        self.height = height                # 720 pixels
        self.width = width                  # 1080 pixels
        self.array_size = array_size        # 720*1080 = 921600 pixels
        self.fps = fps                      # 60 fps
        self.output_format = output_format  # 10-bit RAW RGB ---> (1280, 720, 3)
    
    def GetFrames(self, array_size):