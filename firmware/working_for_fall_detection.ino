#include <Balaji2312-project-1_inferencing.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <TinyGPS++.h>

// OLED SETTINGS
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// MPU6050
Adafruit_MPU6050 mpu;

// GPS
TinyGPSPlus gps;

// UARTS
HardwareSerial gpsSerial(1);
HardwareSerial sim800(2);

// PINS
const int heartPin = 34;
const int buttonPin = 23;
const int buzzerPin = 5;

bool alertCancelled = false;

unsigned long lastFallTime = 0;

// AI BUFFER
static float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];

void setup() {

  Serial.begin(115200);

  // I2C
  Wire.begin(21, 22);

  // GPS UART
  gpsSerial.begin(9600, SERIAL_8N1, 19, 18);

  // GSM UART
  sim800.begin(9600, SERIAL_8N1, 16, 17);

  // OLED INIT
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {

    Serial.println("OLED FAILED");

    while (1);
  }

  // MPU6050 INIT
  if (!mpu.begin()) {

    Serial.println("MPU6050 FAILED");

    while (1);
  }

  // BUTTON + BUZZER
  pinMode(buttonPin, INPUT_PULLUP);

  pinMode(buzzerPin, OUTPUT);

  // START SCREEN
  display.clearDisplay();

  display.setTextSize(2);

  display.setTextColor(WHITE);

  display.setCursor(5,20);

  display.println("SafeStride");

  display.display();

  delay(2000);
}

void loop() {

  sensors_event_t a, g, temp;

  mpu.getEvent(&a, &g, &temp);

  // HEART SENSOR
  int signal = analogRead(heartPin);

  int bpm = map(signal, 0, 4095, 60, 120);

  // GPS READ
  while (gpsSerial.available()) {

    gps.encode(gpsSerial.read());
  }

  // OLED DISPLAY
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(20,0);
  display.println("HEART RATE");

  display.setTextSize(3);
  display.setCursor(20,20);
  display.print(bpm);

  display.setTextSize(1);
  display.setCursor(95,35);
  display.println("BPM");

  display.display();

  // SERIAL MONITOR
  Serial.println("========= SAFE STRIDE =========");

  Serial.print("Heart Rate : ");
  Serial.print(bpm);
  Serial.println(" BPM");

  if (gps.location.isValid()) {

    float latitude = gps.location.lat();
    float longitude = gps.location.lng();

    Serial.print("Latitude   : ");
    Serial.println(latitude, 6);

    Serial.print("Longitude  : ");
    Serial.println(longitude, 6);

    Serial.print("Google Maps: ");

    Serial.print("https://maps.google.com/?q=");

    Serial.print(latitude, 6);

    Serial.print(",");

    Serial.println(longitude, 6);

  } else {

    Serial.println("GPS Status : Searching...");
  }

  // =========================
  // AI FEATURE COLLECTION
  // =========================

  for (size_t i = 0;
       i < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE;
       i += EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME)
  {
      mpu.getEvent(&a, &g, &temp);

      features[i + 0] = a.acceleration.x;
      features[i + 1] = a.acceleration.y;
      features[i + 2] = a.acceleration.z;
      features[i + 3] = analogRead(34);

      delay(10);
  }

  // CREATE SIGNAL
  signal_t signal_ai;

  int err = numpy::signal_from_buffer(
      features,
      EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE,
      &signal_ai
  );

  if (err != 0) {

      Serial.println("Signal Error");

      return;
  }

  // RUN AI MODEL
  ei_impulse_result_t result = { 0 };

  err = run_classifier(
      &signal_ai,
      &result,
      false
  );

  if (err != EI_IMPULSE_OK) {

      Serial.println("Classifier Error");

      return;
  }

  // AI RESULTS
  float fallingValue = 0;

  Serial.println("========== AI RESULT ==========");

  for (size_t ix = 0;
       ix < EI_CLASSIFIER_LABEL_COUNT;
       ix++)
  {
      Serial.print(
          result.classification[ix].label
      );

      Serial.print(": ");

      Serial.println(
          result.classification[ix].value,
          5
      );

      // FALL LABEL
      if (String(result.classification[ix].label) == "falling") {

        fallingValue = result.classification[ix].value;
      }
  }

  Serial.println("================================");

  // =========================
  // AI FALL DETECTION
  // =========================

  static int fallCount = 0;

if (fallingValue > 0.80) {

    fallCount++;

} else {

    fallCount = 0;
}

// FALL CONFIRMED
if (fallCount >= 3 &&
    (millis() - lastFallTime > 15000)) {

    lastFallTime = millis();

    fallCount = 0;

    fallAlert(bpm);
}

  delay(500);
}

// FALL ALERT FUNCTION
void fallAlert(int bpm) {

  alertCancelled = false;

  display.clearDisplay();

  display.setTextSize(2);

  display.setCursor(15,10);

  display.println("FALL!");

  display.setTextSize(1);

  display.setCursor(0,45);

  display.println("Press Button");

  display.display();

  // BUZZER
  digitalWrite(buzzerPin, HIGH);

  unsigned long startTime = millis();

  while (millis() - startTime < 10000) {

    if (digitalRead(buttonPin) == LOW) {

      alertCancelled = true;

      break;
    }

    delay(10);
  }

  digitalWrite(buzzerPin, LOW);

  if (!alertCancelled) {

    sendSMS(bpm);
  }

  display.clearDisplay();

  display.display();
}

// SMS FUNCTION
void sendSMS(int bpm) {

  display.clearDisplay();

  display.setTextSize(1);

  display.setCursor(10,25);

  display.println("SENDING ALERT");

  display.display();

  sim800.println("AT");
  delay(1000);

  sim800.println("AT+CMGF=1");
  delay(1000);

  sim800.println("AT+CMGS=\"+917695820414\"");
  delay(1000);

  sim800.println("EMERGENCY! AI FALL DETECTED");

  sim800.print("Heart Rate: ");
  sim800.print(bpm);
  sim800.println(" BPM");

  if (gps.location.isValid()) {

    sim800.println("Location:");

    sim800.print("https://maps.google.com/?q=");

    sim800.print(gps.location.lat(),6);

    sim800.print(",");

    sim800.println(gps.location.lng(),6);

  } else {

    sim800.println("GPS Location Not Available");
  }

  sim800.write(26);

  delay(5000);

  display.clearDisplay();

  display.setTextSize(2);

  display.setCursor(0,20);

  display.println("ALERT SENT");

  display.display();

  delay(3000);
}
