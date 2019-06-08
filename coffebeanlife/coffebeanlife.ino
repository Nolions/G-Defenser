#include <Adafruit_MLX90614.h>
#include <ArduinoJson.h>
#include <max6675.h>

//difine pin
// Led Green
const int REDLedPin = 12;
const int GREENLedPin = 11;

// HC-05
 const byte BTpin = 5;

// Thermocouple MAX675
const int ThermMISO = 10;
const int ThermCS = 9;
const int ThermSCLK = 8;

// relay(SRD)
const int RelayPin = 4;

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
// 進/出豆 
const String ACTION_IN = "in"; // 進豆 
const String ACTION_OUT = "out"; // 出豆

int waitSec = 0;
int beansTemp = 0; // 豆溫
int stoveTemp = 0; // 爐溫
int envTemp = 0; // 環溫
int targetTemp = 0; //目標溫度
int nowTemp = 0; // 現在設定溫度(爐/豆溫)
int runTime = 0; //烘豆時間(秒)

int runModel;
int isAlertStatus = 0;
bool startRecieve = false;
bool relaySatus = false;
char val;
String recieveData = "";
String model = RUN_MODEL_MANUAL;

MAX6675 thermo(ThermSCLK, ThermCS, ThermMISO);
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

StaticJsonDocument<200> doc;

void setup() {
  Serial.print("setup..."); 
  Serial1.begin(9600);
  
  initPinMode();
  
  // 啟動 MLX90614
  mlx.begin(); 
  
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

    while(Serial1.available()) {
      startRecieve = true;
      val = Serial1.read();
      if (!isWhitespace(val) && val != '\n') {
        recieveData += val;
      }
      
      delay(20);
      waitSec -= 20;
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

    DeserializationError error = deserializeJson(doc, recieveData);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }

    String action = doc["action"];
    if(action == "status") {
      bool isStart = doc["start"];
      RelayLEDStatus(isStart);
      relaySatus = isStart;
      Serial.print("isStart:");
      Serial.println(isStart);
    } else if(action == "model") {
      bool isAuto = doc["auto"];
      if (isAuto) {
        model = RUN_MODEL_AUTO;
      } else {
        model = RUN_MODEL_MANUAL;
      }
      Serial.print("isAuto:");
      Serial.println(isAuto);
    } else if (action == "temp") {
      targetTemp = doc["target"];
      Serial.print("target temp:");
      Serial.println(targetTemp);
    }

    startRecieve = false;
    recieveData = "";
  }

  if (model == RUN_MODEL_AUTO) {
    nowTemp = beansTemp;
  } else {
    nowTemp = stoveTemp;
  }
  
  // 設定繼電器狀態
  setRelay(nowTemp);
  delay(waitSec);
}

/**
 * 指定pin腳模式(OUTPUT/INPUT)
 */
void initPinMode() {
  pinMode(GREENLedPin, OUTPUT);
  pinMode(REDLedPin, OUTPUT);
  pinMode(BuzzerPin, OUTPUT);
  pinMode(RelayPin, OUTPUT);
}

/**
 * 透過藍芽傳送溫度
 */
void bluetoothWrite(double bean, double stove, double env) {
//  Serial1.print("{\"b\":");
//  Serial1.print(bean);
//  Serial1.print(",\"s\":");
//  Serial1.print(stove);
//  Serial1.print(",\"e\":");
//  Serial1.print(env);
//  Serial1.print("}");

//  Serial1.println();

  StaticJsonDocument<200> doc;
  doc["b"] = bean;
  doc["s"] = stove;
  doc["e"] = env;
  serializeJson(doc, Serial1);
  Serial1.println("");
  
  delay(20);
}

/**
 * 從k-type取得溫度
 */
int getKTypeTemp() {
  return thermo.readCelsius();
}

/**
 * 從mlx90614取得目標溫度
 */
int getObjectTemp() {
  return mlx.readObjectTempC();
}

/**
 * 從mlx90614取得環境溫度
 */
int getAmbientTemp(){
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

/**
 * 設定Relay的行為
 */
void setRelay(double nowTemp) {
//  Serial.print("now:" );
//  Serial.println(nowTemp);
//  Serial.print("target:" );
//  Serial.println(targetTemp);
//  Serial.print("relaySatus:");
//  Serial.println(relaySatus);

  if (relaySatus && targetTemp > nowTemp) {
//    Serial.println("RelayPin ON");
    digitalWrite(RelayPin, HIGH);
  } else {
//    Serial.println("RelayPin OFF");
    digitalWrite(RelayPin, LOW);
  }
}

/**
 * Relay狀態指示燈控制
 */
void RelayLEDStatus(bool status) {
  if (status) {
    digitalWrite(REDLedPin, HIGH);
  } else {
    digitalWrite(REDLedPin, LOW);
  }
}

/**
 * 藍芽裝置啟動指定燈
 */
void BLELEDStartAction() {
  digitalWrite(GREENLedPin, HIGH);
}

/**
 * 藍芽連線狀態LED控制
 */
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
