
import time

class Sensor:
    def __init__(self, name, status, current, voltage):
        self.name = name
        self.current = current
        self.voltage = voltage
        self.status = status
    
    def ShowStatus(self, name):
        print(f"{name} initialized.")

    def CheckStatus(self, name):
        print(f"{name} disconnected.")


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
        return 

class Ultrasonic(Sensor):
    def __init__(self, name, status, trigger=10e-6, duration_low=70e-3, duration_high=10e-3, current=15e-3, voltage=5, min_range=2, max_range=4e2, angle_of_measurement=15, sound_vel=340):
        super().__init__(name, status, current, voltage)
        self.trigger = trigger                              # every 10e-6s
        self.current = current                              # 0.015A
        self.voltage = voltage                              # 5V
        self.min_range = min_range                          # 2 cm
        self.max_range = max_range                          # 4e2 cm
        self.angle_of_measurement = angle_of_measurement    # 15 degrees
        self.sound_vel = sound_vel

    def CalTravelTime(self):
        start_time = time.ticks_us()
        end_time = time.ticks_us()
        return end_time - start_time

    def distance(self, travel_time):                        # Input: travel_time
        return (travel_time * self.sound_vel) / 2           # Output: distance from object


    