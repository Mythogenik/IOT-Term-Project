#include "thingProperties.h"
#include <Wire.h>
#include <MPU6050.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

MPU6050 mpu1(0x68);
MPU6050 mpu2(0x69);

#define MOTOR_PIN 5
#define BUTTON_PIN 4
#define THRESHOLD 500
#define ALWAYS_ON_THRESHOLD 3000
#define BAD_POSTURE_TIME 3000
#define DEBOUNCE_DELAY 300
#define PULSE_ON 600

unsigned long badPostureStart = 0;
unsigned long motorPulseTimer = 0;
bool isBadPosture = false;
bool motorOn = false;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
bool vibratingActive = false;
int offsetY = 0;
int currentPulseOff = 1000; // default slow

void getPulseOff(int difference) {
  if (difference >= ALWAYS_ON_THRESHOLD) {
    currentPulseOff = 0; // always on
  } else if (difference >= 2000) {
    currentPulseOff = 300; // fast
  } else if (difference >= 1000) {
    currentPulseOff = 600; // medium
  } else {
    currentPulseOff = 1000; // slow
  }
}

void calibrate() {
  Serial.println("Calibrating... Stand straight!");

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 10);
  display.println("Calibrating...");
  display.println("");
  display.println("Stand straight!");
  display.println("");
  display.println("Hold 2 seconds...");
  display.display();

  long sum = 0;
  for(int i = 0; i < 20; i++) {
    int16_t ax1, ay1, az1;
    int16_t ax2, ay2, az2;
    mpu1.getAcceleration(&ax1, &ay1, &az1);
    mpu2.getAcceleration(&ax2, &ay2, &az2);
    sum += (ay2 - ay1);
    delay(100);
  }
  offsetY = sum / 20;

  Serial.print("Calibration done! Offset: ");
  Serial.println(offsetY);

  display.clearDisplay();
  display.setCursor(0, 20);
  display.setTextSize(2);
  display.println("Calibrated!");
  display.display();
  delay(1000);
}

void setup() {
  Serial.begin(9600);
  delay(1500);
  ledcSetup(0, 5000, 8);
  ledcAttachPin(MOTOR_PIN, 0);
  ledcWrite(0, 0);  // force motor OFF at boot

  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  Wire.begin(21, 22);
  delay(100);

  ledcSetup(0, 5000, 8);
  ledcAttachPin(MOTOR_PIN, 0);
  ledcWrite(0, 0);

  pinMode(BUTTON_PIN, INPUT);

  mpu1.initialize();
  mpu2.initialize();

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Smart Posture");
  display.println("Monitor");
  display.println("");
  display.println("Press button");
  display.println("to start!");
  display.display();
}

void loop() {
  ArduinoCloud.update();

  // Button toggle
  bool currentButtonState = digitalRead(BUTTON_PIN);
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    if (millis() - lastDebounceTime > DEBOUNCE_DELAY) {
      systemSwitch = !systemSwitch;
      lastDebounceTime = millis();
      ledcWrite(0, 0);
      isBadPosture = false;
      vibratingActive = false;
      motorOn = false;
      badPostureStart = 0;

      if (systemSwitch) {
        calibrate();
      }

      Serial.println(systemSwitch ? "System ON" : "System OFF");
    }
  }
  lastButtonState = currentButtonState;

  if (!systemSwitch) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 20);
    display.println("SYSTEM");
    display.println("OFF");
    display.display();
    ledcWrite(0, 0);
    return;
  }

  // Read sensors
  int16_t ax1, ay1, az1;
  int16_t ax2, ay2, az2;

  mpu1.getAcceleration(&ax1, &ay1, &az1);
  mpu2.getAcceleration(&ax2, &ay2, &az2);

  int difference = abs((ay2 - ay1) - offsetY);

  // Update cloud variables
  neckValue = ay2;
  waistValue = ay1;
  postureDiff = difference;

  // Update pulse frequency based on difference
  getPulseOff(difference);

  // Posture logic
  if (difference > THRESHOLD) {
    if (!isBadPosture) {
      badPostureStart = millis();
      isBadPosture = true;
    }
    if (millis() - badPostureStart > BAD_POSTURE_TIME) {
      vibratingActive = true;
      badPosture = true;
    }
  } else {
    if (isBadPosture) {
      if (millis() - badPostureStart > 1000) {
        isBadPosture = false;
        badPostureStart = 0;
        vibratingActive = false;
        motorOn = false;
        badPosture = false;
        ledcWrite(0, 0);
      }
    }
  }

  // Motor pulsing with dynamic frequency
  if (vibratingActive) {
    if (currentPulseOff == 0) {
      // Always on
      ledcWrite(0, 255);
    } else {
      unsigned long now = millis();
      if (motorOn && now - motorPulseTimer > PULSE_ON) {
        motorOn = false;
        motorPulseTimer = now;
        ledcWrite(0, 0);
      } else if (!motorOn && now - motorPulseTimer > currentPulseOff) {
        motorOn = true;
        motorPulseTimer = now;
        ledcWrite(0, 255);
      }
    }
  }

  // OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Smart Posture Monitor");
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  display.setCursor(0, 14);
  display.print("Neck:  "); display.println(ay2);
  display.print("Waist: "); display.println(ay1);
  display.print("Diff:  "); display.println(difference);

  display.setCursor(0, 50);
  display.setTextSize(2);
  if (vibratingActive) {
    if (currentPulseOff == 0) {
      display.println("STOP!");
    } else {
      display.println("BAD!");
    }
  } else {
    display.println("GOOD");
  }

  display.display();

  Serial.print("Diff: "); Serial.print(difference);
  Serial.print(" | PulseOff: "); Serial.print(currentPulseOff);
  Serial.println(vibratingActive ? " | VIBRATING" : " | GOOD");

  delay(200);
}

void onSystemSwitchChange() {
  if (systemSwitch) {
    calibrate();
    Serial.println("System ON from Cloud!");
  } else {
    ledcWrite(0, 0);
    isBadPosture = false;
    vibratingActive = false;
    motorOn = false;
    badPosture = false;
    Serial.println("System OFF from Cloud!");
  }
}