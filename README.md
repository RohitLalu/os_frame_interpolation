# Frame Interpolation System

This project implements a frame interpolation system using ESP32-CAM and Python-based computer vision. The system consists of two main components:

1. RTOS-based ESP32-CAM implementation (rtos_edf/)
2. Python-based Computer Vision processing (py_cv/)

## Project Structure

```
os_frame_interpolation/
├── py_cv/               # Python Computer Vision components
│   ├── serial_cam.py    # Serial camera interface
│   ├── yuv_2.py        # YUV processing
│   └── yuv422_reader.py # YUV422 format reader
│
└── rtos_edf/           # ESP32-CAM RTOS implementation
    ├── src/            # Source code
    │   ├── cam_tasks.cpp
    │   ├── cam_tasks.h
    │   ├── camera_pins.h
    │   ├── main.cpp
    │   └── task_config.h
    └── platformio.ini  # PlatformIO configuration
```

## Requirements

### Hardware
- AI-Thinker ESP32-CAM board
- USB-TTL converter for programming

### Software
- Python 3.x
- PlatformIO
- Required Python packages (OpenCV, etc.)

## Setup

### ESP32-CAM Setup
1. Install PlatformIO in your development environment
2. Navigate to the `rtos_edf` directory
3. Build and upload the project:
   ```
   pio run -t upload
   ```

### Python Setup
1. Navigate to the `py_cv` directory
2. Install required Python packages:
   ```
   python get-pip.py
   pip install -r requirements.txt
   ```

## Usage

1. Connect the ESP32-CAM to your computer
2. Run the Python script for frame processing:
   ```
   python serial_cam.py
   ```

## Features

- Real-time frame capture using ESP32-CAM
- EDF (Earliest Deadline First) scheduling for RTOS tasks
- JPEG format processing
- Frame interpolation using computer vision techniques

## Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Authors

- Rohit L
- Guhan Balaji
- Pranav Jois