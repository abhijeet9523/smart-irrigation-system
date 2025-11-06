#define BLYNK_TEMPLATE_ID "TMPL3XMmwJczI"
#define BLYNK_TEMPLATE_NAME "Smart Plant"
#define BLYNK_AUTH_TOKEN "VUG1LAiHOwSAfCMV9ocv1GvQYDrZ9S49"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

// WiFi credentials
char ssid[] = "POCO";
char pass[] = "Pranjal@25";

// Pin Definitions
#define DHTPIN D6
#define DHTTYPE DHT22
#define SOIL_PIN A0
#define PIR_PIN D5
#define BUZZER_PIN D10
#define PUMP_PIN D7
#define SERVO_PIN D8  // Servo on D8

// Blynk Virtual Pins
#define VPIN_TEMP V0
#define VPIN_HUMIDITY V1
#define VPIN_MOISTURE V3
#define VPIN_LED V5
#define VPIN_PIR_SWITCH V6
#define VPIN_PUMP_SWITCH V12
#define VPIN_SERVO_SWITCH V10

// Flags and variables
bool pirEnabled = true;
bool pumpState = false;
bool manualPumpOverride = false;
bool autoPumpFlag = true;
bool moistureNotified = false;

unsigned long manualOverrideTime = 0;
const unsigned long overrideDuration = 2 * 60 * 1000; // 2 minutes

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
BlynkTimer timer;
Servo myServo;

// BLYNK INPUT HANDLERS
BLYNK_WRITE(VPIN_PIR_SWITCH) {
  pirEnabled = param.asInt();
}

BLYNK_WRITE(VPIN_PUMP_SWITCH) {
  pumpState = param.asInt();
  digitalWrite(PUMP_PIN, pumpState);
  manualPumpOverride = true;
  manualOverrideTime = millis();
}

BLYNK_WRITE(VPIN_SERVO_SWITCH) {
  int servoState = param.asInt();
  if (servoState == 1) {
    myServo.write(90);
    Serial.println("Servo ON (90°)");
  } else {
    myServo.write(0);
    Serial.println("Servo OFF (0°)");
  }
}

// SENSOR READING AND LOGIC
void sendSensorData() {
  float humidity = dht.readHumidity();
  float temp = dht.readTemperature();
  int soilValue = analogRead(SOIL_PIN);
  bool motionDetected = digitalRead(PIR_PIN);

  int moisturePercent = map(soilValue, 1023, 300, 0, 100);
  moisturePercent = constrain(moisturePercent, 0, 100);

  // Serial Monitor Output
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.print(" °C, Humidity: ");
  Serial.print(humidity);
  Serial.print(" %, Moisture: ");
  Serial.print(moisturePercent);
  Serial.println(" %");
    
  Wire.begin(D2, D1);  // D2 = SDA, D1 = SCL
  lcd.begin(16, 2);
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("T:"); lcd.print(temp, 1); lcd.print("C ");
  lcd.print("H:"); lcd.print(humidity, 0); lcd.print("%");
  lcd.setCursor(0, 1);
  lcd.print("Moist: "); lcd.print(moisturePercent); lcd.print("% ");

  /*LCD Display
  
  lcd.setCursor(0, 0);
  lcd.print("T:"); lcd.print(temp, 1); lcd.print("C ");
  lcd.print("H:"); lcd.print(humidity, 0); lcd.print("%");

  lcd.setCursor(0, 1);
  lcd.print("Moist: "); lcd.print(moisturePercent); lcd.print("% ");
*/
  // PIR + Buzzer
  if (pirEnabled && motionDetected) {
    digitalWrite(BUZZER_PIN, HIGH);
    Blynk.virtualWrite(VPIN_LED, 255);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    Blynk.virtualWrite(VPIN_LED, 0);
  }

  // Pump Logic
  if (moisturePercent > 5) {
    digitalWrite(PUMP_PIN, HIGH);
    Blynk.virtualWrite(VPIN_PUMP_SWITCH, 1);
    Blynk.logEvent("emergency_pump", "Critical moisture level! Pump activated.");
   // lcd.setCursor(11, 1);
   // lcd.print("E");
    Serial.println("Emergency: Pump ON");
  }
  else if (manualPumpOverride && (millis() - manualOverrideTime <= overrideDuration)) {
    digitalWrite(PUMP_PIN, pumpState);
   // lcd.setCursor(11, 1);
   // lcd.print("M");
    Serial.println("Manual pump mode");
  }
  else if (autoPumpFlag && moisturePercent < 30) {
    if (!moistureNotified) {
      Blynk.logEvent("low_moisture", "Soil moisture is low!");
      moistureNotified = true;
    }
    digitalWrite(PUMP_PIN, HIGH);
    Blynk.virtualWrite(VPIN_PUMP_SWITCH, 1);
   // lcd.setCursor(11, 1);
   // lcd.print("A");
    Serial.println("Auto pump ON");
  } else {
    moistureNotified = false;
    digitalWrite(PUMP_PIN, LOW);
    Blynk.virtualWrite(VPIN_PUMP_SWITCH, 0);
   // lcd.setCursor(11, 1);
   // lcd.print("O");
  }

  // Send to Blynk
  Blynk.virtualWrite(VPIN_TEMP, temp);
  Blynk.virtualWrite(VPIN_HUMIDITY, humidity);
  Blynk.virtualWrite(VPIN_MOISTURE, moisturePercent);
}

void setup() {
  Serial.begin(9600);
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  dht.begin();
  myServo.attach(SERVO_PIN);
  myServo.write(0);

 // lcd.begin(16, 2);
 // lcd.backlight();
 // lcd.clear();

  pinMode(PIR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);

/*  lcd.setCursor(0, 0);
  lcd.print("Smart Plant");
  lcd.setCursor(0, 1);
  lcd.print("Monitoring...");
  delay(2000);
  lcd.clear();
*/
  timer.setInterval(2000L, sendSensorData);
}




void loop() {
  Blynk.run();
  timer.run();
}