
import serial
import cv2
import numpy as np
import struct

SERIAL_PORT = "/dev/cu.usbserial-A5XK3RJT" 
BAUD_RATE = 921600
IMAGE_START_MARKER = b'*S*'
WORD_START_MARKER = b'*A*'

def decode_jpeg_to_bgr(frame_data):
    try:
        frame = cv2.imdecode(np.frombuffer(frame_data, dtype=np.uint8), cv2.IMREAD_COLOR)
        return frame
    except Exception as e:
        print(f"Error decoding JPEG: {e}")
        return None

def main():
    print("Connecting to ESP32...")
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=2)
    except Exception as e:
        print(f"Error: Could not open serial port {SERIAL_PORT}. {e}")
        return

    print(f"Listening on {SERIAL_PORT} at {BAUD_RATE}...")
    
    frameA = None
    frameB = None
    
    while True:
        try:
            ser.read_until(IMAGE_START_MARKER)
            len_bytes = ser.read(4)
            if len(len_bytes) != 4:
                print("Timeout: Could not read frame length")
                continue
            
            frame_len = struct.unpack('<L', len_bytes)[0]
            
            # Sanity check (a QVGA JPEG won't be > 50KB)
            if frame_len > 50000 or frame_len < 1000:
                print(f"Error: Bad frame length. Got {frame_len}. Re-syncing...")
                ser.flushInput()
                continue

            frame_data = ser.read(frame_len)
            if len(frame_data) != frame_len:
                print(f"Timeout: Incomplete frame data. Expected {frame_len}, got {len(frame_data)}")
                continue

            new_frame = decode_jpeg_to_bgr(frame_data)
            
            if new_frame is not None:                
                # Shift frames: B becomes A, new_frame becomes B
                frameA = frameB
                frameB = new_frame
                
                # Display the raw stream (Frame B)
                cv2.imshow("Raw 3 FPS Stream (Frame B)", frameB)

                if frameA is not None:
                    interp_frame = cv2.addWeighted(frameA, 0.5, frameB, 0.5, 0.0)
                    cv2.imshow("Previous Frame (Frame A)", frameA)
                    cv2.imshow("Interpolated Frame", interp_frame)
                
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
                
        except KeyboardInterrupt:
            print("Stopping...")
            break
        except Exception as e:
            print(f"An error occurred: {e}")
            ser.flushInput()

    ser.close()
    cv2.destroyAllWindows()
    print("Stream stopped. Serial port closed.")

if __name__ == "__main__":
    main()