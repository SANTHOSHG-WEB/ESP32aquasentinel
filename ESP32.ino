/*
 * COMPLETE SMART WATER SYSTEM - MILLISECOND UPDATES
 * Sends data to Firebase every 100ms for REAL-TIME dashboard
 * 
 * TANK MOTOR (GPIO 25): ON when tank < 20%, OFF when tank ≥ 90%
 * GARDEN MOTOR (GPIO 26): ON when soil < 20% AND tank > 20% AND NOT raining
 */

#include <IRremote.hpp>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>

// ============== WiFi CONFIGURATION ==============
#define WIFI_SSID "OPPO A17"
#define WIFI_PASSWORD "leo@2007"

// ============== FIREBASE CONFIGURATION ==============
#define API_KEY "AIzaSyDxbDncNNIfYuAIY3tnJDYjXZtcmLuAaNA"
#define DATABASE_URL "https://aqua-fa925-default-rtdb.asia-southeast1.firebasedatabase.app"
#define USER_EMAIL "santhosh22333@gmail.com"
#define USER_PASSWORD "santhosh@123"

// ============== PIN DEFINITIONS ==============
#define TRIG_PIN 5
#define ECHO_PIN 18
#define SOIL_SENSOR_PIN 35
#define RAIN_SENSOR_PIN 34
#define TANK_MOTOR_PIN 25
#define GARDEN_MOTOR_PIN 26
#define BUZZER_PIN 27
#define IR_PIN 33
#define BUTTON_PIN 32
#define LED_GREEN 13
#define LED_YELLOW 14
#define LED_RED 15

// ============== RELAY LOGIC ==============
#define RELAY_ON LOW
#define RELAY_OFF HIGH

// ============== TANK CONFIG ==============
const float TANK_HEIGHT = 10.0;
const int TANK_MOTOR_ON = 20;
const int TANK_MOTOR_OFF = 90;

// ============== SOIL CONFIG ==============
const int SOIL_DRY_VALUE = 3100;
const int SOIL_WET_VALUE = 1800;
const int SOIL_MOTOR_ON = 20;
const int SOIL_MOTOR_OFF = 80;
const int GARDEN_RUN_TIME = 30;

// ============== RAIN SENSOR CONFIG ==============
const int RAIN_THRESHOLD = 2000;

// ============== FIREBASE OBJECTS ==============
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ============== VARIABLES ==============
float tankLevel = 0;
int soilMoisture = 0;
int rainValue = 0;
bool isRaining = false;
int rainIntensity = 0;

bool tankMotorStatus = false;
bool gardenMotorStatus = false;
unsigned long gardenMotorStartTime = 0;
bool autoMode = true;
bool tankManualOverride = false;
bool gardenManualOverride = false;

unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 300;
unsigned long lastPrintTime = 0;

// ============== FIREBASE TIMING ==============
unsigned long lastFirebaseUpdate = 0;
const unsigned long firebaseInterval = 100; // ⚡ UPDATE EVERY 100ms (0.1 seconds)

// For rate limiting (optional - prevents too many writes)
unsigned long lastTankSend = 0;
unsigned long lastSoilSend = 0;
unsigned long lastRainSend = 0;
float lastTankValue = -1;
int lastSoilValue = -1;
int lastRainValue = -1;
bool lastRainingState = false;

String deviceId = "device_001";

// ============== IR CODES ==============
#define IR_BUTTON_1 0x45
#define IR_BUTTON_2 0x46
#define IR_BUTTON_3 0x47
#define IR_BUTTON_4 0x44
#define IR_BUTTON_5 0x40
#define IR_BUTTON_6 0x43
#define IR_BUTTON_7 0x07
#define IR_BUTTON_8 0x15
#define IR_BUTTON_9 0x09
#define IR_BUTTON_0 0x19

// ============== SETUP ==============
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n==========================================");
  Serial.println("⚡ SMART WATER SYSTEM - MILLISECOND UPDATES");
  Serial.println("==========================================");
  Serial.println("Sending to Firebase every 100ms");
  Serial.println("==========================================\n");
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(SOIL_SENSOR_PIN, INPUT);
  pinMode(RAIN_SENSOR_PIN, INPUT);
  pinMode(TANK_MOTOR_PIN, OUTPUT);
  pinMode(GARDEN_MOTOR_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  
  digitalWrite(TANK_MOTOR_PIN, RELAY_OFF);
  digitalWrite(GARDEN_MOTOR_PIN, RELAY_OFF);
  
  IrReceiver.begin(IR_PIN);
  
  connectToWiFi();
  connectToFirebase();
  
  Serial.println("✅ System Ready!");
}

// ============== LOOP ==============
void loop() {
  // Read all sensors (fast readings)
  readTankLevel();
  readSoilMoisture();
  readRainSensor();
  
  // Check auto conditions
  checkAutoConditions();
  
  // Update LEDs
  updateLEDs();
  
  // Check alerts
  checkAlerts();
  
  // Auto shutoff garden motor
  if (gardenMotorStatus && (millis() - gardenMotorStartTime > GARDEN_RUN_TIME * 1000)) {
    Serial.println("⏱️ Auto shutoff: Garden motor ran 30 seconds");
    setGardenMotor(false);
    gardenManualOverride = false;
  }
  
  // Check IR remote
  checkIRRemote();
  
  // Check manual button
  checkManualButton();
  
  // ============== ⚡ FIREBASE UPDATES EVERY 100ms ==============
  if (millis() - lastFirebaseUpdate >= firebaseInterval) {
    sendToFirebase();
    listenForFirebaseCommands();
    lastFirebaseUpdate = millis();
  }
  
  // Print status every 5 seconds (not too often)
  if (millis() - lastPrintTime > 5000) {
    printStatus();
    lastPrintTime = millis();
  }
  
  delay(10); // Small delay for stability
}

// ============== READ RAIN SENSOR ==============
void readRainSensor() {
  rainValue = analogRead(RAIN_SENSOR_PIN);
  isRaining = (rainValue < RAIN_THRESHOLD);
  rainIntensity = map(rainValue, 0, 4095, 100, 0);
  rainIntensity = constrain(rainIntensity, 0, 100);
}

// ============== READ TANK LEVEL ==============
void readTankLevel() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return;
  
  float distance = (duration * 0.0343) / 2;
  
  if (distance <= 0.5) tankLevel = 100;
  else if (distance >= TANK_HEIGHT) tankLevel = 0;
  else tankLevel = ((TANK_HEIGHT - distance) / TANK_HEIGHT) * 100;
  
  tankLevel = constrain(tankLevel, 0, 100);
}

// ============== READ SOIL MOISTURE ==============
void readSoilMoisture() {
  int rawValue = analogRead(SOIL_SENSOR_PIN);
  soilMoisture = map(rawValue, SOIL_DRY_VALUE, SOIL_WET_VALUE, 0, 100);
  soilMoisture = constrain(soilMoisture, 0, 100);
}

// ============== CHECK AUTO CONDITIONS ==============
void checkAutoConditions() {
  if (!autoMode) return;
  
  // Tank motor control
  if (!tankManualOverride) {
    if (tankLevel < TANK_MOTOR_ON && !tankMotorStatus) {
      setTankMotor(true);
      tone(BUZZER_PIN, 1000, 200);
    }
    else if (tankLevel >= TANK_MOTOR_OFF && tankMotorStatus) {
      setTankMotor(false);
      tone(BUZZER_PIN, 800, 200);
    }
  }
  
  // Garden motor control
  if (!gardenManualOverride) {
    if (tankLevel < 20) {
      if (gardenMotorStatus) setGardenMotor(false);
      return;
    }
    
    if (isRaining) {
      if (gardenMotorStatus) setGardenMotor(false);
      return;
    }
    
    if (soilMoisture < SOIL_MOTOR_ON && !gardenMotorStatus && !isRaining) {
      setGardenMotor(true);
      tone(BUZZER_PIN, 1200, 200);
    }
    else if (soilMoisture >= SOIL_MOTOR_OFF && gardenMotorStatus) {
      setGardenMotor(false);
      tone(BUZZER_PIN, 600, 200);
    }
  }
}

// ============== MOTOR CONTROL ==============
void setTankMotor(bool on) {
  if (on) {
    digitalWrite(TANK_MOTOR_PIN, RELAY_ON);
    tankMotorStatus = true;
    Serial.println("💧 TANK MOTOR: ON");
  } else {
    digitalWrite(TANK_MOTOR_PIN, RELAY_OFF);
    tankMotorStatus = false;
    Serial.println("💧 TANK MOTOR: OFF");
  }
}

void setGardenMotor(bool on) {
  if (on) {
    digitalWrite(GARDEN_MOTOR_PIN, RELAY_ON);
    gardenMotorStatus = true;
    gardenMotorStartTime = millis();
    Serial.println("🌱 GARDEN MOTOR: ON");
  } else {
    digitalWrite(GARDEN_MOTOR_PIN, RELAY_OFF);
    gardenMotorStatus = false;
    Serial.println("🌱 GARDEN MOTOR: OFF");
  }
}

// ============== UPDATE LEDS ==============
void updateLEDs() {
  digitalWrite(LED_GREEN, (tankLevel > 60) ? HIGH : LOW);
  digitalWrite(LED_YELLOW, gardenMotorStatus ? HIGH : LOW);
  digitalWrite(LED_RED, (!autoMode || tankLevel < 15 || isRaining) ? HIGH : LOW);
}

// ============== CHECK ALERTS ==============
void checkAlerts() {
  static unsigned long lastAlertTime = 0;
  
  if (tankLevel < 10 && millis() - lastAlertTime > 30000) {
    for(int i=0; i<3; i++) {
      tone(BUZZER_PIN, 2000, 100);
      delay(200);
    }
    lastAlertTime = millis();
  }
}

// ============== WIFI CONNECTION ==============
void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    Serial.print(".");
    delay(500);
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi Connected!");
  } else {
    Serial.println("\n❌ WiFi Failed");
  }
}

// ============== FIREBASE CONNECTION ==============
void connectToFirebase() {
  Serial.print("Connecting to Firebase");
  
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.token_status_callback = tokenStatusCallback;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
  int attempts = 0;
  while (!Firebase.ready() && attempts < 20) {
    Serial.print(".");
    delay(500);
    attempts++;
  }
  
  if (Firebase.ready()) {
    Serial.println("\n✅ Firebase Connected!");
  } else {
    Serial.println("\n❌ Firebase Failed");
  }
}

// ============== ⚡ SEND TO FIREBASE (EVERY 100ms) ==============
void sendToFirebase() {
  if (!Firebase.ready() || WiFi.status() != WL_CONNECTED) return;
  
  unsigned long now = millis();
  bool dataChanged = false;
  
  // ===== TANK LEVEL - Update only if changed (prevents spam) =====
  if (abs(tankLevel - lastTankValue) > 0.5 || now - lastTankSend > 1000) {
    if (Firebase.RTDB.setFloat(&fbdo, "/devices/" + deviceId + "/data/sensors/tankLevel", tankLevel)) {
      lastTankValue = tankLevel;
      lastTankSend = now;
      dataChanged = true;
    }
  }
  
  // ===== SOIL MOISTURE - Update only if changed =====
  if (abs(soilMoisture - lastSoilValue) > 1 || now - lastSoilSend > 1000) {
    if (Firebase.RTDB.setInt(&fbdo, "/devices/" + deviceId + "/data/sensors/soilMoisture", soilMoisture)) {
      lastSoilValue = soilMoisture;
      lastSoilSend = now;
      dataChanged = true;
    }
  }
  
  // ===== RAIN SENSOR - Update only if changed =====
  if (abs(rainValue - lastRainValue) > 10 || now - lastRainSend > 1000) {
    if (Firebase.RTDB.setInt(&fbdo, "/devices/" + deviceId + "/data/sensors/rainValue", rainValue)) {
      lastRainValue = rainValue;
      lastRainSend = now;
      dataChanged = true;
    }
    
    // Rain intensity
    Firebase.RTDB.setInt(&fbdo, "/devices/" + deviceId + "/data/sensors/rainIntensity", rainIntensity);
    
    // Rain state
    if (isRaining != lastRainingState) {
      Firebase.RTDB.setBool(&fbdo, "/devices/" + deviceId + "/data/sensors/isRaining", isRaining);
      lastRainingState = isRaining;
    }
  }
  
  // ===== MOTOR STATUS - Update when changed =====
  static bool lastTankMotor = false;
  static bool lastGardenMotor = false;
  
  if (tankMotorStatus != lastTankMotor) {
    Firebase.RTDB.setBool(&fbdo, "/devices/" + deviceId + "/data/motors/tankMotor", tankMotorStatus);
    lastTankMotor = tankMotorStatus;
    dataChanged = true;
  }
  
  if (gardenMotorStatus != lastGardenMotor) {
    Firebase.RTDB.setBool(&fbdo, "/devices/" + deviceId + "/data/motors/gardenMotor", gardenMotorStatus);
    lastGardenMotor = gardenMotorStatus;
    dataChanged = true;
  }
  
  // ===== AUTO MODE - Update when changed =====
  static bool lastAutoMode = true;
  if (autoMode != lastAutoMode) {
    Firebase.RTDB.setBool(&fbdo, "/devices/" + deviceId + "/data/motors/autoMode", autoMode);
    lastAutoMode = autoMode;
    dataChanged = true;
  }
  
  // ===== TIMESTAMP - Update every 5 seconds =====
  static unsigned long lastTimestampSend = 0;
  if (now - lastTimestampSend > 5000) {
    Firebase.RTDB.setInt(&fbdo, "/devices/" + deviceId + "/data/lastUpdate", now);
    lastTimestampSend = now;
  }
  
  // ===== DEVICE STATUS - Update every 10 seconds =====
  static unsigned long lastStatusSend = 0;
  if (now - lastStatusSend > 10000) {
    FirebaseJson info;
    info.set("status", "online");
    info.set("lastSeen", now);
    Firebase.RTDB.setJSON(&fbdo, "/devices/" + deviceId + "/info", &info);
    lastStatusSend = now;
  }
  
  // Optional: Print when data changes (for debugging)
  if (dataChanged && (millis() % 1000 < 100)) {
    Serial.println("⚡ Firebase updated");
  }
}

// ============== LISTEN FOR COMMANDS ==============
void listenForFirebaseCommands() {
  if (!Firebase.ready()) return;
  
  // Check commands every 500ms (not too frequent)
  static unsigned long lastCommandCheck = 0;
  if (millis() - lastCommandCheck < 500) return;
  lastCommandCheck = millis();
  
  // Tank motor command
  if (Firebase.RTDB.getString(&fbdo, "/devices/" + deviceId + "/commands/tankMotor")) {
    String cmd = fbdo.stringData();
    if (cmd == "ON") {
      setTankMotor(true);
      tankManualOverride = true;
      Firebase.RTDB.setString(&fbdo, "/devices/" + deviceId + "/commands/tankMotor", "DONE");
    } else if (cmd == "OFF") {
      setTankMotor(false);
      tankManualOverride = true;
      Firebase.RTDB.setString(&fbdo, "/devices/" + deviceId + "/commands/tankMotor", "DONE");
    }
  }
  
  // Garden motor command
  if (Firebase.RTDB.getString(&fbdo, "/devices/" + deviceId + "/commands/gardenMotor")) {
    String cmd = fbdo.stringData();
    if (cmd == "ON") {
      setGardenMotor(true);
      gardenManualOverride = true;
      Firebase.RTDB.setString(&fbdo, "/devices/" + deviceId + "/commands/gardenMotor", "DONE");
    } else if (cmd == "OFF") {
      setGardenMotor(false);
      gardenManualOverride = true;
      Firebase.RTDB.setString(&fbdo, "/devices/" + deviceId + "/commands/gardenMotor", "DONE");
    }
  }
  
  // Mode command
  if (Firebase.RTDB.getString(&fbdo, "/devices/" + deviceId + "/commands/mode")) {
    String cmd = fbdo.stringData();
    if (cmd == "AUTO") {
      autoMode = true;
      tankManualOverride = false;
      gardenManualOverride = false;
      Firebase.RTDB.setString(&fbdo, "/devices/" + deviceId + "/commands/mode", "DONE");
    } else if (cmd == "MANUAL") {
      autoMode = false;
      Firebase.RTDB.setString(&fbdo, "/devices/" + deviceId + "/commands/mode", "DONE");
    }
  }
}

// ============== IR REMOTE ==============
void checkIRRemote() {
  if (IrReceiver.decode()) {
    unsigned long command = IrReceiver.decodedIRData.command;
    
    switch(command) {
      case IR_BUTTON_1: setTankMotor(!tankMotorStatus); tankManualOverride = true; break;
      case IR_BUTTON_2: setGardenMotor(!gardenMotorStatus); gardenManualOverride = true; break;
      case IR_BUTTON_3: setTankMotor(true); tankManualOverride = true; break;
      case IR_BUTTON_4: setTankMotor(false); tankManualOverride = true; break;
      case IR_BUTTON_5: setGardenMotor(true); gardenManualOverride = true; break;
      case IR_BUTTON_6: setGardenMotor(false); gardenManualOverride = true; break;
      
      case IR_BUTTON_7:
        autoMode = !autoMode;
        if (autoMode) {
          tankManualOverride = false;
          gardenManualOverride = false;
        }
        break;
        
      case IR_BUTTON_8: printStatus(); break;
      
      case IR_BUTTON_0:
        setTankMotor(false);
        setGardenMotor(false);
        tankManualOverride = true;
        gardenManualOverride = true;
        autoMode = false;
        break;
    }
    IrReceiver.resume();
  }
}

// ============== MANUAL BUTTON ==============
void checkManualButton() {
  static bool lastButtonState = HIGH;
  bool buttonState = digitalRead(BUTTON_PIN);
  
  if (buttonState == LOW && lastButtonState == HIGH && 
      (millis() - lastButtonPress) > debounceDelay) {
    
    lastButtonPress = millis();
    delay(50);
    
    if (digitalRead(BUTTON_PIN) == LOW) {
      unsigned long pressStart = millis();
      while (digitalRead(BUTTON_PIN) == LOW && millis() - pressStart < 1000) {
        delay(10);
      }
      
      if (millis() - pressStart >= 1000) {
        setTankMotor(false);
        setGardenMotor(false);
        tankManualOverride = true;
        gardenManualOverride = true;
        autoMode = false;
      } else {
        autoMode = !autoMode;
        if (autoMode) {
          tankManualOverride = false;
          gardenManualOverride = false;
        }
      }
    }
  }
  lastButtonState = buttonState;
}

// ============== PRINT STATUS ==============
void printStatus() {
  Serial.println("\n📊 SYSTEM STATUS");
  Serial.println("========================");
  Serial.print("Tank: "); Serial.print(tankLevel); Serial.println("%");
  Serial.print("Soil: "); Serial.print(soilMoisture); Serial.println("%");
  Serial.print("Rain: "); Serial.println(isRaining ? "🌧️ YES" : "☀️ NO");
  Serial.print("Tank Motor: "); Serial.println(tankMotorStatus ? "ON" : "OFF");
  Serial.print("Garden Motor: "); Serial.println(gardenMotorStatus ? "ON" : "OFF");
  Serial.print("Mode: "); Serial.println(autoMode ? "AUTO" : "MANUAL");
  Serial.println("========================");
}

// ============== TONE FUNCTION ==============
void tone(int pin, int freq, int dur) {
  for (int i = 0; i < dur * freq / 1000; i++) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(1000000 / (freq * 2));
    digitalWrite(pin, LOW);
    delayMicroseconds(1000000 / (freq * 2));
  }
}