#include <Arduino.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>

// ---------------------------
// DHT20 (AHT20) Setup
// ---------------------------
Adafruit_AHTX0 aht;

// I2C pins for YOUR Heltec S3 board
#define SDA_PIN 41
#define SCL_PIN 40

// ---------------------------
// Servo + LED + Buzzer pins
// ---------------------------
const int SERVO_PIN  = 19;  // Working servo pin
const int LED_PIN    = 5;
const int BUZZER_PIN = 6;

Servo dispenserServo;

// 3 segments of rotation
struct AngleRange {
  int startA;
  int endA;
};

AngleRange segments[] = {
  {0,   60},
  {60,  120},
  {120,  179}
};

int currentSegment = 0;
const int totalSegments = 4;

// timing
unsigned long lastMove = 0;
const unsigned long intervalMs = 5000; // 5 seconds

// Smooth servo motion
void smoothMove(int startA, int endA) {
  Serial.printf("Moving %d → %d\n", startA, endA);

  if (startA < endA) {
    for (int angle = startA; angle <= endA; angle += 2) {
      dispenserServo.write(angle);
      Serial.printf(" → at %d°\n", angle);
      delay(25);
    }
  } else {
    for (int angle = startA; angle >= endA; angle -= 2) {
      dispenserServo.write(angle);
      Serial.printf(" → at %d°\n", angle);
      delay(25);
    }
  }
}

void setup() {
  Serial.begin(9600);
  delay(300);

  // ---------------------------
  // Initialize I2C + AHT20
  // ---------------------------
  Wire.begin(SDA_PIN, SCL_PIN);

  if (!aht.begin()) {
    Serial.println("AHT20 not detected! Check wiring.");
    while (1);
  }
  Serial.println("AHT20 detected successfully.");

  // ---------------------------
  // Servo + LED + Buzzer
  // ---------------------------
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  dispenserServo.attach(SERVO_PIN, 500, 2400);
  dispenserServo.write(0);

  Serial.println("Servo initialized at 0°");

  delay(600);
  lastMove = millis();
}

void loop() {

  // ---------------------------------------
  // Read Temperature & Humidity EVERY loop
  // ---------------------------------------
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);

  float H = humidity.relative_humidity;
  float T = temp.temperature;

  Serial.printf("Temp: %.2f°C | Humidity: %.2f%%\n", T, H);

  // ---------------------------------------
  // HUMIDITY ALERT: If humidity > 70%
  // → BUZZER rings 5 times (non-blocking)
  // ---------------------------------------
  if (H > 70.0) {
    Serial.println("Humidity above 70% → ALERT BUZZER");

    for (int i = 0; i < 5; i++) {
      tone(BUZZER_PIN, 2000, 150);
      delay(200);
    }
  }
  // ----------------------
  // Servo timing logic
  // ----------------------
  if (currentSegment >= totalSegments) {
    Serial.println("\nAll 4 segments completed.");
    return;
  }

  if (millis() - lastMove >= intervalMs) {
    lastMove = millis();

    digitalWrite(LED_PIN, HIGH);
    tone(BUZZER_PIN, 2000, 150);

    int startA = segments[currentSegment].startA;
    int endA   = segments[currentSegment].endA;

    Serial.printf("\nSEGMENT %d: %d → %d\n",
                  currentSegment, startA, endA);

    smoothMove(startA, endA);

    currentSegment++;

    digitalWrite(LED_PIN, LOW);
  }

  delay(1000); // Slow down DHT20 readings
}
