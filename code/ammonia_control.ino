
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SD.h>
#include <Chrono.h>
#include <Streaming.h>
#include <avr/wdt.h>
#include <virtuabotixRTC.h>
#include <EEPROM.h>


#define BUZZER_PIN A1
#define GREEN_LED_PIN A2
#define RED_LED_PIN A3

#define SD_CS_PIN 10

#define RELAY_PIN 4

#define GAS_SENSOR_PIN A0

#define b 0.323
#define m -0.243
#define RL 47
#define Ro 140

LiquidCrystal_I2C lcd(0x27, 16, 2);

virtuabotixRTC myRTC(5, 6, 7);  //clk, dat, rst

File dataFile;

const long interval = 5000;
float ppm;

Chrono myChrono, chronoBuzzer, chronoBeep;

bool buzzerState = false;



int time1 = 0;
int time2 = 15;
int time3 = 30;
int time4 = 45;
int logDone = 0;

void setup() {

  wdt_disable();
  lcd.begin();
  lcd.backlight();

  lcd.setCursor(0, 0), lcd << F("    STARTING    ");
  lcd.setCursor(0, 1), lcd << F("      ACMCS     ");


  pinMode(GAS_SENSOR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT), GreenOff();
  pinMode(RED_LED_PIN, OUTPUT), RedOff();
  pinMode(BUZZER_PIN, OUTPUT), BuzzerOff();

  Serial.begin(9600);
  Serial.setTimeout(100);
  //myRTC.setDS1302Time(00, 10, 17, 3, 26, 4, 2023);

  if (!SD.begin(SD_CS_PIN)) {
    lcd.setCursor(0, 0), lcd << F("    SD ERROR    ");
    lcd.setCursor(0, 1), lcd << F("                ");
    while (true)
      ;
  }

  delay(5000);

  logDone = EEPROM.read(4);

  if (logDone != 0 && logDone != 1) {
    logDone = 0;
  }
  wdt_enable(WDTO_2S);
  lcd.clear();
}

void loop() {

  myRTC.updateTime();
  int minutes = myRTC.minutes;
  Serial << "time: " << myRTC.dayofmonth << "/" << myRTC.month << "/" << myRTC.year << " " << myRTC.hours << ":" << minutes << ":" << myRTC.seconds << "\t\t";
  ppm = getPPM();
  Serial << "ppm :: " << ppm << "\t\t"
         << "Blower :: " << digitalRead(RELAY_PIN) << "\t\t"
         << "Buzzer :: " << !digitalRead(BUZZER_PIN) << "\t\t"
         << "RED :: " << !digitalRead(RED_LED_PIN) << "\t\t"
         << "Green :: " << !digitalRead(GREEN_LED_PIN) << endl;


  if (minutes == time1 || minutes == time2 || minutes == time3 || minutes == time4) {
    if (logDone == 0) {
      logDone = 1;
      logIt();
      EEPROM.write(4, logDone);
    }
  } else {
    logDone = 0;
    EEPROM.write(4, logDone);
  }

  if (myChrono.hasPassed(1000)) {
    myChrono.restart();
    lcd.setCursor(0, 0), lcd << F("PPM VALUE : ") << ppm << "  ";
  }


  if (ppm > 20) {
    BlowerOn();
    RedOn();
    GreenOff();

    if (chronoBuzzer.hasPassed(60000)) {
      BuzzerOff();
    } else {
      if (chronoBeep.hasPassed(1000)) {
        chronoBeep.restart();
        if (buzzerState) {
          buzzerState = false;
          BuzzerOn();
        } else {
          buzzerState = true;
          BuzzerOff();
        }
      }
    }

    lcd.setCursor(0, 1), lcd << F("  WARNING HIGH  ");
  } else if (ppm < 15) {
    chronoBuzzer.restart();
    BlowerOff();
    GreenOn();
    RedOff();
    BuzzerOff();
    lcd.setCursor(0, 1), lcd << F("     SAFE       ");
  } else {
    chronoBuzzer.restart();
  }

  wdt_reset();
}
void logIt() {
  dataFile = SD.open(F("ppmlog.csv"), FILE_WRITE);
  if (dataFile) {
    dataFile << myRTC.dayofmonth << "/" << myRTC.month << "/" << myRTC.year << " " << myRTC.hours << ":" << myRTC.minutes << ":" << myRTC.seconds << F(",") << ppm << endl;
    dataFile.close();
  } else {
    Serial << "SD ERROR" << endl;
  }
}

void BuzzerOn() {
  digitalWrite(BUZZER_PIN, LOW);
}

void BuzzerOff() {
  digitalWrite(BUZZER_PIN, HIGH);
}

void BlowerOn() {
  digitalWrite(RELAY_PIN, HIGH);
}

void BlowerOff() {
  digitalWrite(RELAY_PIN, LOW);
}

void GreenOn() {
  digitalWrite(GREEN_LED_PIN, LOW);
}

void GreenOff() {
  digitalWrite(GREEN_LED_PIN, HIGH);
}

void RedOn() {
  digitalWrite(RED_LED_PIN, LOW);
}

void RedOff() {
  digitalWrite(RED_LED_PIN, HIGH);
}

double getPPM() {
  float sensor_volt = analogRead(GAS_SENSOR_PIN) / 1023.0 * 5.0;
  float RS_gas = ((5.0 * RL) / sensor_volt) - RL;
  float ratio = RS_gas / Ro;
  Serial << "ratio :: " << ratio << endl;
  float ppm_log = (log10(ratio) - b) / m;
  double ppm = pow(10, ppm_log);
  return ppm;
}
