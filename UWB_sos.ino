// 載入設定參數
#include "setting.h"

// 設定: 通知狀態
boolean sosStatus = false;

// 設定: Wifi
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

// 設定: IoT Server
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
ESP8266WebServer server(80);

// 設定: 按鈕
#include <Bounce2.h>
Bounce bouncer = Bounce(BUTTON_INPUT_PIN, 50);

// 設定: LCD
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

/*
  大片的用 LiquidCrystal_I2C lcd(0X27, 20, 4);
  小片的用 LiquidCrystal_I2C lcd(0X3F, 16, 2);
*/
LiquidCrystal_I2C lcd(0X3F, 16, 2);

/* LCD icons */
uint8_t bell[8]       = {0x4, 0xe, 0xe, 0xe, 0x1f, 0x0, 0x4};
uint8_t note[8]       = {0x2, 0x3, 0x2, 0xe, 0x1e, 0xc, 0x0};
uint8_t clocks[8]     = {0x0, 0xe, 0x15, 0x17, 0x11, 0xe, 0x0};
uint8_t heart[8]      = {0x0, 0xa, 0x1f, 0x1f, 0xe, 0x4, 0x0};
uint8_t duck[8]       = {0x0, 0xc, 0x1d, 0xf, 0xf, 0x6, 0x0};
uint8_t check[8]      = {0x0, 0x1 , 0x3, 0x16, 0x1c, 0x8, 0x0};
uint8_t cross[8]      = {0x0, 0x1b, 0xe, 0x4, 0xe, 0x1b, 0x0};
uint8_t retarrow[8]   = {0x1, 0x1, 0x5, 0x9, 0x1f, 0x8, 0x4};
uint8_t block[8]      = {0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f};
uint8_t half_block[8] = {0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c};

// 設定: 核心 API
#include <ESP8266HTTPClient.h>
WiFiClient client;
HTTPClient http;

// 抓目前時間
#include <ezTime.h>
Timezone myTZ;

void setup() {

  // 初始化: 按鈕
  pinMode(BUTTON_INPUT_PIN, INPUT);

  // 初始化: LCD
  lcd.begin();
  lcd.backlight();
  lcd.createChar(0, bell);
  lcd.createChar(1, note);
  lcd.createChar(2, clocks);
  lcd.createChar(3, heart);
  lcd.createChar(4, duck);
  lcd.createChar(5, check);
  lcd.createChar(6, cross);
  lcd.createChar(7, retarrow);
  lcd.createChar(8, block);
  lcd.createChar(9, half_block);
  lcd.home();
  lcd.clear();
  lcd.write(3);
  lcd.write(3);
  lcd.print(" System ON ");
  lcd.write(3);
  lcd.write(3);
  lcd.write(3);
  lcd.setCursor(0, 1);

  // 初始化: Wifi
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  lcd.write(1);
  lcd.print("Wifi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    lcd.setCursor(5, 1);
    lcd.write(6);
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  lcd.setCursor(5, 1);
  lcd.write(5);
  delay(1000);

  // 回報裝置 IP
  String request = String();
  request += "http://" + String(HOST_NAME) + String(API_URL) + "?";
  request += "region_id=" + String(REGION_ID);
  request += "&device_ip=" + WiFi.localIP().toString();

  if (http.begin(client, request)) {
    Serial.println("[HTTP] Request: " + request);
    int httpCode = http.GET();

    if (httpCode > 0) {
      Serial.printf("[HTTP] GET code: %d\n", httpCode);

      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        Serial.println(http.getString());
      }

    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();

  } else {
    Serial.printf("[HTTP} Unable to connect\n");
  }

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  // 初始化: IoT Server
  server.on("/", handleRoot);
  server.on("/turnOff", handleTurnOff);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
  lcd.write(1);
  lcd.print("IoT");
  lcd.write(5);
  delay(1000);

  // 初始化: 時區
  myTZ.setLocation(F("Asia/Taipei"));
  waitForSync();

  // 初始化: 蜂鳴器
  pinMode(BUZZ_OUTPUT_PIN, OUTPUT);
  //buzz_play();
  lcd.write(1);
  lcd.print("BUZ");
  lcd.write(5);
  delay(1000);

  // 初始化 OK, 重置 */
  delay(2000);
  initAll();
}


void loop() {

  /* 按下按鈕: 執行重置 */
  if (bouncer.update() && bouncer.read() == HIGH) {
    initAll();
    delay(100);
  }

  // 通知狀態為 true 時處理
  if (sosStatus) {

    /* 啟動蜂鳴器 */
    buzz_play();
    delay(1000);
  }

  // 每日定時重啟 (9:00 AM):
  if (myTZ.dateTime("H:i:s") == "09:00:00") {
    ESP.restart();
  }

  server.handleClient();
  MDNS.update();
}


// 重置
void initAll() {

  /* LCD */
  lcd.clear();
  lcd.home();
  lcd.noBacklight();

  /* 狀態 */
  sosStatus = false;

  /* 蜂鳴器 */
  noTone(BUZZ_OUTPUT_PIN);

  /* 預設顯示裝置 IP */
  lcd.home();
  lcd.print("IP address: ");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
}


// 蜂鳴器發出聲音
void buzz_play() {
  for ( uint8_t i = 0; i < 10; i++ ) {
    tone(BUZZ_OUTPUT_PIN, 1000);
    delay(50);
    tone(BUZZ_OUTPUT_PIN, 500);
    delay(50);
  }

  noTone(BUZZ_OUTPUT_PIN);
}


// IoT Server: 根目錄
void handleRoot() {

  /* 回傳 Client 的訊息: 0:失敗 | 1:成功 */
  String result = "0";

  /* 檢查是否 POST 呼叫 */
  if (server.method() != HTTP_POST) {
    server.send(200, "text/plain", result);

  } else {

    /* 檢查 POST 欄位是否存在 */
    boolean checkArgExist[2] = {false, false};

    for (uint8_t i = 0; i < server.args(); i++) {

      if (server.argName(i) == "uwb_index") {

        /* 手環編號 */
        checkArgExist[0] = true;

      } else if (server.argName(i) == "last_area") {

        /* 區域編號 */
        checkArgExist[1] = true;
      }
    }

    /* 狀態通知處理 */
    if (checkArgExist[0] && checkArgExist[1]) {

      /* 變更狀態 */
      sosStatus = true;

      /* 變更回傳 Client 訊息 */
      result = "1";

      /* 亮 LCD 背光 */
      lcd.backlight();

      /* 秀 LCD 訊息 */
      lcd.clear();
      lcd.home();

      for (uint8_t i = 0; i < server.args(); i++) {
        if (server.argName(i) == "uwb_index") {

          /* 手環編號 */
          lcd.home();
          lcd.write(1);
          lcd.print("UWB No: ");
          lcd.print(server.arg(i));

        } else if (server.argName(i) == "last_area") {

          /* 區域編號 */
          lcd.setCursor(0, 1);
          lcd.write(1);
          lcd.print("Area: ");
          lcd.print(server.arg(i));
        }
      }
    }

    /* 回傳 Client */
    server.send(200, "text/plain", result);
  }
}

// IoT Server: 員工 UWB 關閉通知
void handleTurnOff() {

  /* 回傳 Client 的訊息: 0:失敗 | 1:成功 */
  String result = "0";

  /* 檢查是否 POST 呼叫 */
  if (server.method() != HTTP_POST) {
    server.send(200, "text/plain", result);

  } else {

    /* 檢查 POST 欄位是否存在 */
    boolean checkArgExist[2] = {false, false};

    for (uint8_t i = 0; i < server.args(); i++) {

      if (server.argName(i) == "uwb_index") {

        /* 手環編號 */
        checkArgExist[0] = true;

      } else if (server.argName(i) == "last_area") {

        /* 區域編號 */
        checkArgExist[1] = true;
      }
    }

    /* 狀態通知處理 */
    if (checkArgExist[0] && checkArgExist[1]) {

      /* 重置 */
      initAll();

      /* 變更回傳 Client 訊息 */
      result = "1";
    }

    /* 回傳 Client */
    server.send(200, "text/plain", result);
  }
}

// IoT Server: 404
void handleNotFound() {

  /* 回傳 Client 訊息 */
  String result = "File Not Found\n\n";
  result += "URI: ";
  result += server.uri();
  result += "\nMethod: ";
  result += (server.method() == HTTP_GET) ? "GET" : "POST";
  result += "\nArguments: ";
  result += server.args();
  result += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    result += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  /* 回傳 Client */
  server.send(404, "text/plain", result);
}
