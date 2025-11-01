import numpy as np
import serial
import cv2
import struct

class YUV422Reader:
    def __init__(self, port='/dev/cu.usbserial-A5XK3RJT', baudrate=921600):
        print("Initializing serial port...")
        self.serial_port = serial.Serial(port, baudrate)
        if (self.serial_port.is_open):
            print("Serial port is open.")
        else:
            print("Failed to open serial port.")
        self.frame_width = 320  # Default ESP32-CAM resolution
        self.frame_height = 240
        print("Serial port initialized.")
        
    def read_frame(self):
        # YUV422 has 2 bytes per pixel
        print("Reading frame...")
        expected_bytes = self.frame_width * self.frame_height * 2
        print(f"Expecting {expected_bytes} bytes for frame of size {self.frame_width}x{self.frame_height}")
        # Read raw data
        print("1")
        raw_data = self.serial_port.read(expected_bytes)
        print("raw data length:", len(raw_data))
        if len(raw_data) != expected_bytes:
            print(f"Incomplete frame received. Expected {expected_bytes} bytes, got {len(raw_data)}")
            
        # Convert raw data to numpy array
        yuv_array = np.frombuffer(raw_data, dtype=np.uint8)
        print("2")
        # Reshape to image dimensions with 2 bytes per pixel
        yuv_shaped = yuv_array.reshape((self.frame_height, self.frame_width, 2))
        print("3")
        # Convert YUV422 to BGR
        # YUV422 format: [Y0 U Y1 V] where U,V are shared between 2 pixels
        y = yuv_shaped[:, :, 0]
        u = yuv_shaped[::2, ::2, 1].repeat(2, axis=0).repeat(2, axis=1)
        v = yuv_shaped[1::2, ::2, 1].repeat(2, axis=0).repeat(2, axis=1)
        print("4")
        # Create YUV matrix
        yuv = cv2.merge([y, u, v])
        
        # Convert to BGR
        bgr = cv2.cvtColor(yuv, cv2.COLOR_YUV2BGR)
        print("5")
        return bgr
    
    def set_resolution(self, width, height):
        self.frame_width = width
        self.frame_height = height
        
    def close(self):
        if self.serial_port.is_open:
            self.serial_port.close()
            
    def __del__(self):
        self.close()

# Example usage
if __name__ == "__main__":
    reader = YUV422Reader()
    
    try:
        while True:
            # Read a frame
            frame = reader.read_frame()
            
            # Display the frame
            cv2.imshow('Frame', frame)
            
            # Break loop on 'q' press
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
                
    except KeyboardInterrupt:
        print("Stopping capture...")

    finally:
        reader.close()
        cv2.destroyAllWindows()