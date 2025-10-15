// ESP32-S3-Zero Crawler Robot

#include "Adafruit_VL53L0X.h"

Adafruit_VL53L0X lox = Adafruit_VL53L0X();

#define MOTOR_BAK_IN1 2
#define MOTOR_BAK_IN2 3
#define MOTOR_BAK_EN 1

#define MOTOR_FRAM_IN3 4
#define MOTOR_FRAM_IN4 5
#define MOTOR_FRAM_EN 6

#define MIN_AVSTAND_FRAM 200 // Stop and act if object is closer than 200mm
#define HASTIGHET_NORMAL 200 // PWM-value (0-255) for normal speed
#define HASTIGHET_BACKA 150 // PWM-value for reverse

// PWM settings for ESP32 (ledc)
const int pwmKanalBak = 0; // Choose a PWM Channel
const int pwmFreq = 5000; // Frequency in Hz
const int pwmUpplosning = 8; // 8-bit resolution (0-255)

// Values to keep track of current state
enum BilStatus { KOR_FRAMAT, VAXLAR, BACKAR, SVANGER, STANNA };
BilStatus nuvarandeStatus = STANNA;

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(1); }

  Serial.println("Självkörande bil startar...");

  if (!lox.begin()) {
    Serial.println(F("Kunde inte initiera VL53L0X. Kontrollera kopplingar."));
    while (1);
  }
  Serial.println("VL53L0X sensor hittad!");

  pinMode(MOTOR_BAK_IN1, OUTPUT);
  pinMode(MOTOR_BAK_IN2, OUTPUT);
  pinMode(MOTOR_FRAM_IN3, OUTPUT);
  pinMode(MOTOR_FRAM_IN4, OUTPUT);

  // Configure PWM for driving motor with ESP32:s ledc-function
  ledcSetup(pwmKanalBak, pwmFreq, pwmUpplosning);
  ledcAttachPin(MOTOR_BAK_EN, pwmKanalBak);

  Serial.println("Setup klar. Startar huvudloop");
  delay(1000);
  stannaBilen(); // Make sure the car is standing still at start
}

void loop() {
  int avstand = lasAvstand(); // Read distance
  if (avstand < 0) {
    Serial.printlin("Sensorfel");
    stannaBilen();
    delay(500);
    return; // Try again
  }

  // Print distance for debugging
  Serial.print("Avstånd: ");
  Serial.print(avstand);
  Serial.println(" mm");

  // Make decision and act
  korLogik(avstand);

  delay(50);
}

/*
 * Read distance with VL53L0X
 * Return distance in mm or -1 if error
 */
int lasAvstand() {
  VL53L0X_RangingMeasurementData_t measure;

  lox.rangingTest(&measure, false); // pass in true to get debug data printout

  if (measure.RangeStatus != 4) {  // phase failures have incorrect data
    return measure.RangeMillimeter;
  } else {
    return -1; // Error in reading
  }
}

void korLogik(int avstand) {
  if (avstand < MIN_AVSTAND_FRAM && nuvarandeStatus == KOR_FRAMAT) {
    nuvarandeStatus = VAXLAR;
    Serial.println("HINDER! Stannar och backar...");

    stannaBilen();
    delay(300);

    backa(HASTIGHET_BACKA);
    delay(1000);

    stannaBilen();
    delay(300);

    Serial.println("Svänger...");
    svangHoger();
    delay(1000);

    stannaStyrning();
    delay(200);

    nuvarandeStatus = KOR_FRAMAT;
  } else if (nuvarandeStatus != KOR_FRAMAT && avstand >= MIN_AVSTAND_FRAM) {
    Serial.println("Fri väg, kör framåt.");
    nuvarandeStatus = KOR_FRAMAT;
    korFramat(HASTIGHET_NORMAL);
  }
}

void korFramat(int hastighet) {
  digitalWrite(MOTOR_BAK_IN1, HIGH);
  digitalWrite(MOTOR_BAK_IN2, LOW);
  ledcWrite(pwmKanalBak, hastighet);
}

void backa(int hastighet) {
  digitalWrite(MOTOR_BAK_IN1, LOW);
  digitalWrite(MOTOR_BAK_IN2, HIGH);
  ledcWrite(pwmKanalBak, hastighet);
}

void stannaBilen() {
  digitalWrite(MOTOR_BAK_IN1, LOW);
  digitalWrite(MOTOR_BAK_IN2, LOW);
  ledcWrite(pwmKanalBak, 0);
  stannaStyrning();
}

void svangHoger() {
  digitalWrite(MOTOR_FRAM_IN3, HIGH);
  digitalWrite(MOTOR_FRAM_IN4, LOW);
}

void svangVanster() {
  digitalWrite(MOTOR_FRAM_IN3, LOW);
  digitalWrite(MOTOR_FRAM_IN4, HIGH);
}

void stannaStyrning() {
  digitalWrite(MOTOR_FRAM_IN3, LOW);
  digitalWrite(MOTOR_FRAM_IN4, LOW);
}