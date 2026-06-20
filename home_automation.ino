/*
  Full Final: Voice + Gesture Controlled Home Automation (ESP32)
  - BluetoothSerial (strict text commands)
  - MPU6050 gesture: left/right select, up = ON, down = OFF
  - 4 relays (ACTIVE-LOW modules) -> corrected logic
  - Timers per device (minutes)
  - OLED SSD1306 (0x3C) UI - Option B (Icons + Clean Text, non-overlapping)
*/

#include "BluetoothSerial.h"
#include <Wire.h>
#include "MPU6050.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

MPU6050 mpu;

// I2C Pins
#define SDA_PIN 21
#define SCL_PIN 22

// Relay pins
#define RELAY1_PIN 25  // Light
#define RELAY2_PIN 26  // Fan
#define RELAY3_PIN 27  // TV
#define RELAY4_PIN 14  // Switch

// Relay logic: set this to true if your relay module is ACTIVE LOW (most common)
// ACTIVE_LOW: LOW -> relay ON, HIGH -> relay OFF
#define RELAY_ACTIVE_LOW true

// Convenience macros for relay on/off
#if RELAY_ACTIVE_LOW
  #define RELAY_ON(pin)  digitalWrite(pin, LOW)
  #define RELAY_OFF(pin) digitalWrite(pin, HIGH)
#else
  #define RELAY_ON(pin)  digitalWrite(pin, HIGH)
  #define RELAY_OFF(pin) digitalWrite(pin, LOW)
#endif

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

BluetoothSerial SerialBT;

// Gesture / selection
int selectedAppliance = 1;  // 1..4
float THRESHOLD = 1.2f;     // adjust 0.8 - 2.0 as needed
unsigned long lastGestureTime = 0;
unsigned long debounceTime = 500; // ms

// States
bool light_state = false;
bool fan_state = false;
bool tv_state = false;
bool switch_state = false;

// Timers (durations & end times). timer > 0 indicates active.
unsigned long light_timer = 0;    // milliseconds duration left tracker (not needed but kept)
unsigned long fan_timer = 0;
unsigned long tv_timer = 0;
unsigned long switch_timer = 0;

unsigned long light_end_time = 0; // millis() timestamp when timer should expire
unsigned long fan_end_time = 0;
unsigned long tv_end_time = 0;
unsigned long switch_end_time = 0;

// Bluetooth input buffer
String voiceCommand = "";
unsigned long lastRxMs = 0;
const unsigned long idleCutMs = 150;

// ---- helpers ----
int extractNumber(const String& cmd) {
  String num = "";
  for (int i = 0; i < (int)cmd.length(); i++) {
    if (isDigit(cmd.charAt(i))) num += cmd.charAt(i);
  }
  return (num.length() > 0) ? num.toInt() : 0;
}

void toStrictLowerTrim(String &s) {
  s.toLowerCase();
  s.trim();
}

// collapse repeated spaces to single space
void normalizeCommand(String &s) {
  String out = "";
  bool prevSpace = false;
  for (int i = 0; i < (int)s.length(); i++) {
    char c = s.charAt(i);
    if (c == ' ') {
      if (!prevSpace) out += c;
      prevSpace = true;
    } else {
      out += c;
      prevSpace = false;
    }
  }
  s = out;
}

// ---- device low-level control (keeps states in sync) ----
void setRelayStateRaw(int appliance, bool on) {
  int pin = RELAY1_PIN;
  switch (appliance) {
    case 1: pin = RELAY1_PIN; break;
    case 2: pin = RELAY2_PIN; break;
    case 3: pin = RELAY3_PIN; break;
    case 4: pin = RELAY4_PIN; break;
  }
  if (on) RELAY_ON(pin); else RELAY_OFF(pin);
}

void setApplianceState(int appliance, bool on) {
  setRelayStateRaw(appliance, on);
  switch (appliance) {
    case 1: light_state = on; break;
    case 2: fan_state = on; break;
    case 3: tv_state = on; break;
    case 4: switch_state = on; break;
  }
}

// ---- device actions (use above helpers) ----
void turnOnLight()  { setApplianceState(1, true); Serial.println("? Light ON"); SerialBT.println("Light ON"); }
void turnOffLight() { setApplianceState(1, false); light_timer = 0; light_end_time = 0; Serial.println("? Light OFF"); SerialBT.println("Light OFF"); }
void setLightTimer(int min) { if(min>0){ light_timer = (unsigned long)min * 60000UL; light_end_time = millis() + light_timer; Serial.print("? Light timer: "); Serial.print(min); Serial.println(" min"); SerialBT.print("Light "); SerialBT.print(min); SerialBT.println("min timer"); } }

void turnOnFan()  { setApplianceState(2, true); Serial.println("? Fan ON"); SerialBT.println("Fan ON"); }
void turnOffFan() { setApplianceState(2, false); fan_timer = 0; fan_end_time = 0; Serial.println("? Fan OFF"); SerialBT.println("Fan OFF"); }
void setFanTimer(int min) { if(min>0){ fan_timer = (unsigned long)min * 60000UL; fan_end_time = millis() + fan_timer; Serial.print("? Fan timer: "); Serial.print(min); Serial.println(" min"); SerialBT.print("Fan "); SerialBT.print(min); SerialBT.println("min timer"); } }

void turnOnTv()  { setApplianceState(3, true); Serial.println("? TV ON"); SerialBT.println("TV ON"); }
void turnOffTv() { setApplianceState(3, false); tv_timer = 0; tv_end_time = 0; Serial.println("? TV OFF"); SerialBT.println("TV OFF"); }
void setTvTimer(int min) { if(min>0){ tv_timer = (unsigned long)min * 60000UL; tv_end_time = millis() + tv_timer; Serial.print("? TV timer: "); Serial.print(min); Serial.println(" min"); SerialBT.print("TV "); SerialBT.print(min); SerialBT.println("min timer"); } }

void turnOnSwitch()  { setApplianceState(4, true); Serial.println("? Switch ON"); SerialBT.println("Switch ON"); }
void turnOffSwitch() { setApplianceState(4, false); switch_timer = 0; switch_end_time = 0; Serial.println("? Switch OFF"); SerialBT.println("Switch OFF"); }
void setSwitchTimer(int min) { if(min>0){ switch_timer = (unsigned long)min * 60000UL; switch_end_time = millis() + switch_timer; Serial.print("? Switch timer: "); Serial.print(min); Serial.println(" min"); SerialBT.print("Switch "); SerialBT.print(min); SerialBT.println("min timer"); } }

void turnOnAll()  { turnOnLight(); turnOnFan(); turnOnTv(); turnOnSwitch(); Serial.println("??? ALL ON"); SerialBT.println("ALL ON"); }
void turnOffAll() { turnOffLight(); turnOffFan(); turnOffTv(); turnOffSwitch(); cancelAllTimers(); Serial.println("??? ALL OFF"); SerialBT.println("ALL OFF"); }
void setAllTimer(int min) { setLightTimer(min); setFanTimer(min); setTvTimer(min); setSwitchTimer(min); }
void cancelAllTimers() { light_timer = fan_timer = tv_timer = switch_timer = 0; light_end_time = fan_end_time = tv_end_time = switch_end_time = 0; }

// Print to serial status
void printStatus() {
  Serial.println("\n??? STATUS ???");
  Serial.print("Light:    "); Serial.println(light_state ? "ON ?" : "OFF ?");
  Serial.print("Fan:      ");   Serial.println(fan_state ? "ON ?" : "OFF ?");
  Serial.print("TV:       ");    Serial.println(tv_state ? "ON ?" : "OFF ?");
  Serial.print("Switch:   "); Serial.println(switch_state ? "ON ?" : "OFF ?");
  Serial.println("??????????????\n");
}

// ---------------- RELAY CONTROL (for gestures) ----------------
void controlRelay(int appliance, bool turnOn) {
  setApplianceState(appliance, turnOn);
}

// ---- command processing ----
void processSingleCommand(char cmd) {
  Serial.print("\n>>> Command: ");
  Serial.println(cmd);

  switch (cmd) {
    case '1': turnOnLight(); break;
    case 'A':
    case 'a': turnOffLight(); break;

    case '2': turnOnFan(); break;
    case 'B':
    case 'b': turnOffFan(); break;

    case '3': turnOnTv(); break;
    case 'C':
    case 'c': turnOffTv(); break;

    case '4': turnOnSwitch(); break;
    case 'D':
    case 'd': turnOffSwitch(); break;

    case '5': turnOnAll(); break;
    case '0': turnOffAll(); break;

    default:
      Serial.println("Unknown command");
      SerialBT.println("Unknown");
  }
  printStatus();
}

// Helper to parse and handle strict text commands (like "fan on 5")
void processTextCommand(String cmd) {
  toStrictLowerTrim(cmd);
  normalizeCommand(cmd);

  if (cmd.length() == 0) return;

  Serial.print("[Process] ");
  Serial.println(cmd);

  int minutes = extractNumber(cmd);

  if (cmd == "light on" || cmd.startsWith("light on ")) {
    turnOnLight();
    if (minutes > 0) setLightTimer(minutes);
  }
  else if (cmd == "light off" || cmd.startsWith("light off ")) {
    turnOffLight();
  }
  else if (cmd == "fan on" || cmd.startsWith("fan on ")) {
    turnOnFan();
    if (minutes > 0) setFanTimer(minutes);
  }
  else if (cmd == "fan of" || cmd.startsWith("fan off ")) {
    turnOffFan();
  }
  else if (cmd == "tv on" || cmd.startsWith("tv on ")) {
    turnOnTv();
    if (minutes > 0) setTvTimer(minutes);
  }
  else if (cmd == "tv off" || cmd.startsWith("tv off ")) {
    turnOffTv();
  }
  else if (cmd == "switch on" || cmd.startsWith("switch on ")) {
    turnOnSwitch();
    if (minutes > 0) setSwitchTimer(minutes);
  }
  else if (cmd == "switch off" || cmd.startsWith("switch off ")) {
    turnOffSwitch();
  }
  else if (cmd == "all on" || cmd.startsWith("all on ")) {
    turnOnAll();
    if (minutes > 0) setAllTimer(minutes);
  }
  else if (cmd == "all off" || cmd.startsWith("all off ")) {
    turnOffAll();
  }
  else if (cmd == "status") {
    printStatus();
    // no side-effects
  }
  else {
    Serial.println("Command not recognized (strict mode)");
    SerialBT.println("Not recognized (strict)");
    return;
  }

  printStatus();
}

// ---- timers ----
void checkTimers() {
  unsigned long now = millis();

  if (light_state && light_end_time != 0 && (long)(now - light_end_time) >= 0) {
    turnOffLight();
    Serial.println("? Light timer expired");
    SerialBT.println("Light OFF (timer)");
    light_end_time = 0;
    light_timer = 0;
    updateOLED();
  }
  if (fan_state && fan_end_time != 0 && (long)(now - fan_end_time) >= 0) {
    turnOffFan();
    Serial.println("? Fan timer expired");
    SerialBT.println("Fan OFF (timer)");
    fan_end_time = 0;
    fan_timer = 0;
    updateOLED();
  }
  if (tv_state && tv_end_time != 0 && (long)(now - tv_end_time) >= 0) {
    turnOffTv();
    Serial.println("? TV timer expired");
    SerialBT.println("TV OFF (timer)");
    tv_end_time = 0;
    tv_timer = 0;
    updateOLED();
  }
  if (switch_state && switch_end_time != 0 && (long)(now - switch_end_time) >= 0) {
    turnOffSwitch();
    Serial.println("? Switch timer expired");
    SerialBT.println("Switch OFF (timer)");
    switch_end_time = 0;
    switch_timer = 0;
    updateOLED();
  }
}

// ----------------- OLED UI (Option B, clean, no overlap) -----------------

void drawRowBand(int rowIndex, const char* icon, const char* name, bool state, bool highlight) {
  const int ROW_HEIGHT = 12;
  int y = 12 + rowIndex*ROW_HEIGHT;

  if (highlight) {
    display.fillRect(0,y-2,SCREEN_WIDTH,ROW_HEIGHT,SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  } else display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(2,y); display.print("["); display.print(icon); display.print("]");
  display.setCursor(28,y); display.print(name);
  display.setCursor(88,y); display.print(state?"ON":"OFF");

  if (highlight) display.setTextColor(SSD1306_WHITE);
}

void updateOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0); display.print("GESTURE HOME CONTROL");

  drawRowBand(0,"L","Light", light_state, selectedAppliance==1);
  drawRowBand(1,"F","Fan", fan_state, selectedAppliance==2);
  drawRowBand(2,"T","TV", tv_state, selectedAppliance==3);
  drawRowBand(3,"S","Switch", switch_state, selectedAppliance==4);

  display.display();
}
// -------------------- SETUP & LOOP --------------------

void setup() {
  Serial.begin(115200);
  delay(200);

  // Pins
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(RELAY4_PIN, OUTPUT);

  // Set initial relay OFF state according to module logic
  RELAY_OFF(RELAY1_PIN);
  RELAY_OFF(RELAY2_PIN);
  RELAY_OFF(RELAY3_PIN);
  RELAY_OFF(RELAY4_PIN);

  Serial.println("\n=== Voice+Gesture Home (ESP32) ===");

  // Bluetooth
  SerialBT.begin("ESP32_Home");
  Serial.println("Bluetooth Started: ESP32_Home");

  // I2C
  Wire.begin(SDA_PIN, SCL_PIN);

  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("OLED init failed - continuing without display.");
    // If display fails, still keep running the rest (but updateOLED will do nothing)
  } else {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("System Starting...");
    display.display();
    delay(300);
  }

  // MPU init
  Serial.println("Initializing MPU6050...");
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection FAILED! Check wiring.");
    // halt - easier to debug wiring problems
    while (1) { delay(1000); }
  }
  Serial.println("MPU6050 OK");

  // initial OLED
  updateOLED();

  // Small stabilization delay
  delay(200);
}

void loop() {
  // Check and apply timers (turn off devices when timers expire)
  checkTimers();

  // Bluetooth reading (strict commands)
  while (SerialBT.available()) {
    char c = SerialBT.read();
    lastRxMs = millis();

    if (c == '\r') continue; // ignore CR

    Serial.write(c); // echo incoming char for debug

    if (c == '\n') {
      if (voiceCommand.length() > 0) {
        String cmdToProcess = voiceCommand;
        voiceCommand = "";

        Serial.print("\n[BT RX Newline] ");
        Serial.println(cmdToProcess);

        String trimmedCmd = cmdToProcess;
        trimmedCmd.trim();
        if (trimmedCmd.length() == 1) {
          processSingleCommand(trimmedCmd.charAt(0));
        } else {
          processTextCommand(cmdToProcess);
        }
        updateOLED();
      }
    } else {
      voiceCommand += c;
    }
  }

  // Idle timeout processing if sender didn't send newline
  if (voiceCommand.length() > 0 && (millis() - lastRxMs) > idleCutMs) {
    String cmdToProcess = voiceCommand;
    voiceCommand = "";

    Serial.print("\n[Idle Process] ");
    Serial.println(cmdToProcess);

    String trimmedCmd = cmdToProcess;
    trimmedCmd.trim();
    if (trimmedCmd.length() == 1) {
      processSingleCommand(trimmedCmd.charAt(0));
    } else {
      processTextCommand(cmdToProcess);
    }
    updateOLED();
  }

  // -------------------- GESTURE DETECTION (MPU6050) --------------------
  unsigned long now = millis();
  if (now - lastGestureTime >= debounceTime) {
    int16_t ax_raw, ay_raw, az_raw;
    int16_t gx, gy, gz;
    mpu.getMotion6(&ax_raw, &ay_raw, &az_raw, &gx, &gy, &gz);

    float ax = ax_raw / 16384.0f;
    float ay = ay_raw / 16384.0f;

    // RIGHT ? Next appliance
    if (ax > THRESHOLD) {
      selectedAppliance++;
      if (selectedAppliance > 4) selectedAppliance = 1;
      Serial.print("Selected Appliance: ");
      Serial.println(selectedAppliance);
      lastGestureTime = now;
      updateOLED();
    }
    // LEFT ? Previous appliance
    else if (ax < -THRESHOLD) {
      selectedAppliance--;
      if (selectedAppliance < 1) selectedAppliance = 4;
      Serial.print("Selected Appliance: ");
      Serial.println(selectedAppliance);
      lastGestureTime = now;
      updateOLED();
    }
    // UP ? Turn ON selected appliance
    else if (ay > THRESHOLD) {
      controlRelay(selectedAppliance, true);
      Serial.print("Appliance ");
      Serial.print(selectedAppliance);
      Serial.println(" ON (gesture)");
      lastGestureTime = now;
      printStatus();
      updateOLED();
    }
    // DOWN ? Turn OFF selected appliance
    else if (ay < -THRESHOLD) {
      controlRelay(selectedAppliance, false);
      Serial.print("Appliance ");
      Serial.print(selectedAppliance);
      Serial.println(" OFF (gesture)");
      lastGestureTime = now;
      printStatus();
      updateOLED();
    }
  }

  delay(20); // small loop delay
}
