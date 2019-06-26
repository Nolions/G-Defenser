#include <Adafruit_MLX90614.h>
#include <ArduinoJson.h>
#include <max6675.h>

struct tempJob {
  int sec;
  int temp;
};

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
const String RUN_MODEL_MANUAL = "manual";
const String RUN_MODEL_AUTO = "auto";
// Relay ready
//const String RELAY_READY_OPEN = "o\r";
//const String RELAY_READY_CLOSE = "c\r";
// 進/出豆 
//const String ACTION_IN = "in"; // 進豆 
//const String ACTION_OUT = "out"; // 出豆

int beansTemp = 0; // 豆溫
int stoveTemp = 0; // 爐溫
int envTemp = 0; // 環溫

int nowTemp = 0; // 現在設定溫度(爐/豆溫)
int targetTemp = 0; //目標溫度

int runTime = 0; //烘豆時間(秒)
int waitSec = 0;

int addSec = 0;

//int runModel;
int isAlertStatus = 0;
bool startRecieve = false;
bool relaySatus = false;
char val;
String recieveData = "";
String model = RUN_MODEL_MANUAL;

MAX6675 thermo(ThermSCLK, ThermCS, ThermMISO);
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

tempJob jobs[5];

int jobsNum = 0;
bool inBean = false;

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
  waitSec = 1000;
  
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
    bluetoothWrite(beansTemp, stoveTemp, envTemp, runTime);

    while(Serial1.available()) {
      startRecieve = true;
      val = Serial1.read();
      if (!isWhitespace(val) && val != '\n') {
        recieveData += val;
      }
      waitDelay(20);
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
      jobsNum = 0;
      runTime = 0;
      
      inBean = false;

      bool isStart = doc["start"];
      RelayLEDStatus(isStart);
      relaySatus = isStart;

//      if (isStart) {
//        addSec = 2;
//      }
      
      Serial.print("isStart:");
      Serial.println(isStart);
    } else if(action == "model") {
      jobsNum = 0;
      inBean = false;
      runTime = 0;
      
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
    } else if (action == "jobs") {
      int sec = doc["sec"];
      int temp = doc["temp"];
      struct tempJob job;
      job.sec = sec;
      job.temp = temp;
      jobs[jobsNum++] = job;
    } else if (action == "inBean") {
        runTime = 0;
        inBean = doc["in"];
        Serial.print("inBean:");
        Serial.println(inBean);
    }

    startRecieve = false;
    recieveData = "";
  }


  for (int i =0; i<5; i++) {
      struct tempJob job = jobs[i];

      if (model == RUN_MODEL_AUTO && inBean) {
          if (runTime+2 == job.sec || runTime-2 == job.sec || runTime+1 == job.sec || runTime-1 == job.sec) {
            targetTemp = job.temp;
            Serial.println(targetTemp);
          }
      }
  }

  if (model == RUN_MODEL_AUTO) {
    nowTemp = beansTemp;
  } else {
    nowTemp = stoveTemp;
  }
  
  // 設定繼電器狀態
  setRelay(nowTemp);
  runTime += addSec;
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
void bluetoothWrite(double bean, double stove, double env, int sec) {
  StaticJsonDocument<200> doc;
  doc["b"] = bean;
  doc["s"] = stove;
  doc["e"] = env;
  doc["t"] = sec;
  serializeJson(doc, Serial1);
  Serial1.println("");

  waitDelay(20);
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
    waitDelay(200);
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
  waitDelay(100);
  digitalWrite(GREENLedPin, HIGH);
  waitDelay(100);
  digitalWrite(GREENLedPin, LOW);
  waitDelay(100);
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

/**
 * 印出訊息
 */
void print(String data, bool isEOF) {
  if (isEOF) {
    Serial.println(data);
  } else {
    Serial.print(data);
  }
}

void waitDelay(int mSec) {
  delay(mSec);
  waitSec -= mSec;
}
