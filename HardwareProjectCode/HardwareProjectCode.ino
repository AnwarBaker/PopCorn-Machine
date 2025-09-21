#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>

// ===================== LCD I2C =====================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ===================== RFID Setup =====================
#define SS_PIN 53
#define RST_PIN 32
MFRC522 rfid(SS_PIN, RST_PIN);

// ===================== Keypad setup =====================
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {22,23,24,25};
byte colPins[COLS] = {26,27,28};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ===================== Servos =====================
Servo servo1;
Servo servo2;

// ===================== Stepper Pins =====================
// Stepper1 (A4988)
#define stepPin 4
#define dirPin 5
#define enablePin 6
// Stepper2 (TB6600)
#define stepPin2 3
#define dirPin2 2

// ===================== Pump Relay =====================
#define pumpRelay 35 //it was 30
#define HeaterRelay 29

// ===================== DC Motors =====================
// Motor A
const int enA = 10;
const int in1 = 36;
const int in2 = 37;
// Motor B
const int enB = 7;
const int in3 = 39;
const int in4 = 40;

// ===================== Relays & IR =====================
#define DVD_RELAY 47
#define FAN_RELAY 45
#define IR_SENSOR 43

// ===================== Ultrasonic Sensor =====================
#define trigPin 12
#define echoPin 13

// ===================== RFID Authentication =====================
bool rfidAuthenticated = false;

// ===================== Function Prototypes =====================
void checkRFID();
void runMode1(bool isSmall);
void runMode2(bool isSmall);
void runMode3(bool isSmall);
void driveMotorA(int speed, boolean forward);
void stopMotorA();
void driveMotorB(int speed, boolean forward);
void stopMotorB();
void runHeater7Minutes();
void runHeater8Minutes();
void runStepper1(long duration, int pulseDelay);
void runStepper2(bool direction, long duration, int pulseDelay);
void runServos(bool doubleSpeed);
void runPump(long duration);
void runDVD(long duration);
void runFan(long duration);
void waitForIRSensor();
void readUltrasonic();
void runPump(long duration);




void setup() {
  Serial.begin(9600);
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  pinMode(enablePin, OUTPUT);
  digitalWrite(enablePin, HIGH);

  

  // RFID Initialization
  SPI.begin();
  rfid.PCD_Init();
  
  pinMode(HeaterRelay, OUTPUT);

  digitalWrite(HeaterRelay, HIGH);

  // Stepper2
  pinMode(stepPin2, OUTPUT);
  pinMode(dirPin2, OUTPUT);
  digitalWrite(dirPin2, HIGH);

  // Pump relay
  pinMode(pumpRelay, OUTPUT);
  digitalWrite(pumpRelay, HIGH);

  // DC Motors
  pinMode(enA, OUTPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(enB, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);

  // Relays
  pinMode(DVD_RELAY, OUTPUT);
  pinMode(FAN_RELAY, OUTPUT);
  digitalWrite(DVD_RELAY, HIGH);
  digitalWrite(FAN_RELAY, HIGH);

  // IR Sensor
  pinMode(IR_SENSOR, INPUT);

  // Ultrasonic pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // LCD
  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Scan RFID Card");

  // Servos initial position
  servo1.attach(8);
  servo2.attach(31);
  servo1.write(5);
  servo2.write(2);
  delay(500);
  servo1.detach();
  servo2.detach();

  
  Serial.println("System Ready. Place RFID card...");
}

void loop() {
  if (!rfidAuthenticated) {
    checkRFID();
    return;
  }

  char key = keypad.getKey();
  if (!key) {
    readUltrasonic();
    return;
  }

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Mode ");
  lcd.print(key);
  Serial.print("Mode ");
  Serial.println(key);

  // اختيار المود
  if (key == '1' || key == '3' || key == '4') {
    char sizeKey = 0;
    // lcd.clear();
    // lcd.setCursor(0,0);
    // lcd.print("Press 1 Small");
    // lcd.setCursor(0,1);
    // lcd.print("Press 3 Big");
lcd.clear();
lcd.setCursor(0,0);
lcd.print("                "); // مسح السطر الأول
lcd.setCursor(0,1);
lcd.print("                "); // مسح السطر الثاني
lcd.setCursor(0,0);
lcd.print("Press 1 Small");
lcd.setCursor(0,1);
lcd.print("Press 3 Big");

    // انتظار اختيار الحجم
    unsigned long sizeStartTime = millis();
    while (!sizeKey && millis() - sizeStartTime < 10000) {
      sizeKey = keypad.getKey();
      delay(50);
    }

    if (sizeKey == '1') {
      Serial.println("Small selected");
      if (key == '1') runMode1(true);
      else if (key == '3') runMode2(true);
      else if (key == '4') runMode3(true);
    } 
    else if (sizeKey == '3') {
      Serial.println("Big selected");
      if (key == '1') runMode1(false);
      else if (key == '3') runMode2(false);
      else if (key == '4') runMode3(false);
    }
    else {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("No size selected");
      delay(2000);
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Ready...");
      return;
    }
  }

  readUltrasonic();
}

void checkRFID() {
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }
  
  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  Serial.print("Card UID: ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? "0" : "");
    Serial.print(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) Serial.print(":");
  }
  Serial.println();

  rfidAuthenticated = true;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Welcome !");
  Serial.println("Access Granted!");

  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Ready...");

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

// ===================== دوال مساعدة محسنة =====================

void runStepper1(long duration, int pulseDelay) {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Stepper1 ON");
  digitalWrite(dirPin, HIGH);
  digitalWrite(enablePin, LOW);
  unsigned long startTime = millis();
  while (millis() - startTime < duration) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(pulseDelay);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(pulseDelay);
  }
  digitalWrite(enablePin, HIGH);
}

void runStepper2(bool direction, long duration, int pulseDelay) {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(direction ? "Stepper2 CCW" : "Stepper2 CW");
  digitalWrite(dirPin2, direction ? HIGH : LOW);
  unsigned long startTime = millis();
  while (millis() - startTime < duration) {
    digitalWrite(stepPin2, HIGH);
    delayMicroseconds(pulseDelay);
    digitalWrite(stepPin2, LOW);
    delayMicroseconds(pulseDelay);
  }
}

void runServos(bool doubleSpeed) {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Servos Moving");
  if (doubleSpeed) {
    lcd.setCursor(0,1);
    lcd.print("Servo1 x2 Speed");
  }
  
  servo1.attach(8);
  servo2.attach(31);
  unsigned long startTime = millis();
  while (millis() - startTime < 5000) {
    for(int pos = 0; pos <= 6; pos += 2) {
      servo1.write(doubleSpeed ? pos * 2 : pos);
      servo2.write(pos);
      delay(300);
    }
    for(int pos = 6; pos >= 0; pos -= 2) {
      servo1.write(doubleSpeed ? pos * 2 : pos);
      servo2.write(pos);
      delay(300);
    }
  }
  servo1.write(5);
  servo2.write(2);
  delay(500);
  servo1.detach();
  servo2.detach();
}

// void runPump(long duration) {
//   lcd.clear();
//   lcd.setCursor(0,0);
//   lcd.print("Pump ON");
//   digitalWrite(pumpRelay, LOW);
//   delay(duration);
//   digitalWrite(pumpRelay, HIGH);
// }

void runDVD(long duration) {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("DVD Relay ON");
  digitalWrite(DVD_RELAY, LOW);
  delay(duration);
  digitalWrite(DVD_RELAY, HIGH);
}

void runFan(long duration) {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Fan ON");
  digitalWrite(FAN_RELAY, LOW);
  delay(duration);
  digitalWrite(FAN_RELAY, HIGH);
}


void runPump(long duration) {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Pump ON");
  digitalWrite(pumpRelay, LOW);
  unsigned long startTime = millis();
  while (millis() - startTime < duration) {
    // Allow other operations to continue if needed
    delay(100);
  }
  digitalWrite(pumpRelay, HIGH);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Pump OFF");
  delay(500);
}



void waitForIRSensor() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Waiting for IR...");
  Serial.println("Waiting for IR Sensor...");
  while(digitalRead(IR_SENSOR) == HIGH) {
    delay(10);
  }
  delay(3000);
}

void readUltrasonic() {
  long duration, distance;
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.0343 / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  if (distance > 20) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("You have to add");
    lcd.setCursor(0, 1);
    lcd.print("corn!");
  }

  delay(500);
}

// ===================== دوال الأوضاع المحسنة =====================

void runMode1(bool isSmall) {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Mode 1 Started");
  
  // Stepper2 CW
  runStepper2(false, 1790, 2000);
  delay(2000);



//  lcd.clear();
//   lcd.setCursor(0,0);
//   lcd.print("Pump ON");
//   digitalWrite(pumpRelay, LOW);   // تشغيل
// delay(2000);
// digitalWrite(pumpRelay, HIGH);  // إطفاء
// delay(100);                     // تأكيد رجوع الريلاي
runPump(2000);

delay(1000);
lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Motor B ON");
  
  driveMotorB(200, true);
  // Stepper1 حسب الحجم
  if (isSmall) {
    runStepper1(1150, 2000);
    runHeater5_5Minutes();
  } else {
    runStepper1(1250, 2100);
    runHeater6_5Minutes();
  }
  
 


  // Motor A
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Motor A ON");
  driveMotorA(255, true);
  
  // DVD Relay
  runDVD(4000);
  
  // IR Sensor
  waitForIRSensor();
  
  stopMotorA();
  digitalWrite(DVD_RELAY, LOW);
  delay(1000);
  digitalWrite(DVD_RELAY, HIGH);
  
  // Servos
  runServos(false);
  
  // Stepper2 CW
   runStepper2(false, 7120, 2000);//7080
  delay(10000);
  stopMotorB();
  
  // Stepper2 CCW
  runStepper2(true, 9020, 2000);//8980
  
  // Motor A
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Motor A ON");
  driveMotorA(255, true);
  
  // Fan
  runFan(30000);
  stopMotorA();


  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Mode 1 Complete");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Ready...");
}

 // Run pump for 2 seconds


void runMode2(bool isSmall) {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Mode 2 Started");
  
  // Stepper2 CW
  runStepper2(false, 1790, 2000);
  delay(2000);
  


 
//  lcd.clear();
//   lcd.setCursor(0,0);
//   lcd.print("Pump ON");
//   digitalWrite(pumpRelay, LOW);   // تشغيل
// delay(2000);
// digitalWrite(pumpRelay, HIGH);  // إطفاء
// delay(100);                     // تأكيد رجوع الريلاي

  // In your mode functions, replace the pump code with:
runPump(2000);


delay(1000);
lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Motor B ON");
  
  driveMotorB(200, true);
  // Stepper1 حسب الحجم
   if (isSmall) {
    runStepper1(1150, 2000);
    runHeater5_5Minutes();
  } else {
    runStepper1(1250, 2100);
    runHeater6_5Minutes();
  }
  // Motor B
  // lcd.clear();
  // lcd.setCursor(0,0);
  // lcd.print("Motor B ON");
  // driveMotorB(200, true);
  
  // Pump


   



  // Motor A
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Motor A ON");
  driveMotorA(255, true);
  
  // DVD Relay
  runDVD(4000);
  
  // IR Sensor
  waitForIRSensor();
  
  stopMotorA();
  digitalWrite(DVD_RELAY, LOW);
  delay(1000);
  digitalWrite(DVD_RELAY, HIGH);
  
  // Servo2 فقط
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Servo2 Moving");
  servo2.attach(31);
  unsigned long startTime = millis();
  while (millis() - startTime < 5000) {
    for(int pos = 0; pos <= 6; pos += 2) {
      servo2.write(pos);
      delay(300);
    }
    for(int pos = 6; pos >= 0; pos -= 2) {
      servo2.write(pos);
      delay(300);
    }
  }
  servo2.write(2);
  delay(500);
  servo2.detach();
  
  // Stepper2 CW
   runStepper2(false, 7120, 2000);//7080
  delay(10000);
  stopMotorB();
  
  // Stepper2 CCW
  runStepper2(true, 9020, 2000);//8980
  
  // Fan
  runFan(30000);
  stopMotorA();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Mode 2 Complete");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Ready...");
}





void runMode3(bool isSmall) {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Mode 3 Started");
  
  // Stepper2 CW
  runStepper2(false, 1790, 2000);
  delay(2000);
  

//  lcd.clear();
//   lcd.setCursor(0,0);
//   lcd.print("Pump ON");
//   digitalWrite(pumpRelay, LOW);   // تشغيل
// delay(2000);
// digitalWrite(pumpRelay, HIGH);  // إطفاء
// delay(100);                     // تأكيد رجوع الريلاي

  // In your mode functions, replace the pump code with:
runPump(2000);
delay(1000);


 lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Motor B ON");
  

  driveMotorB(200, true);
  // Stepper1 حسب الحجم
 if (isSmall) {
    runStepper1(1150, 2000);
    runHeater5_5Minutes();
  } else {
    runStepper1(1250, 2100);
    runHeater6_5Minutes();
  }
  
  // Pump



  
  // Motor A
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Motor A ON");
  driveMotorA(255, true);
  
  // DVD Relay
  runDVD(4000);
  
  // IR Sensor
  waitForIRSensor();
  
  stopMotorA();
  digitalWrite(DVD_RELAY, LOW);
  delay(1000);
  digitalWrite(DVD_RELAY, HIGH);
  
  // Servos مع مضاعفة سرعة Servo1
  runServos(true);
  
  // Stepper2 CW
  runStepper2(false, 7120, 2000);//7080
  delay(10000);
  stopMotorB();
  
  // Stepper2 CCW
  runStepper2(true, 9020, 2000);//8980
  
  // Motor A
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Motor A ON");
  driveMotorA(255, true);
  
  // Fan
  runFan(30000);
  stopMotorA();

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Mode 3 Complete");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Ready...");
}

// ===================== دوال المحركات والتسخين =====================

void driveMotorA(int speed, boolean forward){
  analogWrite(enA, speed);
  digitalWrite(in1, forward ? HIGH : LOW);
  digitalWrite(in2, forward ? LOW : HIGH);
}

void stopMotorA(){
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  analogWrite(enA, 0);
}

void driveMotorB(int speed, boolean forward){
  analogWrite(enB, speed);
  digitalWrite(in3, forward ? HIGH : LOW);
  digitalWrite(in4, forward ? LOW : HIGH);
}



void stopMotorB(){
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
  analogWrite(enB, 0);
}


void runHeater5_5Minutes() {
  unsigned long heaterStart = millis();
  unsigned long heaterDuration = 410000; // 5.5 دقائق = 5.5 * 60 * 1000 = 330000   it was 330000 now 6 and 50 seconeds
  

  int lastShownMinute = -1;

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Heater Running");
  Serial.println("Heater ON for 5.5 minutes");

  digitalWrite(HeaterRelay, LOW);

  while (millis() - heaterStart < heaterDuration) {
    unsigned long elapsed = millis() - heaterStart;
    int secondsLeft = (heaterDuration - elapsed) / 1000;
    int minutesLeft = secondsLeft / 60;
    int remainingSeconds = secondsLeft % 60;

    if (minutesLeft != lastShownMinute) {
      lcd.setCursor(0,1);
      lcd.print("Time Remain: ");
      lcd.print(minutesLeft);
      lcd.print(":");
      if (remainingSeconds < 10) lcd.print("0");
      lcd.print(remainingSeconds);
      lcd.print("   "); // مساحة إضافية لمسح أي بقايا
      
      Serial.print("Heater time left: ");
      Serial.print(minutesLeft);
      Serial.print(":");
      if (remainingSeconds < 10) Serial.print("0");
      Serial.println(remainingSeconds);

      lastShownMinute = minutesLeft;
    }
    delay(1000);
  }

  digitalWrite(HeaterRelay, HIGH);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Heater OFF");
  Serial.println("Heater OFF");
}

void runHeater6_5Minutes() {
  unsigned long heaterStart = millis();
  unsigned long heaterDuration = 450000; // 6.5 دقائق = 6.5 * 60 * 1000 ="390000  it was 390000  now 7.5 minutes
  int lastShownMinute = -1;

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Heater Running");
  Serial.println("Heater ON for 6.5 minutes");

  digitalWrite(HeaterRelay, LOW);

  while (millis() - heaterStart < heaterDuration) {
    unsigned long elapsed = millis() - heaterStart;
    int secondsLeft = (heaterDuration - elapsed) / 1000;
    int minutesLeft = secondsLeft / 60;
    int remainingSeconds = secondsLeft % 60;

    if (minutesLeft != lastShownMinute) {
      lcd.setCursor(0,1);
      lcd.print("Time Remain: ");
      lcd.print(minutesLeft);
      lcd.print(":");
      if (remainingSeconds < 10) lcd.print("0");
      lcd.print(remainingSeconds);
      lcd.print("   "); // مساحة إضافية لمسح أي بقايا
      
      Serial.print("Heater time left: ");
      Serial.print(minutesLeft);
      Serial.print(":");
      if (remainingSeconds < 10) Serial.print("0");
      Serial.println(remainingSeconds);

      lastShownMinute = minutesLeft;
    }
    delay(1000);
  }

  digitalWrite(HeaterRelay, HIGH);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Heater OFF");
  Serial.println("Heater OFF");
}