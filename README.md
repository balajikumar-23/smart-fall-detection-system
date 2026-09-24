# Smart Fall Detection and Health Monitoring System

An ESP32-based wearable safety device that uses an on-device Edge AI model to detect falls in real time, 
monitors heart rate, and sends an emergency SMS with the wearer's GPS location if a fall is confirmed.

## Hardware Used
- ESP32 Dev Board
- MPU6050 (accelerometer + gyroscope) — motion data for fall detection
- Pulse Sensor / Heart Rate Sensor module (analog) — BPM monitoring
- NEO-6M GPS module
- SIM800L GSM module — for sending emergency SMS
- SSD1306 OLED display — live status/BPM display
- Push button — to cancel a false alert
- Buzzer — audible fall alert

## How It Works
1. Motion data (accelerometer + gyroscope) is continuously sampled from the MPU6050.
2. The data is fed into a machine learning model trained on 4 activity classes — **Falling, Sitting, 
   Standing, Walking** — using [Edge Impulse](https://edgeimpulse.com).
3. The model was exported as a C++ inference library and deployed directly on the ESP32 (fully on-device, 
   no cloud/internet dependency for detection).
4. If the model classifies "falling" with high confidence for 3 consecutive readings, a fall is confirmed.
5. On confirmation, the buzzer sounds and a 10-second window is given to cancel the alert via the push button 
   (to avoid false-alarm SMS spam).
6. If not cancelled, the system sends an SMS via the SIM800L GSM module containing the wearer's current heart 
   rate and a Google Maps link to their GPS location.
7. Heart rate (BPM) and GPS coordinates are also shown live on the OLED display.

## Machine Learning
- Platform: Edge Impulse
- Classes: Falling, Sitting, Standing, Walking
- Input: raw accelerometer + gyroscope samples (6-axis)
- Deployment: exported as an Arduino-compatible C++ inference library, running fully on-device on the ESP32

## Repository Structure
- `firmware/` — main ESP32 Arduino sketch
- `model/` — exported Edge Impulse inference library
- `dataset/` — raw motion data collected for each activity class
- `docs/` — circuit diagram / demo media

## Setup
1. Install the Arduino IDE and ESP32 board support.
2. Install the required libraries: Adafruit_MPU6050, Adafruit_SSD1306, Adafruit_GFX, TinyGPS++.
3. Add the exported Edge Impulse library from `model/` to your Arduino libraries folder.
4. Wire the components per the pin definitions in the sketch.
5. Upload `firmware/working_for_fall_detection.ino` to the ESP32.