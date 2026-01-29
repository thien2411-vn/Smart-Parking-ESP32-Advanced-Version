#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h> 
#include <ESP32Servo.h>
#include <map>
#include <string>
#include <EEPROM.h>
#include <time.h> 

#define SS_IN_PIN 3
#define RST_IN_PIN 5
#define SS_OUT_PIN 1
#define RST_OUT_PIN 22
#define SERVO_IN_PIN 14 
#define SERVO_OUT_PIN 13 
#define SERVO_DISPENSE 12 
#define LED_PIN 27
#define BUZZER_PIN 32
#define BUTTON_PIN 4 

#define IR_IN_PIN 16 
#define IR_OUT_PIN 17 

LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 rfidIn(SS_IN_PIN, RST_IN_PIN);
MFRC522 rfidOut(SS_OUT_PIN, RST_OUT_PIN);
Servo servoIn;
Servo servoOut;
Servo servoDispense;

const char* ssid = "Thiên";
const char* password = "12345678999";

String scriptUrl = "https://script.google.com/macros/s/AKfycbzfUEJVE6pk578YD4KWKI5JW17XoMAXjSb3wJ3Qn_Zo-HqPttazo5kb5jl36mF0S4ap/exec";

std::map<String, time_t> entryTimeEpoch;

bool cardDispensed = false;
bool waitingForCardTake = false;
unsigned long dispenseTime = 0;
// Cấu hình EEPROM
#define EEPROM_SIZE 512
#define ID_LEN 8 
#define ENTRY_RECORD_SIZE (ID_LEN + sizeof(time_t)) // 8 + 8 = 16 bytes
// Biến toàn cục mới cho thời gian thực
time_t bootTime = 0;          
bool timeSynced = false;     

String urlencode(String str) {
  String encoded = "";
  for (int i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += c;
    } else {
      encoded += '%';
      encoded += String(c >> 4, HEX);
      encoded += String(c & 0xF, HEX);
    }
  }
  return encoded;
}

void sendToGoogleSheets(String id, String status, float fee = 0) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = scriptUrl + "?id=" + urlencode(id) + "&status=" + urlencode(status);
    if (fee > 0) url += "&fee=" + String((int)fee);
    Serial.println("Gửi: " + url);
    http.begin(url);
    int code = http.GET();
    Serial.println("HTTP Code: " + String(code));
    http.end();
  } else {
    Serial.println("WiFi mất kết nối!");
  }
}

void syncTimeWithNTP() {
  configTime(7 * 3600, 0, "pool.ntp.org", "time.google.com"); 
  Serial.println("Đang đồng bộ thời gian NTP...");

  int attempts = 0;
  while (attempts < 30) {
    time_t now;
    time(&now);
    if (now > 1700000000ULL) 
      timeSynced = true;
      bootTime = now;
      lcd.setCursor(0, 0); lcd.print("Da dong bo thoi");
      lcd.setCursor(0, 1); lcd.print("gian...");
      delay(2000);
      lcd.clear();
      Serial.println("Đã đồng bộ thời gian thành công!");
      Serial.printf("Thời gian hiện tại: %s", ctime(&now));
      return;
    }
    delay(1000);
    attempts++;
  }
  lcd.setCursor(0, 0); lcd.print("Khong the dong");
  lcd.setCursor(0, 1); lcd.print("bo thoi gian...");
  Serial.println("Không thể đồng bộ NTP sau 30s → dùng thời gian dự phòng");
  delay(2000);
  lcd.clear();
}

float calculateFee(time_t entryEpoch) {
  time_t now;
  time(&now);

  if (!timeSynced || now < 1700000000ULL) {
    unsigned long ms = millis() - (entryEpoch - bootTime) * 1000ULL;
    float days = ms / 86400000.0;
    int dayCount = ceil(days);
    if (dayCount < 1) dayCount = 1;
    return dayCount * 5000.0;
  }

  double seconds = difftime(now, entryEpoch);
  int days = ceil(seconds / 86400.0);
  if (days < 1) days = 1;
  return days * 5000.0;
}

void dispenseCard() {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Dang cap the...");
  lcd.setCursor(0, 1); lcd.print("Vui long lay the");
  tone(BUZZER_PIN, 2000, 300);
  servoDispense.write(90);
  delay(600);
  servoDispense.write(0);
  cardDispensed = true;
  waitingForCardTake = true;
  dispenseTime = millis();
}

String readCard(MFRC522 &rfid, uint8_t ssPin) {
  digitalWrite(SS_IN_PIN, HIGH);
  digitalWrite(SS_OUT_PIN, HIGH);
  digitalWrite(ssPin, LOW);
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    String id = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
      if (rfid.uid.uidByte[i] < 0x10) id += "0";
      id += String(rfid.uid.uidByte[i], HEX);
    }
    id.toUpperCase();
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    digitalWrite(ssPin, HIGH);
    return id;
  }
  digitalWrite(ssPin, HIGH);
  return "";
}

bool waitForPass(uint8_t pin, unsigned long timeout = 15000) {
  unsigned long start = millis();
  int idleState = digitalRead(pin);
  // chờ detect: trạng thái khác idle
  while (millis() - start < timeout) {
    int v = digitalRead(pin);
    if (v != idleState) {
      unsigned long start2 = millis();
      while (millis() - start2 < timeout) {
        if (digitalRead(pin) == idleState) {
          return true; // hoàn tất: đã đổi trạng thái rồi trả về trạng thái ban đầu -> coi là đã qua
        }
        delay(10);
      }
      return true; 
    }
    delay(10);
  }
  return false; 
}

void saveEntryTimes() {
  int addr = 0;
  uint8_t count = entryTimeEpoch.size();
  EEPROM.write(addr++, count);
  for (auto& pair : entryTimeEpoch) {
    String id = pair.first;
    if (id.length() != ID_LEN) continue; 
    for (int j = 0; j < ID_LEN; j++) {
      EEPROM.write(addr++, id[j]);
    }
    time_t t = pair.second;
    EEPROM.put(addr, t);
    addr += sizeof(time_t);
  }
  EEPROM.commit();
}

void loadEntryTimes() {
  int addr = 0;
  uint8_t count = EEPROM.read(addr++);
  for (uint8_t i = 0; i < count; i++) {
    char buf[ID_LEN + 1];
    for (int j = 0; j < ID_LEN; j++) {
      buf[j] = EEPROM.read(addr++);
    }
    buf[ID_LEN] = '\0';
    String id = String(buf);
    time_t savedTime;
    EEPROM.get(addr, savedTime);
    addr += sizeof(time_t);
    if (timeSynced) {
      entryTimeEpoch[id] = savedTime;
    } else {
      entryTimeEpoch[id] = bootTime;
    }
  }
}

void setup() {
  Serial.begin(115200);
    // LCD
  Wire.begin(25, 26); 
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Bai Do Xe");
  lcd.setCursor(0, 1);
  lcd.print("Dang khoi dong...");
  delay(2000);
  lcd.clear();
  // WiFi
  WiFi.begin(ssid, password);
  int wifiTry = 0;
  while (WiFi.status() != WL_CONNECTED && wifiTry < 20) {
    lcd.setCursor(0, 0);
    lcd.print("Dang ket noi");
    lcd.setCursor(0, 1);
    lcd.print("wifi...");
    delay(500);
    Serial.print(".");
    wifiTry++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    syncTimeWithNTP();  
  }
  else 
  {
    lcd.setCursor(0, 0);
    lcd.print("Chua duoc ket");
    lcd.setCursor(0, 1);
    lcd.print("noi wifi...");
    delay(5000);
    lcd.clear();
  }
  // RFID
  SPI.begin();
  pinMode(SS_IN_PIN, OUTPUT);
  pinMode(SS_OUT_PIN, OUTPUT);
  digitalWrite(SS_IN_PIN, HIGH);
  digitalWrite(SS_OUT_PIN, HIGH);
  rfidIn.PCD_Init();
  rfidOut.PCD_Init();
  // Servo
  servoIn.attach(SERVO_IN_PIN);
  servoOut.attach(SERVO_OUT_PIN);
  servoDispense.attach(SERVO_DISPENSE);
  servoIn.write(0);
  servoOut.write(0);
  servoDispense.write(0);
  // Input/Output
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  // Cấu hình cảm biến IR:
  pinMode(IR_IN_PIN, INPUT_PULLUP);
  pinMode(IR_OUT_PIN, INPUT); 
  EEPROM.begin(EEPROM_SIZE);
  loadEntryTimes(); 
  lcd.setCursor(0, 0); lcd.print("San sang!");
  lcd.setCursor(0, 1); lcd.print("Nhan nut lay the");
}

void loop() {
  // 1. Nhấn nút để nhả thẻ
  if (digitalRead(BUTTON_PIN) == LOW && !waitingForCardTake) {
    delay(20);
    if (digitalRead(BUTTON_PIN) == LOW) {
      dispenseCard();
      while (digitalRead(BUTTON_PIN) == LOW) delay(10);
    }
  }
  if (waitingForCardTake) {
    String card = readCard(rfidIn, SS_IN_PIN);
    if (card != "") {
      lcd.clear();
      lcd.print("Cam on quy khach!");
      lcd.setCursor(0, 1); lcd.print("Chuc vui ve!");
      digitalWrite(LED_PIN, HIGH);
      tone(BUZZER_PIN, 1000, 200);
      time_t now;
      time(&now);
      if (!timeSynced) now = bootTime;
      entryTimeEpoch[card] = now;
      sendToGoogleSheets(card, "Xe vao");
      saveEntryTimes();
      servoIn.write(90);

      bool passed = waitForPass(IR_IN_PIN, 15000);
      if (!passed) {
        unsigned long t0 = millis();
        while (millis() - t0 < 5000) {
          if (waitForPass(IR_IN_PIN, 100)) break;
        }
      }
      servoIn.write(0);
      digitalWrite(LED_PIN, LOW);
      cardDispensed = false;
      waitingForCardTake = false;
      delay(2000);
      lcd.clear();
      lcd.print("San sang!");
      lcd.setCursor(0, 1); lcd.print("Nhan nut lay the");
    }
    // Timeout 15s
    else if (millis() - dispenseTime > 15000) {
      lcd.clear();
      lcd.print("Qua thoi gian!");
      tone(BUZZER_PIN, 300, 1000);
      cardDispensed = false;
      waitingForCardTake = false;
      delay(2000);
      lcd.clear();
      lcd.print("San sang!");
      lcd.setCursor(0, 1); lcd.print("Nhan nut lay the");
    }
  }
  String cardOut = readCard(rfidOut, SS_OUT_PIN);
  if (cardOut != "") {
    auto it = entryTimeEpoch.find(cardOut);
    if (it != entryTimeEpoch.end()) {
      time_t timeIn = it->second;
      float fee = calculateFee(timeIn);
      lcd.clear();
      lcd.print("Tong tien:");
      lcd.setCursor(0, 1); lcd.print(fee); lcd.print(" VND");
      digitalWrite(LED_PIN, HIGH);
      tone(BUZZER_PIN, 1500, 300);
      sendToGoogleSheets(cardOut, "Xe ra", fee);
      servoOut.write(90);
      // CHỜ xe đi qua cảm biến IR_OUT_PIN
      bool passedOut = waitForPass(IR_OUT_PIN, 15000);
      if (!passedOut) {
        unsigned long t0 = millis();
        while (millis() - t0 < 5000) {
          if (waitForPass(IR_OUT_PIN, 100)) break;
        }
      }
      // ĐÓNG barie ra
      servoOut.write(0);
      digitalWrite(LED_PIN, LOW);
      entryTimeEpoch.erase(it); 
      saveEntryTimes(); 
      lcd.clear();
      lcd.print("Cam on quy khach!");
      lcd.setCursor(0, 1);
      lcd.print("Chuc vui ve!");
      delay(2000);
      lcd.clear();
      lcd.setCursor(0, 0); lcd.print("San sang! ");
      lcd.setCursor(0, 1); lcd.print("Nhan nut lay the");
    } else {
      lcd.clear();
      lcd.print("The khong hop le!");
      tone(BUZZER_PIN, 200, 1000);
      delay(2000);
      lcd.clear();
      lcd.setCursor(0, 0); lcd.print("San sang! ");
      lcd.setCursor(0, 1); lcd.print("Nhan nut lay the");
    }
  }
  delay(100);
}