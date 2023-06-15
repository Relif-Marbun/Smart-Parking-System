#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>

String UID[3] = {"C4 23 5C 07",
                 "47 E2 AF 6B",
                 "73 EA D8 1B"
                };

#define SS_PIN 53
#define RST_PIN 47
#define portal 10
#define echo  9
#define trig  8

const int IR[6] = {2, 3, 4, 5, 6, 7};
int IRvalue[6];
static String slot = "";
unsigned long currTime = 0;

MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial mySerial(12, 11);
Servo servo;

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

int x = 0;
bool statusMasuk, statusOpen;
static long jarak;

uint8_t getFingerprintID();
int getFingerprintIDez();

void DisplayLCD(int angka) {
  switch (angka) {
    case 0:
      lcd.setCursor(0, 0); lcd.print("   Slot Parkir  ");
      lcd.setCursor(2, 1); lcd.print(slot); lcd.print("                 ");
      break;
    case 1:
      lcd.setCursor(0, 0); lcd.print("  Sistem Parkir ");
      lcd.setCursor(0, 1); lcd.print("    Otomatis    ");
      delay(1500);
      break;
    case 2:
      lcd.setCursor(0, 0); lcd.print(" Silahkan Masuk ");
      lcd.setCursor(0, 1); lcd.print("                ");
      break;
    case 3:
      lcd.setCursor(0, 0); lcd.print("MAAF! ID Anda   ");
      lcd.setCursor(0, 1); lcd.print("Tidak Terdaftar ");
      break;
    case 4:
      lcd.setCursor(0, 0); lcd.print("MAAF! Jari Anda   ");
      lcd.setCursor(0, 1); lcd.print("Tidak Terdaftar ");
      break;
  }
}

long distance(int TRIG, int ECHO) {
  pinMode(TRIG, OUTPUT);    pinMode(ECHO, INPUT);
  digitalWrite(TRIG, LOW);  delayMicroseconds(8);
  digitalWrite(TRIG, HIGH); delayMicroseconds(8);
  digitalWrite(TRIG, LOW);  delayMicroseconds(8);

  long durasi = pulseIn(ECHO, HIGH);
  long jarak = (durasi / 2) / 29.1;
  delay(5);
  return jarak;
}
void fingerprint() {
  while (!Serial);  // For Yun/Leo/Micro/Zero/...
  delay(100);
  Serial.println("\n\nAdafruit finger detect test");

  // set the data rate for the sensor serial port
  finger.begin(57600);
  delay(5);
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) {
      delay(1);
    }
  }

  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
  Serial.print(F("Security level: ")); Serial.println(finger.security_level);
  Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);

  finger.getTemplateCount();

  if (finger.templateCount == 0) {
    Serial.print("Sensor doesn't contain any fingerprint data. Please run the 'enroll' example.");
  }
  else {
    Serial.println("Waiting for valid finger...");
    Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
  }
}
void setup() {
  Serial.begin(9600);
  fingerprint();
  SPI.begin(); mfrc522.PCD_Init();
  lcd.init(); lcd.clear(); lcd.backlight();
  servo.attach(portal);
  servo.write(0);

  for (int i = 0; i < 6; i++) {
    pinMode(IR[i], INPUT);
  }

  DisplayLCD(1);
  Serial.println("Approximate your card to the reader...");
  Serial.println();

}

void loop() {
  getFingerprintID();
  delay(50);
  slot = "";
  jarak = distance( trig, echo);

  for (int i = 0; i < 6; i++) {
    IRvalue[i] = digitalRead(IR[i]);
    if (IRvalue[i] == 1) {
      slot += String(i + 1) + String(" ");
    }
  }
  DisplayLCD(0);

  String content = "";
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
      content.concat(String(mfrc522.uid.uidByte[i], HEX));
    } Serial.println();
    content.toUpperCase();
    for (byte i = 0; i < 5; i++) {
      if (content.substring(1) == UID[i]) {
        statusMasuk = true;
      }
    } if (!statusMasuk) {
      Serial.println("AKses Ditolak");
      DisplayLCD(3);
      servo.write(0);
      statusOpen = false;
      delay(2500); lcd.clear();
    } else if (statusMasuk) {
      Serial.println("Silahkan Masuk");
      servo.write(90);
      DisplayLCD(2);
      statusOpen = true;
      delay(2500); lcd.clear();
    }
  } statusMasuk = false;

  if (statusOpen && jarak <= 2) {
    servo.write(0); statusOpen = false;
    delay(2000);
  } else if (!statusOpen && jarak <= 2) {
    servo.write(90); x = 1;
  } else if (!statusOpen && jarak > 2 && x == 1) {
    delay(2000);
    servo.write(0); x = 0;
  } else if (statusOpen && jarak > 2) {
    servo.write(90);
  }
}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");      
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
    Serial.println("Silahkan Masuk");
      servo.write(90);
      DisplayLCD(2);
      statusOpen = true;
      delay(2500); lcd.clear();
      if (statusOpen && jarak <= 2) {
        servo.write(0);  statusOpen = false;
        delay(2000);
      } else if (!statusOpen && jarak <= 2) {
        servo.write(90); x = 1;
      } else if (!statusOpen && jarak > 2 && x == 1) {
        delay(2000);
        servo.write(0); x = 0;
      } else if (statusOpen && jarak > 2) {
        servo.write(90);
      }
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    Serial.println("AKses Ditolak");
    DisplayLCD(4);
    servo.write(0);
//    statusOpen = false;
    delay(2500); lcd.clear();
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);

  return finger.fingerID;

}

// returns -1 if failed, otherwise returns ID #
int getFingerprintIDez() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID;
}
