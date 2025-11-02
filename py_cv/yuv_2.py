# import serial
# import cv2
# import numpy as np
# import struct

# # --- Configuration ---
# SERIAL_PORT = "/dev/cu.usbserial-A5XK3RJT" # Change this! (e.g., "COM3")
# BAUD_RATE = 921600                         # Must match your ESP32
# FRAME_WIDTH = 320
# FRAME_HEIGHT = 240
# # ---------------------

# # Calculate the expected number of bytes for a YUV422 frame
# # (WIDTH * HEIGHT * 2 bytes/pixel)
# EXPECTED_FRAME_LEN = FRAME_WIDTH * FRAME_HEIGHT * 2

# def decode_yuv_to_bgr(frame_data):
#     """Converts a raw YUV422 byte buffer to an OpenCV BGR frame."""
#     try:
#         # 1. Create a 1D numpy array from the raw byte buffer
#         yuv_array = np.frombuffer(frame_data, dtype=np.uint8)
        
#         # 2. Reshape the array into the correct YUV422 dimensions
#         yuv_img = yuv_array.reshape((FRAME_HEIGHT, FRAME_WIDTH, 2))
        
#         # 3. Convert from YUV to BGR (what OpenCV uses)
#         #    cv2.COLOR_YUV2BGR_Y422 is the specific format from the cam
#         bgr_frame = cv2.cvtColor(yuv_img, cv2.COLOR_YUV2BGR_Y422)
        
#         return bgr_frame
#     except Exception as e:
#         print(f"Error decoding frame: {e}")
#         return None

# def main():
#     print("Connecting to ESP32...")
#     try:
#         # Add a 2-second timeout to handle any sync issues
#         ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=2)
#     except Exception as e:
#         print(f"Error: Could not open serial port {SERIAL_PORT}. {e}")
#         return

#     print(f"Listening on {SERIAL_PORT} at {BAUD_RATE}...")
    
#     while True:
#         try:
#             # 1. Synchronize: Read until a '*' marker is found
#             # This efficiently discards any junk/partial data
#             ser.read_until(b'*')
            
#             # 2. Read the 2-byte frame type (e.g., 'A*', 'I*', 'B*')
#             frame_type_bytes = ser.read(2)
#             if len(frame_type_bytes) != 2:
#                 print("Timeout waiting for frame type")
#                 continue

#             # 3. Read the 4-byte length
#             len_bytes = ser.read(4)
#             if len(len_bytes) != 4:
#                 print("Timeout waiting for frame length")
#                 continue
            
#             # Unpack the 4 bytes (little-endian unsigned long)
#             frame_len = struct.unpack('<L', len_bytes)[0]
            
#             # 4. Sanity Check
#             if frame_len != 153600:
#                 print(f"Error: Bad frame length. Expected {EXPECTED_FRAME_LEN}, got {frame_len}. Re-syncing...")
#                 ser.flushInput() # We are desynced, clear the buffer
#                 continue

#             # 5. Read the full frame data
#             frame_data = ser.read(frame_len)
#             if len(frame_data) != frame_len:
#                 print(f"Timeout: Incomplete frame data. Expected {frame_len}, got {len(frame_data)}")
#                 continue

#             # 6. Decode and display the frame in the correct window
#             bgr_frame = decode_yuv_to_bgr(frame_data)
            
#             if bgr_frame is not None:
#                 if frame_type_bytes == b'A*':
#                     cv2.imshow("Frame A (Real)", bgr_frame)
#                 elif frame_type_bytes == b'I*':
#                     cv2.imshow("Frame I (Interpolated)", bgr_frame)
#                 elif frame_type_bytes == b'B*':
#                     cv2.imshow("Frame B (Real)", bgr_frame)

#             # Break loop on 'q' press
#             if cv2.waitKey(1) & 0xFF == ord('q'):
#                 break
                
#         except KeyboardInterrupt:
#             print("Stopping...")
#             break
#         except Exception as e:
#             print(f"An error occurred: {e}")
#             ser.flushInput() # Flush on error and try to re-sync

#     # Clean up
#     ser.close()
#     cv2.destroyAllWindows()
#     print("Stream stopped. Serial port closed.")

# if __name__ == "__main__":
#     main()

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