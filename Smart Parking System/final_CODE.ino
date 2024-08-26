#include <SoftwareSerial.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Servo.h> 
#include <TimeLib.h>


LiquidCrystal_I2C lcd(0x27,16,2);
Servo myservo1;
SoftwareSerial mySerial(3, 2);

const int TRIG_PIN = 6;
const int ECHO_PIN = 7;
const int LED_PIN  = 13;
const int DISTANCE_THRESHOLD = 5;

int IR1 = 8;
int IR2 = 4;
int Slot = 4;

unsigned long lastIR1Time = 0;
unsigned long lastIR2Time = 0;
int flag1 = 0;
int flag2 = 0;

enum ParkingState {
  IDLE,
  IR1_TRIGGERED,
  IR2_TRIGGERED,
};

ParkingState currentState = IDLE;

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  mySerial.println("AT");
  mySerial.println("AT+CMGF=1");
  mySerial.println("AT+CNMI=1,2,0,0,0");
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(IR1, INPUT);
  pinMode(IR2, INPUT);
  
  myservo1.attach(9);
  myservo1.write(100);

  lcd.init();
  lcd.backlight();
  
  lcd.setCursor(0, 0);
  lcd.print("     ARDUINO    ");
  lcd.setCursor(0, 1);
  lcd.print(" PARKING SYSTEM ");
  delay(2000);
  lcd.clear(); 
}

void loop() {
  handleSMS();
  handleParkingSensors();
  handleUltrasonicSensor();
  
}

void handleSMS() {
  Serial.println("Checking for SMS...");
  
  while (Serial.available()) {
    mySerial.write(Serial.read());
  }
  
  while (mySerial.available()) {
    if (mySerial.find("+CMT:")) {
      String number = mySerial.readStringUntil(',');
      mySerial.readStringUntil('"');
      mySerial.readStringUntil('"');
      
      String message = mySerial.readStringUntil('\n');
      String restOfText = mySerial.readString();
      message.trim();
      restOfText.trim();
      Serial.print("Rest of Text after newline: ");
      Serial.println(restOfText);

      for (int i = 0; i < restOfText.length(); i++) {
        message.setCharAt(i, tolower(message.charAt(i)));
      }

      Serial.print("Received message from ");
      Serial.println(number);
      Serial.print("Message content: ");
      Serial.println(message);

      if (restOfText.equalsIgnoreCase("Available")) {
        String smsContent = createFormattedMessage(Slot);
        sendSMS(number, smsContent);
      } else {
        sendSMS(number, "Not recognized command! Try again.");
      }
    }
  }
}

String createFormattedMessage(int slotCount) {
  String formattedTime = String(day()) + " " + monthShortStr(month()) + " " + year();
  formattedTime += ", " + String(hourFormat12()) + "." + String(minute()) + " " + (isAM() ? "AM" : "PM");
  String message = String(slotCount) + " Slots free at FOT Car Park as at " + formattedTime + ". Safe parking!";
  return message;
}

void sendSMS(String recipient, String content) {
  Serial.print("Sending SMS to ");
  Serial.println(recipient);
  Serial.print("SMS content: ");
  Serial.println(content);
  
  mySerial.println("AT+CMGS=\"" + recipient + "\"");
  delay(1000);
  mySerial.println(content);
  mySerial.write(26);
}

void handleParkingSensors() {
  unsigned long currentTime = millis();
  
  if(digitalRead (IR1) == LOW && flag1==0){
if(Slot>0){flag1=1;
if(flag2==0){myservo1.write(0); Slot = Slot-1;}
}else{
lcd.setCursor (0,0);
lcd.print("    SORRY :(    ");  
lcd.setCursor (0,1);
lcd.print("  Parking Full  "); 
delay (3000);
lcd.clear(); 
}
}

if(digitalRead (IR2) == LOW && flag2==0){flag2=1;
if(flag1==0){myservo1.write(0); Slot = Slot+1;}
}

if(flag1==1 && flag2==1){
delay (1000);
myservo1.write(100);
flag1=0, flag2=0;
}

lcd.setCursor (0,0);
lcd.print("    WELCOME!    ");
lcd.setCursor (0,1);
lcd.print("Slot Left: ");
lcd.print(Slot);
}

void handleUltrasonicSensor() {
  unsigned long duration_us, distance_cm;
  Serial.print("distance: ");
  Serial.print(distance_cm);
  Serial.println(" cm");


  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration_us = pulseIn(ECHO_PIN, HIGH);
  distance_cm = 0.017 * duration_us;

  if (distance_cm < DISTANCE_THRESHOLD)
    digitalWrite(LED_PIN, HIGH);
  else
    digitalWrite(LED_PIN, LOW);

  delay(500);
}
