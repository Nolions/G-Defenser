#include "max6675.h"
#include <SoftwareSerial.h>
#include <Adafruit_MLX90614.h>
#include <Wire.h>

//difine pin
// Led Green
const int REDLedPin = 2;
const int GREENLedPin = 3;

// HC-05
const byte BTpin = 9;
const int BTRXpin = 7;
const int BTTXpin = 8;

// Thermocouple MAX675
const int ThermDO = 6;
const int ThermCS = 5;
const int ThermCLK = 4;

// relay(SRD)
const int RelayPin = 12;

//  Buzzer
const int BuzzerPin = 13;

// Led Light Model
const int LED_MODEL_Start = 0;
const int LED_MODEL_BTConn = 1;
// Run Model
const String RUN_MODEL_MANUAL = "m\r";
const String RUN_MODEL_AUTO = "a\r";
// Relay ready
const String RELAY_READY_OPEN = "o\r";
const String RELAY_READY_CLOSE = "c\r";


MAX6675 thermo(ThermCLK, ThermCS, ThermDO);
SoftwareSerial BT(BTRXpin,BTTXpin);
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

int waitSec = 0;
double beansTemp = 0;
double stoveTemp = 0;
double envTemp = 0;
double targetTemp = 0;
double nowTemp = 0;

int runModel;
int isAlertStatus = 0;
bool startRecieve = false;
bool relaySatus = false;
char val;
String recieveData = "";

void setup() {
  Serial.print("setup..."); 
  Serial.begin(9600);
  BT.begin(9600);
  mlx.begin(); 
  pinMode(GREENLedPin, OUTPUT);
  ledLight(LED_MODEL_Start);
  alarmBeep(1, 50);
  delay(500);
}

void loop() {
  recieveData = "";
  waitSec = 2000;

  // 取得溫度
  beansTemp = getKTypeTemp();
  stoveTemp = getObjectTemp();
  envTemp = getAmbientTemp();

  if (isBTConn()) {
    // 藍芽Client裝置連線
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
    
    //  透過藍芽回傳溫度
    bluetoothWrite(beansTemp, stoveTemp, envTemp);
    
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
    // 未有藍芽Client裝置連線
    if (isAlertStatus > 0) {
      isAlertStatus = -1;
    }
    if (isAlertStatus == -1) {
      ledLight(LED_MODEL_Start);
       alarmBeep(2, 50);
    }
    isAlertStatus--;
    // 
    relaySatus = false;
    RelayLEDStatus(false);
  }

  if(startRecieve){
    Serial.println(recieveData);
    
    if( recieveData == RUN_MODEL_MANUAL) {
      // 根據模式決定目標溫度判斷依據
      // 手動模式 => 豆溫
      Serial.println("Model:MANUAL" );
      nowTemp = beansTemp;
    } else if( recieveData == RUN_MODEL_AUTO) {
      // 根據模式決定目標溫度判斷依據
      // 自動模式 => 爐溫
      Serial.println("Model:AUTO" );
      nowTemp = stoveTemp;
    } else if (recieveData == RELAY_READY_OPEN) {
      Serial.println("RELAY Ready:YES" );
      RelayLEDStatus(true);
      relaySatus = true;
    } else if (recieveData == RELAY_READY_CLOSE) {
      Serial.println("RELAY Ready:NO" );
      RelayLEDStatus(false);
      relaySatus = false;
    } else {
      // 設定目標溫度
      targetTemp = recieveData.toInt();
    }

    // 設定繼電器狀態
    setRelay(nowTemp);
    
    startRecieve = false;
    recieveData = "";
  }

  delay(waitSec);
}

void bluetoothWrite(double bean, double stove, double env) {  
  BT.print("{\"b\":");
  BT.print(bean);
  BT.print(",\"s\":");
  BT.print(stove);
  BT.print(",\"e\":");
  BT.print(env);
  BT.print("}");
  BT.println();
  delay(20);
}

/**
 * 從k-type取得溫度
 */
double getKTypeTemp() {
  return  thermo.readCelsius();
}

/**
 * 從mlx90614取得目標溫度
 */
double getObjectTemp() {
  return mlx.readObjectTempC();
}

/**
 * 從mlx90614取得環境溫度
 */
double getAmbientTemp(){
  return mlx.readAmbientTempC();
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
      BLELEDStartAction();
    break;
    case LED_MODEL_BTConn:
      BLELEDBTConnAction();
    break;
  }
}

void setRelay(double nowTemp) {
  Serial.print("now:" );
  Serial.println(nowTemp);
  Serial.print("target:" );
  Serial.println(targetTemp);
  
  int isStart = HIGH;
  if (relaySatus && targetTemp > nowTemp) {
    digitalWrite(RelayPin, HIGH);
  } else {
    digitalWrite(RelayPin, LOW);
  }
}

void RelayLEDStatus(bool status) {
  if (status) {
    digitalWrite(REDLedPin, HIGH);
  } else {
    digitalWrite(REDLedPin, LOW);
  }
}

void BLELEDStartAction() {
  digitalWrite(GREENLedPin, HIGH);
}

void BLELEDBTConnAction() {
  digitalWrite(GREENLedPin, LOW);
  delay(100);
  waitSec -=100;
  digitalWrite(GREENLedPin, HIGH);
  delay(100);
  waitSec -=100;
  digitalWrite(GREENLedPin, LOW);
  delay(100);
  waitSec -=100;
  digitalWrite(GREENLedPin, HIGH);
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
