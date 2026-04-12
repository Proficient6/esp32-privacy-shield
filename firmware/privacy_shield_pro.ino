/*
 * ============================================
 * ESP32 Privacy Shield Pro — Firmware v2.0
 * ============================================
 * 
 * Features:
 *  - Ultrasonic distance-based auto-lock (Win+L via BLE Keyboard)
 *  - Hand-wave gesture quick-lock
 *  - WiFi web dashboard for real-time configuration
 *  - Persistent settings stored in NVS (Non-Volatile Storage)
 *  - WiFiManager captive portal for first-time WiFi setup
 * 
 * Required Libraries (install via Arduino Library Manager):
 *  1. ESP32-BLE-Keyboard     by T-vK          (https://github.com/T-vK/ESP32-BLE-Keyboard)
 *  2. ESPAsyncWebServer      by me-no-dev     (https://github.com/me-no-dev/ESPAsyncWebServer)
 *  3. AsyncTCP               by me-no-dev     (https://github.com/me-no-dev/AsyncTCP)
 *  4. ArduinoJson            by Benoit Blanchon (Library Manager: "ArduinoJson")
 *  5. WiFiManager            by tzapu         (Library Manager: "WiFiManager" by tzapu)
 *  6. LittleFS               (built-in with ESP32 core >= 2.0)
 * 
 * Upload Instructions:
 *  1. Upload sketch via Arduino IDE (Tools > Board > ESP32 Dev Module)
 *  2. Upload LittleFS data: Tools > ESP32 Sketch Data Upload
 *     (Requires "ESP32 LittleFS Data Upload" plugin)
 *  3. On first boot, connect to WiFi AP "PrivacyShield-Setup" and configure WiFi
 *  4. Open the ESP32's IP address in any browser to access the dashboard
 */

#include <WiFi.h>
#include <WiFiManager.h>        // WiFiManager by tzapu
#include <ESPAsyncWebServer.h>  // ESPAsyncWebServer by me-no-dev
#include <AsyncTCP.h>           // AsyncTCP by me-no-dev
#include <LittleFS.h>           // Built-in
#include <Preferences.h>        // Built-in (NVS)
#include <ArduinoJson.h>        // ArduinoJson by Benoit Blanchon
#include <BleKeyboard.h>        // ESP32-BLE-Keyboard by T-vK
#include <algorithm>            // std::sort

// =============================================
// PIN DEFINITIONS
// =============================================
const int TRIG_PIN  = 5;
const int ECHO_PIN  = 18;
const int RED_LED   = 19;
const int GREEN_LED = 21;
const int BUZZER    = 22;

// =============================================
// CONFIGURABLE SETTINGS (loaded from NVS)
// =============================================
int    DIST_THRESHOLD_LOCK  = 50;    // cm — distance to trigger auto-lock
int    DIST_THRESHOLD_AWAKE = 30;    // cm — distance to detect user return
unsigned long LOCK_DELAY    = 3000;  // ms — delay before locking

int    GESTURE_NEAR    = 5;          // cm — hand "in" threshold
int    GESTURE_FAR     = 10;         // cm — hand "out" threshold
int    WAVES_REQUIRED  = 2;          // count — waves to trigger quick-lock

// =============================================
// CONSTANTS (not configurable)
// =============================================
const int SAMPLES = 5;
const unsigned long GESTURE_TIMEOUT = 3000;  // ms

// =============================================
// STATE MACHINE
// =============================================
enum SystemState { USER_PRESENT, USER_AWAY, LOCKING };
SystemState currentState = USER_PRESENT;

// =============================================
// VARIABLES
// =============================================
int  waveCount = 0;
bool handWasNear = false;
unsigned long lastWaveTime = 0;
unsigned long awayStartTime = 0;
float lastDistance = 0.0;

// =============================================
// OBJECTS
// =============================================
BleKeyboard   bleKeyboard("Privacy Shield Pro", "ESP32", 100);
AsyncWebServer server(80);
Preferences   preferences;

// =============================================
// NVS: LOAD SETTINGS
// =============================================
void loadSettings() {
  preferences.begin("shield", true); // read-only
  DIST_THRESHOLD_LOCK  = preferences.getInt("distLock",  50);
  DIST_THRESHOLD_AWAKE = preferences.getInt("distAwake", 30);
  LOCK_DELAY           = preferences.getULong("lockDelay", 3000);
  GESTURE_NEAR         = preferences.getInt("gestNear",  5);
  GESTURE_FAR          = preferences.getInt("gestFar",   10);
  WAVES_REQUIRED       = preferences.getInt("wavesReq",  2);
  preferences.end();

  Serial.println("[NVS] Settings loaded:");
  Serial.printf("  Lock Distance:  %d cm\n", DIST_THRESHOLD_LOCK);
  Serial.printf("  Wake Distance:  %d cm\n", DIST_THRESHOLD_AWAKE);
  Serial.printf("  Lock Delay:     %lu ms\n", LOCK_DELAY);
  Serial.printf("  Gesture Near:   %d cm\n", GESTURE_NEAR);
  Serial.printf("  Gesture Far:    %d cm\n", GESTURE_FAR);
  Serial.printf("  Waves Required: %d\n", WAVES_REQUIRED);
}

// =============================================
// NVS: SAVE SETTINGS
// =============================================
void saveSettings() {
  preferences.begin("shield", false); // read-write
  preferences.putInt("distLock",   DIST_THRESHOLD_LOCK);
  preferences.putInt("distAwake",  DIST_THRESHOLD_AWAKE);
  preferences.putULong("lockDelay", LOCK_DELAY);
  preferences.putInt("gestNear",   GESTURE_NEAR);
  preferences.putInt("gestFar",    GESTURE_FAR);
  preferences.putInt("wavesReq",   WAVES_REQUIRED);
  preferences.end();
  Serial.println("[NVS] Settings saved.");
}

// =============================================
// SENSOR: FILTERED DISTANCE
// =============================================
float getFilteredDistance() {
  float readings[SAMPLES];
  for (int i = 0; i < SAMPLES; i++) {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    long duration = pulseIn(ECHO_PIN, HIGH, 25000);
    if (duration == 0) readings[i] = 999;
    else readings[i] = (duration * 0.0343) / 2;
    delay(10);
  }
  std::sort(readings, readings + SAMPLES);
  return readings[SAMPLES / 2]; // median
}

// =============================================
// WEB SERVER: SETUP API ROUTES
// =============================================
void setupWebServer() {
  // Serve static files from LittleFS
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  // GET /api/settings — return current settings as JSON
  server.on("/api/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["distThresholdLock"]  = DIST_THRESHOLD_LOCK;
    doc["distThresholdAwake"] = DIST_THRESHOLD_AWAKE;
    doc["lockDelay"]          = LOCK_DELAY;
    doc["gestureNear"]        = GESTURE_NEAR;
    doc["gestureFar"]         = GESTURE_FAR;
    doc["wavesRequired"]      = WAVES_REQUIRED;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // POST /api/settings — update settings from JSON body
  server.on("/api/settings", HTTP_POST, 
    [](AsyncWebServerRequest *request) {},    // onRequest (unused)
    NULL,                                      // onUpload
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      // Accumulate body
      String body;
      for (size_t i = 0; i < len; i++) {
        body += (char)data[i];
      }

      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, body);
      if (error) {
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
      }

      // Update settings from JSON
      if (doc.containsKey("distThresholdLock"))  DIST_THRESHOLD_LOCK  = doc["distThresholdLock"].as<int>();
      if (doc.containsKey("distThresholdAwake")) DIST_THRESHOLD_AWAKE = doc["distThresholdAwake"].as<int>();
      if (doc.containsKey("lockDelay"))          LOCK_DELAY           = doc["lockDelay"].as<unsigned long>();
      if (doc.containsKey("gestureNear"))        GESTURE_NEAR         = doc["gestureNear"].as<int>();
      if (doc.containsKey("gestureFar"))         GESTURE_FAR          = doc["gestureFar"].as<int>();
      if (doc.containsKey("wavesRequired"))      WAVES_REQUIRED       = doc["wavesRequired"].as<int>();

      // Save to NVS
      saveSettings();

      request->send(200, "application/json", "{\"status\":\"ok\"}");
      Serial.println("[API] Settings updated via web dashboard.");
    }
  );

  // GET /api/status — return live sensor data
  server.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["distance"]     = lastDistance;
    doc["state"]        = (int)currentState;
    doc["bleConnected"] = bleKeyboard.isConnected();

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });

  // POST /api/reset — reset all settings to defaults
  server.on("/api/reset", HTTP_POST, [](AsyncWebServerRequest *request) {
    DIST_THRESHOLD_LOCK  = 50;
    DIST_THRESHOLD_AWAKE = 30;
    LOCK_DELAY           = 3000;
    GESTURE_NEAR         = 5;
    GESTURE_FAR          = 10;
    WAVES_REQUIRED       = 2;
    saveSettings();
    request->send(200, "application/json", "{\"status\":\"reset\"}");
    Serial.println("[API] Settings reset to defaults.");
  });

  // Handle CORS preflight
  server.on("/api/*", HTTP_OPTIONS, [](AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(204);
    response->addHeader("Access-Control-Allow-Origin", "*");
    response->addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    response->addHeader("Access-Control-Allow-Headers", "Content-Type");
    request->send(response);
  });

  // Add CORS headers to all responses
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");

  server.begin();
  Serial.println("[WEB] Server started on port 80");
}

// =============================================
// SETUP
// =============================================
void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("========================================");
  Serial.println("   ESP32 Privacy Shield Pro v2.0");
  Serial.println("========================================");
  
  // GPIO setup
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  // Load saved settings from NVS
  loadSettings();

  // Initialize LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("[FS] ERROR: LittleFS mount failed!");
  } else {
    Serial.println("[FS] LittleFS mounted successfully.");
  }

  // WiFiManager — captive portal for WiFi configuration
  WiFiManager wm;
  wm.setConfigPortalTimeout(180);  // 3 minute timeout
  wm.setAPCallback([](WiFiManager *mgr) {
    Serial.println("[WiFi] Config portal started.");
    Serial.println("[WiFi] Connect to AP: PrivacyShield-Setup");
    // Flash both LEDs to indicate setup mode
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, HIGH);
  });

  // Auto-connect or start config portal
  bool wifiConnected = wm.autoConnect("PrivacyShield-Setup", "shield123");
  
  if (wifiConnected) {
    Serial.println("[WiFi] Connected!");
    Serial.print("[WiFi] IP Address: ");
    Serial.println(WiFi.localIP());
    
    // Setup web server
    setupWebServer();
    
    // Success tone
    tone(BUZZER, 1500, 100);
    delay(150);
    tone(BUZZER, 2000, 100);
  } else {
    Serial.println("[WiFi] Failed to connect. Continuing without web server.");
    tone(BUZZER, 500, 500);
  }

  // Start BLE Keyboard
  bleKeyboard.begin();
  Serial.println("[BLE] Keyboard started. Waiting for connection...");
  
  // Startup complete tone
  tone(BUZZER, 1000, 200);
  
  Serial.println("========================================");
  Serial.println("   System ready. Open browser to:");
  Serial.print("   http://");
  Serial.println(WiFi.localIP());
  Serial.println("========================================");
}

// =============================================
// MAIN LOOP
// =============================================
void loop() {
  // BLE not connected — blink green LED
  if (!bleKeyboard.isConnected()) {
    digitalWrite(GREEN_LED, (millis() / 500) % 2);
    digitalWrite(RED_LED, LOW);
    
    // Still read distance for web dashboard
    lastDistance = getFilteredDistance();
    delay(100);
    return;
  }

  float dist = getFilteredDistance();
  lastDistance = dist;  // Store for web API
  unsigned long currentTime = millis();

  switch (currentState) {
    case USER_PRESENT:
      digitalWrite(GREEN_LED, HIGH);
      digitalWrite(RED_LED, LOW);

      // --- Gesture Detection ---
      if (dist < GESTURE_NEAR && !handWasNear) {
        handWasNear = true;
        waveCount++;
        lastWaveTime = currentTime;
        tone(BUZZER, 2000, 100);
        Serial.print("[Gesture] Wave detected! Count: ");
        Serial.println(waveCount);
      } 
      else if (dist > GESTURE_FAR && handWasNear) {
        handWasNear = false;
      }

      // Reset wave count if user is too slow
      if (waveCount > 0 && (currentTime - lastWaveTime > GESTURE_TIMEOUT)) {
        waveCount = 0;
        Serial.println("[Gesture] Timeout. Resetting.");
      }

      // If waves completed, trigger LOCKING
      if (waveCount >= WAVES_REQUIRED) {
        waveCount = 0;
        currentState = LOCKING;
        Serial.println("[State] -> LOCKING (gesture)");
      }
      
      // Absence Detection
      if (dist > DIST_THRESHOLD_LOCK) {
        awayStartTime = currentTime;
        currentState = USER_AWAY;
        tone(BUZZER, 1500, 100);
        Serial.println("[State] -> USER_AWAY");
      }
      break;

    case USER_AWAY:
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(RED_LED, (millis() / 200) % 2);

      if (dist < DIST_THRESHOLD_AWAKE) {
        currentState = USER_PRESENT;
        waveCount = 0;
        noTone(BUZZER);
        Serial.println("[State] -> USER_PRESENT (returned)");
      } 
      else if (millis() - awayStartTime > LOCK_DELAY) {
        currentState = LOCKING;
        Serial.println("[State] -> LOCKING (timeout)");
      }
      break;

    case LOCKING:
      digitalWrite(RED_LED, HIGH);
      tone(BUZZER, 2000, 500);
      
      Serial.println("[LOCK] Sending Win+L");
      bleKeyboard.press(KEY_LEFT_GUI);
      bleKeyboard.press('l');
      delay(100);
      bleKeyboard.releaseAll();
      
      delay(5000);
      currentState = USER_AWAY;
      Serial.println("[State] -> USER_AWAY (post-lock)");
      break;
  }
}
