#include "max6675.h"
#include <SoftwareSerial.h>
#include <Wire.h>

//difine pin
// HC-05
const byte BTpin = 9;
const int BTRXpin = 7;
const int BTTXpin = 8;
// Thermocouple MAX675
const int ThermDO = 4;
const int ThermCS = 5;
const int ThermCLK = 6;
//  Buzzer
const int BuzzerPin = 13;
// Led Green
const int LedPin = 11;
// relay(SRD)
const int RelayPin = 12;

// Led Light Model
const int LED_MODEL_Start = 0;
const int LED_MODEL_BTConn = 1;

MAX6675 thermo(ThermCLK, ThermCS, ThermDO);
SoftwareSerial BT(BTRXpin,BTTXpin);

int waitSec = 0;
double beansTemp = 0;
double stoveTemp = 0;
double envTemp = 0;

int isAlertStatus = 0;
bool startRecieve = false;
char val;
String recieveData = "";

void setup() {
  Serial.print("setup..."); 
  Serial.begin(9600);
  BT.begin(9600);
  pinMode(LedPin, OUTPUT);
  ledLight(LED_MODEL_Start);
  alarmBeep(1, 50);
  delay(500);
}

void loop() {
//  Serial.println( isAlertStatus); 
  recieveData = "";
  waitSec = 1000;
  
  beansTemp = getThermoTemp();

  if (isBTConn()) {
    if (isAlertStatus <= 0) {
      isAlertStatus = 1;
    }
    // 當確定藍芽順利連線時，逼兩短音，且LED連續閃爍兩次   
    if (isAlertStatus == 1) {
      // 第一次才會逼
      alarmBeep(1, 500);
    }
    isAlertStatus++;
    ledLight(LED_MODEL_BTConn);
    
    String jsonStr = genTempJSONStr(beansTemp, 0, 0);
    //  透過藍芽回傳溫度
    bluetoothWrite(jsonStr);
    
    while(BT.available()) {
      startRecieve = true;
      val = BT.read();
      if (!isWhitespace(val) && val != '\n') {
        recieveData += val;
      }
      
      delay(200);
      waitSec -= 200;
    }
  } else {
    if (isAlertStatus > 0) {
      isAlertStatus = -1;
    }
    if (isAlertStatus == -1) {
      ledLight(LED_MODEL_Start);
       alarmBeep(2, 50);
    }
    isAlertStatus--;
    setRelay(0);
  }

  if(startRecieve){
    Serial.println(recieveData);
    if (recieveData == "1\r") {
      Serial.println("1");
      setRelay(1);
    } else if (recieveData == "0\r") {
      Serial.println("0");
      setRelay(0);
    }
    
    startRecieve = false;
    recieveData = "";
  }

  delay(waitSec);
}

void bluetoothWrite(String data) {
  BT.println(data);
}

/**
 * 取得溫度
 */
double getThermoTemp() {
  return  thermo.readCelsius();
}

/**
 * 控制蜂鳴器
 */
void alarmBeep(int count, int mSec) {
  int i = 0;
  for (i; i < count; i++) {
    tone(BuzzerPin, 1000, mSec);
    delay(600);
  }
}

/**
 * 控制Led燈
 */
void ledLight(int model) {
  switch (model){
    case LED_MODEL_Start:
      LEDStartAction();
    break;
    case LED_MODEL_BTConn:
      LEDBTConnAction();
    break;
  }
}

void setRelay(int relayState) {
  digitalWrite(RelayPin, relayState);
}

void LEDStartAction() {
  digitalWrite(LedPin, HIGH);
}

void LEDBTConnAction() {
  digitalWrite(LedPin, LOW);
  delay(100);
  waitSec -=100;
  digitalWrite(LedPin, HIGH);
  delay(100);
  waitSec -=100;
  digitalWrite(LedPin, LOW);
  delay(100);
  waitSec -=100;
  digitalWrite(LedPin, HIGH);
}
/**
 * 藍芽連線是否建立
 */
bool isBTConn() {
  if ( digitalRead(BTpin)==HIGH)  {
    return true;
  }
  return false;
}

/**
 * 產生相關溫度JSON字串
 */
String genTempJSONStr(double beans, double stove, double env) {
  String jsonStr = "{\"b\":";
  jsonStr += String(beans,2);
  jsonStr += ",\"s\":";
  jsonStr += String(stove,2);
  jsonStr += ",\"e\":";
  jsonStr += String(env,2);
  jsonStr += "}";

  return jsonStr;
}
