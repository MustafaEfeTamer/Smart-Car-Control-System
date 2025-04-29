#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define BUTTON_MOTOR 22
#define BUTTON_BELT 23     
#define SWITCH_DOOR 24         
#define RED_LED 26
#define BUZZER 29

#define IN1 37
#define IN2 36
#define EN1 44

#define LDR_PIN A0
#define FAR_LED 27

#define LED_RED_RGB 28     
#define LED_BLUE_RGB    29     

#define POT_FUEL A2
#define LED_FUEL_YELLOW 25

#define LM35_PIN A3           
#define FAN_IN1 34            
#define FAN_IN2 35            
#define FAN_EN 45             

LiquidCrystal_I2C lcd(0x27, 16, 2);

bool pressedMotor = false;
bool activeMotor = false;
bool lampOpen = false;
bool beltIsOn = false;             
bool buttonBeforeBelt = HIGH;
bool doorOpenBefore = false;
bool beltWarningActive = false;
bool doorWarningActive = false;

// LCD mesaj kuyruğu için struct yapısı kurdum
struct LCDMesaj {
  String line1;
  String line2;
};
LCDMesaj lcdMesajlar[6];
int lcdMessageCount = 0;
int lcdMesajIndex = 0;
unsigned long lcdPassTime = 0;
const unsigned long lcdWaitingTime = 1000;

void lcdMesajEkle(String s1, String s2) {
  if (lcdMessageCount < 6) {
    lcdMesajlar[lcdMessageCount].line1 = s1;
    lcdMesajlar[lcdMessageCount].line2 = s2;
    lcdMessageCount++;
  }
}

void showLcdMessage() {
  if (lcdMessageCount == 0) return;

  unsigned long current = millis();
  if (current - lcdPassTime >= lcdWaitingTime) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(lcdMesajlar[lcdMesajIndex].line1);
    lcd.setCursor(0, 1);
    lcd.print(lcdMesajlar[lcdMesajIndex].line2);

    lcdMesajIndex = (lcdMesajIndex + 1) % lcdMessageCount;
    lcdPassTime = current;
  }
}

void setup() {
  pinMode(BUTTON_MOTOR, INPUT_PULLUP);
  pinMode(BUTTON_BELT, INPUT_PULLUP);  
  pinMode(SWITCH_DOOR, INPUT_PULLUP);     
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(FAR_LED, OUTPUT);
  pinMode(LED_RED_RGB, OUTPUT);
  pinMode(LED_BLUE_RGB, OUTPUT);
  pinMode(LED_FUEL_YELLOW, OUTPUT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(EN1, OUTPUT);

  pinMode(FAN_IN1, OUTPUT);
  pinMode(FAN_IN2, OUTPUT);
  pinMode(FAN_EN, OUTPUT);

  lcd.init();
  lcd.backlight();

  digitalWrite(RED_LED, LOW);
  digitalWrite(BUZZER, LOW);
  digitalWrite(LED_RED_RGB, HIGH);
  digitalWrite(LED_BLUE_RGB, HIGH);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  analogWrite(EN1, 0);
  digitalWrite(FAN_IN1, LOW);
  digitalWrite(FAN_IN2, LOW);
  analogWrite(FAN_EN, 0);
}

void loop() {
  lcdMessageCount = 0;

  bool buttonSitutation = digitalRead(BUTTON_MOTOR) == LOW;
  bool doorOpen = digitalRead(SWITCH_DOOR) == HIGH;
  bool beltButtonSitutation = digitalRead(BUTTON_BELT);

  // Emniyet kemeri durumunu algılamak için
  if (beltButtonSitutation == LOW && buttonBeforeBelt == HIGH) {
    delay(50);
    beltIsOn = !beltIsOn;
  }
  buttonBeforeBelt = beltButtonSitutation;

  // Kapı açık durumunun değişimi için
  if (doorOpen != doorOpenBefore) {
    doorOpenBefore = doorOpen;
    lcd.clear(); // kapı durumu değişirse LCD'yi temizliyoruz
    lcdMessageCount = 0;
    lcdMesajIndex = 0;
  }

  if (doorOpen) {
    digitalWrite(LED_RED_RGB, LOW);
    digitalWrite(LED_BLUE_RGB, LOW);
    digitalWrite(BUZZER, HIGH);
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    analogWrite(EN1, 0);
    activeMotor = false;

    lcdMesajEkle("Uyari: Kapi Acik", "Motor Calismaz!");
  } else {
    digitalWrite(LED_RED_RGB, HIGH);
    digitalWrite(LED_BLUE_RGB, HIGH);
    digitalWrite(BUZZER, LOW);

    if (buttonSitutation && !pressedMotor) {
      pressedMotor = true;
      if (!beltIsOn) {
        digitalWrite(RED_LED, HIGH);
        digitalWrite(BUZZER, HIGH);
        activeMotor = false;
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, LOW);
        analogWrite(EN1, 0);

        lcd.clear(); // Emniyet kemeri uyarısı geldiğinde LCD'yi temizliyoruz
        lcdMessageCount = 0;
        lcdMesajIndex = 0;
        lcdMesajEkle("Emniyet Kemeri", "Takili Degil!");
        beltWarningActive = true;
      } else {
        digitalWrite(RED_LED, LOW);
        digitalWrite(BUZZER, LOW);
        activeMotor = true;
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
        analogWrite(EN1, 200);
      }
    }

    if (!buttonSitutation) pressedMotor = false;

    if (beltIsOn && beltWarningActive) {
      lcd.clear(); // Kemer takılınca LCD'yi temizliyoruz
      lcdMessageCount = 0;
      lcdMesajIndex = 0;
      beltWarningActive = false;
    }

    if (beltIsOn && !activeMotor) {
      digitalWrite(RED_LED, LOW);
      digitalWrite(BUZZER, LOW);
    }

    if (activeMotor) {
      lcdMesajEkle("Motor Calisiyor", "");

      int ldrDeger = analogRead(LDR_PIN);
      if (ldrDeger <= 250) {
        digitalWrite(FAR_LED, HIGH);
        lcdMesajEkle("Farlar Acik", "");
      } else {
        digitalWrite(FAR_LED, LOW);
        lcdMesajEkle("Farlar Kapandi", "");
      }

      int analogValue = analogRead(LM35_PIN);
      float voltage = analogValue * (5.0 / 1023.0);
      float temperatureC = voltage * 100.0;

      if (temperatureC > 25.0) {
        digitalWrite(FAN_IN1, HIGH);
        digitalWrite(FAN_IN2, LOW);
        analogWrite(FAN_EN, 200);
        lcdMesajEkle("Sicaklik: " + String(temperatureC, 1) + "C", "Klima Acildi");
      } else {
        digitalWrite(FAN_IN1, LOW);
        digitalWrite(FAN_IN2, LOW);
        analogWrite(FAN_EN, 0);
        lcdMesajEkle("Sicaklik: " + String(temperatureC, 1) + "C", "Klima Kapali");
      }
    }

    int potValue = analogRead(POT_FUEL);
    int percentFuel = map(potValue, 0, 1023, 0, 100);

    if (percentFuel <= 0) {
      digitalWrite(LED_FUEL_YELLOW, LOW);
      digitalWrite(RED_LED, LOW);
      digitalWrite(LED_RED_RGB, HIGH);
      digitalWrite(LED_BLUE_RGB, HIGH);
      digitalWrite(FAR_LED, LOW);
      digitalWrite(BUZZER, LOW);
      digitalWrite(IN1, LOW);
      digitalWrite(IN2, LOW);
      analogWrite(EN1, 0);
      activeMotor = false;

      lcdMesajEkle("Yakit Bitti!", "Motor Durdu");
    } else if (percentFuel < 5) {
      digitalWrite(LED_FUEL_YELLOW, millis() % 500 < 250);
      lcdMesajEkle("Kritik: Yakit Az", "Seviye: %" + String(percentFuel));
    } else if (percentFuel < 10) {
      digitalWrite(LED_FUEL_YELLOW, HIGH);
      lcdMesajEkle("Uyari: Yakit Dusuk", "Seviye: %" + String(percentFuel));
    } else {
      digitalWrite(LED_FUEL_YELLOW, LOW);
    }
  }

  showLcdMessage();
  delay(100);
}