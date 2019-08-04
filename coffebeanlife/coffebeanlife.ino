#include <Adafruit_MLX90614.h>
#include <ArduinoJson.h>
#include <max6675.h>

struct tempJob {
  int sec;
  int temp;
};
// 系統版本
const String VERSION = "1.0.0";

//difine const of pin
//  Buzzer
const int BUZZEER_PIN = 13;
// Led
const int RELAY_STATUS_PIN = 12; // Green LED
const int SYSTEM_STATUS_PIN = 11; // Red LED
// Thermocouple MAX675
const int THERM_SO_PIN = 10;
const int THERM_CS_PIN = 9;
const int THERM_SCK_PIN = 8;
// HC-05
const byte BLUETOOTH_STATEL_PIN = 5; // Yellow LED
// relay(SRD)
const int RelayPin = 4;

// Led Light Model
const int LED_MODEL_Start = 0;
const int LED_MODEL_BTConn = 1;
// Run Model
const int RUN_MODEL_MANUAL = 0; // 自動模式
const int RUN_MODEL_AUTO = 1; // 手動模式
// 進/出豆 
const int ACTION_IN = 1; // 進豆 
const int ACTION_OUT = 0; // 出豆

int beansTemp = 0; // 豆溫
int stoveTemp = 0; // 爐溫
int envTemp = 0; // 環溫
int nowTemp = 0; // 現在設定溫度(爐/豆溫)
int targetTemp = 0; //目標溫度

int runTime = 0; //烘豆時間(秒)
int waitSec = 0;

int model = RUN_MODEL_MANUAL;

MAX6675 thermo(THERM_SCK_PIN, THERM_CS_PIN, THERM_SO_PIN);
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

int isAlertStatus = 0;
bool startRecieve = false;
bool relaySatus = false;
bool inBean = false;

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
//  while (!Serial);
  Serial.println("version:" + VERSION);
  Serial.println("==============================setup=============================="); 
  
  digitalWrite(SYSTEM_STATUS_PIN, HIGH);
  alarmBeep(1, 50);

  initPinMode();

  // 啟動 MLX90614
  mlx.begin(); 
  
  delay(500);
}

void loop() {
  commend(serialRead());

  String recieveData = "";
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
    // 當確定藍芽順利連線時，逼一長音 
    if (isAlertStatus == 1) {
      // 第一次才會逼
      alarmBeep(1, 500);
    }
    isAlertStatus++;
    
    //  透過藍芽回傳溫度
    bluetoothWrite(beansTemp, stoveTemp, envTemp, runTime);

    recieveData = read();
  } else {
    // 未有藍芽Client裝置連線
    if (isAlertStatus > 0) {
      isAlertStatus = -1;
    }
    if (isAlertStatus == -1) {
       alarmBeep(2, 50);
    }
    isAlertStatus--;
    // 藍芽連線中斷時強制關閉relay
    relaySatus = false;
  }

  if(startRecieve){
    Serial.println("recieveData:" + recieveData);
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, recieveData);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());

      startRecieve = false;
      recieveData = "";
      return;
    }

    String action = doc["action"];
    if(action == "status") {
      runTime = 0;
      
      inBean = false;

      bool isStart = doc["start"];
      relaySatus = isStart;

      if (relaySatus) {
        digitalWrite(RELAY_STATUS_PIN, HIGH);
      } else {
        digitalWrite(RELAY_STATUS_PIN, LOW);
      }
      Serial.print("isStart:" + isStart);
    } else if(action == "model") {
      inBean = false;
      runTime = 0;
      
      bool isAuto = doc["auto"];
      if (isAuto) {
        model = RUN_MODEL_AUTO;
        Serial.print("isAuto:true");
      } else {
        model = RUN_MODEL_MANUAL;
        Serial.print("isAuto:false");
      }
    } else if (action == "temp") {
      targetTemp = doc["target"];
      Serial.print("target temp:" + targetTemp);
    } else if (action == "inBean") {
        runTime = 0;
        inBean = doc["in"];
        Serial.print("inBean:" + inBean);
    }

    startRecieve = false;
    recieveData = "";
  }

  setTarget(model);
  
  // 設定繼電器狀態
  setRelay(nowTemp);
  delay(waitSec);
}

/**
 * 指定pin腳模式(OUTPUT/INPUT)
 */
void initPinMode() {
  pinMode(RELAY_STATUS_PIN, OUTPUT);
  pinMode(SYSTEM_STATUS_PIN, OUTPUT);
  pinMode(BUZZEER_PIN, OUTPUT);
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

String serialRead() {
  String cmd = "";
  while(Serial.available() > 0) {
    char val = Serial.read();
    if (!isWhitespace(val) &&  val != '-' && val != '\n' && val != '\r') {
      cmd += val;
    }
    waitDelay(20);
  }
 
  return cmd; 
}

void commend(String cmd) {
  if (cmd == "v" || cmd == "version") {
    Serial.println("version:" + VERSION);
  }
}

String read() {
  String data = "";
  
  while(Serial1.available()) {
    startRecieve = true;
    char val = Serial1.read();
    if (!isWhitespace(val) && val != '\n') {
      data += val;
    }
    waitDelay(20);
  }
  return data;
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
    tone(BUZZEER_PIN, 1000, mSec);
    waitDelay(200);
  }
}

/**
 * 設定Relay的行為
 */
void setRelay(double nowTemp) {
  if (relaySatus && targetTemp > nowTemp) {
    digitalWrite(RelayPin, HIGH);
  } else {
    digitalWrite(RelayPin, LOW);
  }
}

/**
 * 藍芽連線是否建立
 */
bool isBTConn() {
  if ( digitalRead(BLUETOOTH_STATEL_PIN)==HIGH)  {
    return true;
  }
  return false;
}

/**
 * 設定溫度目標基準
 * ------------------
 * 自動(auto) ==> 豆溫, 
 */
void setTarget(int model) {
  // 設定溫度目標基準: auto ==> 豆溫, 
  if (model == RUN_MODEL_AUTO) {
    nowTemp = beansTemp;
    Serial.println("isAuto:true");
  } else if (model == RUN_MODEL_MANUAL) {
    nowTemp = stoveTemp;
    Serial.println("isAuto:false");
  }
} 

void waitDelay(int mSec) {
  delay(mSec);
  waitSec -= mSec;
}
