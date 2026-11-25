// =============================================================
//  Barrier Gate System - ESP32-S3 (I2C slave of Raspberry Pi)
// =============================================================
//
//  ESP32 -> Pi  (Pi reads 32 bytes, text padded with '\0'):
//    "CAR_PASSED"            a car has passed the IR sensor
//    "TEMP:25.0,HUM:60.0"    DHT11 data (at start, then every hour)
//    "DHT_ERROR"             DHT11 read failed
//    "LIGHT:DAY"             it became day
//    "LIGHT:NIGHT"           it became night
//    "NONE"                  nothing new
//
//  Pi -> ESP32  (Pi writes 1 byte):
//    1  -> buzzer ON
//    0  -> buzzer OFF (default)
// =============================================================

#include <Arduino.h>
#include <Wire.h>
#include <DHT.h>

// ----- Pins -----
#define I2C_SDA      8
#define I2C_SCL      9
#define BUZZER_PIN   4
#define DHT_PIN      5
#define LDR_PIN      1
#define IR_PIN       6

// ----- Settings -----
#define I2C_ADDRESS      0x08
#define MSG_SIZE         32                     // bytes per message
#define DHT_INTERVAL     (60UL * 60UL * 1000UL) // 1 hour in ms
#define LIGHT_INTERVAL   1000                   // check light every 1 s
#define LIGHT_THRESHOLD  2000                   // 0..4095, adjust after testing
#define LIGHT_GAP        200                    // stops flicker near the threshold
#define DARK_IS_HIGH     true                   // if day/night is reversed, change to false

DHT dht(DHT_PIN, DHT11);
QueueHandle_t msgQueue;      // messages waiting for the Pi to read

bool carPresent = false;
bool isNight = false;
unsigned long lastDhtTime = 0;
unsigned long lastLightTime = 0;

// ---------- Put a message in the queue for the Pi ----------
void sendToPi(const char *text) {
  char msg[MSG_SIZE] = {0};
  strncpy(msg, text, MSG_SIZE - 1);

  if (xQueueSend(msgQueue, msg, 0) == pdTRUE) {
    Serial.print("To Pi: ");
    Serial.println(msg);
  } else {
    Serial.println("Queue full, message dropped");
  }
}

// ---------- I2C: Pi reads from ESP32 ----------
void onPiRequest() {
  char msg[MSG_SIZE] = {0};
  if (xQueueReceive(msgQueue, msg, 0) != pdTRUE) {
    strcpy(msg, "NONE");
  }
  Wire.write((uint8_t *)msg, MSG_SIZE);
}

// ---------- I2C: Pi writes to ESP32 ----------
void onPiSend(int count) {
  while (Wire.available()) {
    int cmd = Wire.read();
    if (cmd == 1 || cmd == '1') {
      digitalWrite(BUZZER_PIN, HIGH);
      Serial.println("Buzzer ON");
    } else if (cmd == 0 || cmd == '0') {
      digitalWrite(BUZZER_PIN, LOW);
      Serial.println("Buzzer OFF");
    }
  }
}

// ---------- IR sensor: detect car passing ----------
void checkCar() {
  bool blocked = (digitalRead(IR_PIN) == LOW);  // LOW = something in front

  if (blocked && !carPresent) {
    carPresent = true;                 // car arrived at the sensor
    Serial.println("Car detected");
  } else if (!blocked && carPresent) {
    carPresent = false;                // car has left the sensor = passed
    sendToPi("CAR_PASSED");
  }
}

// ---------- DHT11: send temperature and humidity ----------
void sendDht() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  if (isnan(temp) || isnan(hum)) {
    sendToPi("DHT_ERROR");
    return;
  }

  char msg[MSG_SIZE];
  snprintf(msg, MSG_SIZE, "TEMP:%.1f,HUM:%.1f", temp, hum);
  sendToPi(msg);
}

// ---------- Photoresistor: read day / night ----------
bool readIsNight(bool current) {
  int value = analogRead(LDR_PIN);
  if (!DARK_IS_HIGH) value = 4095 - value;     // make "dark" always = high

  if (value > LIGHT_THRESHOLD + LIGHT_GAP) return true;   // night
  if (value < LIGHT_THRESHOLD - LIGHT_GAP) return false;  // day
  return current;                                         // in between: no change
}

void checkLight() {
  bool nowNight = readIsNight(isNight);
  if (nowNight != isNight) {
    isNight = nowNight;
    sendToPi(isNight ? "LIGHT:NIGHT" : "LIGHT:DAY");
  }
}

// =============================================================
void setup() {
  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);     // buzzer off by default
  pinMode(IR_PIN, INPUT);
  analogReadResolution(12);          // 0..4095

  dht.begin();

  msgQueue = xQueueCreate(20, MSG_SIZE);  // can hold 20 messages

  Wire.onRequest(onPiRequest);
  Wire.onReceive(onPiSend);
  Wire.begin((uint8_t)I2C_ADDRESS, I2C_SDA, I2C_SCL, 100000);

  Serial.println("Barrier gate ready");

  // Send the first values at startup
  delay(2000);                       // DHT11 needs time after power on
  sendDht();
  isNight = readIsNight(false);
  sendToPi(isNight ? "LIGHT:NIGHT" : "LIGHT:DAY");

  lastDhtTime = millis();
  lastLightTime = millis();
}

void loop() {
  unsigned long now = millis();

  checkCar();

  if (now - lastLightTime >= LIGHT_INTERVAL) {
    lastLightTime = now;
    checkLight();
  }

  if (now - lastDhtTime >= DHT_INTERVAL) {
    lastDhtTime = now;
    sendDht();
  }

  delay(50);   // small delay also filters IR noise
}
