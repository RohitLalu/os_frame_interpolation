import serial
import cv2
import numpy as np
import struct

# --- Configuration ---
SERIAL_PORT = "COM3"  # Change this to your FTDI's port
BAUD_RATE = 921600
START_MARKER = b'*S*'
# ---------------------

def main():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    except Exception as e:
        print(f"Error opening serial port {SERIAL_PORT}: {e}")
        return

    print(f"Listening on {SERIAL_PORT} at {BAUD_RATE}...")
    
    while True:
        # 1. Wait for the start marker
        ser.read_until(START_MARKER)
        
        # 2. Read the 4-byte length
        len_bytes = ser.read(4)
        if len(len_bytes) != 4:
            print("Timeout or error reading frame length")
            continue
            
        frame_len = struct.unpack('<L', len_bytes)[0] # '<L' = unsigned long, little-endian

        # 3. Read the JPEG data (exactly frame_len bytes)
        frame_data = ser.read(frame_len)
        if len(frame_data) != frame_len:
            print(f"Timeout or error reading frame data (expected {frame_len}, got {len(frame_data)})")
            continue

        # 4. Decode the JPEG data
        try:
            frame = cv2.imdecode(np.frombuffer(frame_data, dtype=np.uint8), cv2.IMREAD_COLOR)
            if frame is not None:
                # 5. Display the frame
                cv2.imshow("ESP32-CAM Serial Stream", frame)
                
                # Exit on 'q' key
                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break
            else:
                print("Error decoding frame")
        except Exception as e:
            print(f"Error processing frame: {e}")

    ser.close()
    cv2.destroyAllWindows()

if __name__ == '__main__':
    main()