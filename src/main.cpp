#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP32Servo.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_CAP1188.h>
#include <ArduinoJson.h>

// =============================
// I2C SETUP (Heltec ESP32-S3)
// =============================
#define SDA_PIN 41
#define SCL_PIN 40

// =============================
// HARDWARE PINS (DO NOT CHANGE)
// =============================
const int SERVO_PIN  = 19;
const int LED_PIN    = 5;
const int BUZZER_PIN = 6;

// =============================
// WIFI CONFIG
// =============================
const char* WIFI_SSID = "IOT";
const char* WIFI_PASS = "Tanya123";

// =============================
// AZURE IOT HUB (HTTPS)
// =============================
const char* AZURE_HOST = "cs147Group9IotHub.azure-devices.net";
const int AZURE_PORT = 443;
const char* AZURE_SAS =
  "SharedAccessSignature sr=cs147Group9IotHub.azure-devices.net%2Fdevices%2F147esp32&sig=VHvdOUM%2FHZuFoQeu8sXud%2FlT16yDSjw%2BbjYG5C2IvnY%3D&se=1763680997";

// =============================
// OBJECTS
// =============================
WiFiClientSecure secureClient;
Servo dispenserServo;
Adafruit_AHTX0 aht;
Adafruit_CAP1188 cap;

// =============================
// SERVO SEGMENTS
// =============================
struct AngleRange {
  int startA;
  int endA;
};

AngleRange segments[] = {
  {0,   60},
  {60,  120},
  {120, 179}
};

int currentSegment = 0;
const int totalSegments = 3;

// =============================
// TIMING
// =============================
unsigned long lastAutoDispense = 0;
const unsigned long AUTO_INTERVAL_MS = 10000; // 10 seconds

// =============================
// TOUCH SEQUENCE (1 → 2 → 3 → 4)
// =============================
const uint8_t requiredSequence[] = {0, 1, 2, 3};
int sequenceIndex = 0;
unsigned long lastTouchTime = 0;
const unsigned long SEQUENCE_TIMEOUT = 3000;
uint8_t previousTouch = 0;

// =============================
// SMOOTH SERVO MOVE
// =============================
void smoothMove(int startA, int endA) {
  Serial.printf("Servo moving %d → %d\n", startA, endA);

  if (startA < endA) {
    for (int a = startA; a <= endA; a += 2) {
      dispenserServo.write(a);
      delay(25);
    }
  } else {
    for (int a = startA; a >= endA; a -= 2) {
      dispenserServo.write(a);
      delay(25);
    }
  }
}

// =============================
// AZURE LOGGING
// =============================
void sendToAzure(float temp, float hum, const char* eventType) {
  StaticJsonDocument<256> doc;
  doc["deviceId"] = "147esp32";
  doc["temperature"] = temp;
  doc["humidity"] = hum;
  doc["event"] = eventType;

  String payload;
  serializeJson(doc, payload);

  secureClient.setInsecure(); // OK for class/demo

  if (!secureClient.connect(AZURE_HOST, AZURE_PORT)) {
    Serial.println("❌ Azure connection failed");
    return;
  }

  secureClient.println("POST /devices/147esp32/messages/events?api-version=2021-04-12 HTTP/1.1");
  secureClient.println("Host: cs147Group9IotHub.azure-devices.net");
  secureClient.println("Authorization: " + String(AZURE_SAS));
  secureClient.println("Content-Type: application/json");
  secureClient.print("Content-Length: ");
  secureClient.println(payload.length());
  secureClient.println();
  secureClient.println(payload);

  Serial.print("📡 Azure log sent: ");
  Serial.println(payload);

  delay(100);
  secureClient.stop();
}

// =============================
// DISPENSE ACTION
// =============================
void dispenseOnce(const char* reason, float T, float H) {
  digitalWrite(LED_PIN, HIGH);
  tone(BUZZER_PIN, 2000, 150);

  smoothMove(
    segments[currentSegment].startA,
    segments[currentSegment].endA
  );

  currentSegment = (currentSegment + 1) % totalSegments;
  digitalWrite(LED_PIN, LOW);

  sendToAzure(T, H, reason);
}

// =============================
// SETUP
// =============================
void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // =============================
  // WIFI FIRST (CRITICAL)
  // =============================
  Serial.println("Starting WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(1000);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - startAttempt > 20000) break;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ WiFi FAILED");
  }

  // =============================
  // I2C AFTER WIFI
  // =============================
  Wire.begin(SDA_PIN, SCL_PIN);
  delay(100);

  // =============================
  // SENSORS
  // =============================
  if (!aht.begin()) Serial.println("❌ AHT20 not detected");
  else Serial.println("✅ AHT20 detected");

  if (!cap.begin(0x29)) Serial.println("❌ CAP1188 not detected");
  else Serial.println("✅ CAP1188 detected");

  // =============================
  // SERVO
  // =============================
  dispenserServo.attach(SERVO_PIN, 500, 2400);
  dispenserServo.write(0);
  delay(500);

  lastAutoDispense = millis();
}

// =============================
// LOOP
// =============================
void loop() {

  // ---- Read environment ----
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);

  float H = humidity.relative_humidity;
  float T = temp.temperature;

  Serial.printf("Temp: %.2f°C | Humidity: %.2f%%\n", T, H);
  sendToAzure(T, H, "env_reading");

  // ---- Humidity alert ----
  if (H > 70.0) {
    Serial.println("⚠ Humidity > 70%");
    for (int i = 0; i < 3; i++) {
      tone(BUZZER_PIN, 2000, 150);
      delay(200);
    }
  }

  // ---- Automatic dispense ----
  if (millis() - lastAutoDispense >= AUTO_INTERVAL_MS) {
    Serial.println("⏱ Auto dispense");
    lastAutoDispense = millis();
    dispenseOnce("auto_dispense", T, H);
  }

  // ---- Manual override (CAP1188) ----
  uint8_t currentTouch = cap.touched();
  uint8_t newTouch = currentTouch & ~previousTouch;
  previousTouch = currentTouch;

  if (newTouch) {
    unsigned long now = millis();

    if (now - lastTouchTime > SEQUENCE_TIMEOUT) {
      sequenceIndex = 0;
      Serial.println("⏳ Touch sequence reset");
    }

    for (int i = 0; i < 8; i++) {
      if (newTouch & (1 << i)) {
        Serial.printf("Pad %d touched\n", i + 1);

        if (i == requiredSequence[sequenceIndex]) {
          sequenceIndex++;
          lastTouchTime = now;

          if (sequenceIndex == 4) {
            Serial.println("✅ MANUAL DISPENSE");
            dispenseOnce("manual_dispense", T, H);
            sequenceIndex = 0;
          }
        } else {
          Serial.println("❌ Wrong pad");
          sequenceIndex = 0;
        }
      }
    }
  }

  delay(1000);
}
