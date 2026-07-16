/********************************************************************************
  Blynk Agricultural Scarecrow & Smart Irrigation Control System
  Board: ESP32 DevKit V1 (or standard ESP32)
  
  Features:
  - Soil moisture measurement uploaded to Blynk Gauge (Virtual Pin V3)
  - Control Eagle Pole Rotation (Relay 1) via Blynk Switch (Virtual Pin V1)
  - Control Irrigation Pump Motor (Relay 2) via Blynk Switch (Virtual Pin V2)
  - Control audio deterrence Buzzer via Blynk Button (Virtual Pin V4)
  - Customizable low moisture threshold adjustable via Blynk Slider (Virtual Pin V5)
  - Automatic Blynk Event/Email alerting when soil moisture drops below threshold
********************************************************************************/

/* Fill-in information from Blynk Device Info (Device Info Tab in Console) */
#define BLYNK_TEMPLATE_ID           "TMPLxxxxxx"
#define BLYNK_TEMPLATE_NAME         "Eagle Scarecrow System"
#define BLYNK_AUTH_TOKEN            "YourAuthToken"

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// Wi-Fi Credentials
char ssid[] = "YourWiFiSSID";
char pass[] = "YourWiFiPassword";

// Pin configuration
#define SOIL_MOISTURE_PIN   34  // Analog pin for Soil Moisture Sensor (ADC1 is Wi-Fi safe)
#define EAGLE_RELAY_PIN     25  // GPIO 25 for Rotating Eagle Pole Relay
#define PUMP_RELAY_PIN      26  // GPIO 26 for Water Pump Relay
#define BUZZER_PIN          27  // GPIO 27 for Buzzer Alert

// Calibration values for Soil Moisture (adjust these based on your sensor)
// These values represent analog readings in dry air vs. pure water.
const int AirValue = 3500;   // Reading in dry air (higher value)
const int WaterValue = 1200; // Reading in pure water (lower value)

// Variables
int soilMoisturePercent = 0;
int dryThreshold = 30;       // Default threshold: 30%. Can be adjusted from Blynk V5
bool isLowMoistureAlertSent = false;
unsigned long lastAlertTime = 0;
const unsigned long ALERT_INTERVAL = 3600000; // 1 hour cooldown for alerts to avoid spam

BlynkTimer timer;

// Virtual Pin Callbacks

// V1: Eagle Relay Control (Switch Widget)
BLYNK_WRITE(V1) {
  int value = param.asInt();
  digitalWrite(EAGLE_RELAY_PIN, value);
  Serial.print("Eagle Relay V1 state changed to: ");
  Serial.println(value);
}

// V2: Water Pump Relay Control (Switch Widget)
BLYNK_WRITE(V2) {
  int value = param.asInt();
  digitalWrite(PUMP_RELAY_PIN, value);
  Serial.print("Water Pump Relay V2 state changed to: ");
  Serial.println(value);
}

// V4: Buzzer Alert Control (Button or Switch Widget)
BLYNK_WRITE(V4) {
  int value = param.asInt();
  digitalWrite(BUZZER_PIN, value);
  Serial.print("Buzzer V4 state changed to: ");
  Serial.println(value);
}

// V5: Adjust Dry Threshold (0 - 100% Slider Widget)
BLYNK_WRITE(V5) {
  dryThreshold = param.asInt();
  Serial.print("Moisture Dry Threshold updated to: ");
  Serial.print(dryThreshold);
  Serial.println("%");
}

// Function to read sensors and upload data to Blynk
void checkSensors() {
  int rawAnalog = analogRead(SOIL_MOISTURE_PIN);
  
  // Calculate percentage (map raw input to 0-100%)
  // Map AirValue (dry, 0%) to WaterValue (wet, 100%)
  soilMoisturePercent = map(rawAnalog, AirValue, WaterValue, 0, 100);
  
  // Constrain value to 0-100 to handle out of bounds readings
  soilMoisturePercent = constrain(soilMoisturePercent, 0, 100);
  
  Serial.print("Soil Moisture Raw: ");
  Serial.print(rawAnalog);
  Serial.print(" | Percentage: ");
  Serial.print(soilMoisturePercent);
  Serial.println("%");
  
  // Upload value to Blynk Gauge (Virtual Pin V3)
  Blynk.virtualWrite(V3, soilMoisturePercent);
  
  // Soil Moisture threshold alert logic
  if (soilMoisturePercent < dryThreshold) {
    unsigned long currentMillis = millis();
    // Send email alert if not recently sent (cooldown to avoid spamming the user)
    if (!isLowMoistureAlertSent || (currentMillis - lastAlertTime > ALERT_INTERVAL)) {
      String alertMessage = "Low soil moisture: " + String(soilMoisturePercent) + 
                            "%. Threshold: " + String(dryThreshold) + 
                            "%. Switch on the pump to water the crops.";
      
      // Trigger the Blynk event configured in your dashboard
      Blynk.logEvent("low_soil_moisture", alertMessage);
      
      Serial.println("Soil moisture alert triggered. Event sent to Blynk.");
      isLowMoistureAlertSent = true;
      lastAlertTime = currentMillis;
    }
  } else {
    // Reset alert trigger when soil moisture goes above threshold
    isLowMoistureAlertSent = false;
  }
}

void setup() {
  Serial.begin(115200);
  
  // Pin modes setup
  pinMode(SOIL_MOISTURE_PIN, INPUT);
  pinMode(EAGLE_RELAY_PIN, OUTPUT);
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Turn off relays & buzzer initially 
  // (Change LOW to HIGH if your relays are active-LOW)
  digitalWrite(EAGLE_RELAY_PIN, LOW);
  digitalWrite(PUMP_RELAY_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  
  // Connect to Blynk cloud
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  
  // Setup timer to run sensor check every 2 seconds (2000ms)
  timer.setInterval(2000L, checkSensors);
}

void loop() {
  Blynk.run();
  timer.run();
}
