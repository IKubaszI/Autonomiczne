#include <WiFi.h>
#include <ESPmDNS.h>
#include <NetworkUdp.h>
#include <ArduinoOTA.h>
#include <WebServer.h>
#include <Wire.h>
#include <SparkFun_VL53L5CX_Library.h>
#include <HTTPClient.h>

// --- LIDAR SETUP ---
SparkFun_VL53L5CX myImager;
VL53L5CX_ResultsData measurementData;
int imageResolution = 0;
int imageWidth = 0;
#define INT_PIN 10
volatile bool dataReady = false;

// --- KONFIGURACJA SIECI ---
const char *ssid = "Sniezny internet";
const char *password = "SzypkiSnieznyInternet1444!";

// --- PINY SILNIKÓW ---
#define BIN_2 1
#define BIN_1 2
#define PWMB 0
#define PWMA 5
#define AIN_1 3
#define AIN_2 4
#define SPECIAL_PIN 9

String lastInfo = "Sterowanie wyłączone";

// --- PRĘDKOŚĆ SILNIKÓW (0-255) ---
int motor_speed = 180;

// --- LOGIKA POJAZDU ---
enum Mode { REMOTE, AUTO, MODE_DISABLED };
Mode mode = MODE_DISABLED;

WebServer server(80);

// --- ZAPAMIĘTANIE IP SERWERA DO WYSYŁKI DANYCH ---
String serverIp = "";
const int serverPort = 3100;

// --- FUNKCJE STEROWANIA ---
void Forward() {
  if (mode == MODE_DISABLED) { Stop(); lastInfo = "Sterowanie wyłączone"; return; }
  digitalWrite(BIN_1, HIGH);
  digitalWrite(BIN_2, LOW);
  analogWrite(PWMB, motor_speed);
  digitalWrite(AIN_1, LOW);
  digitalWrite(AIN_2, HIGH);
  analogWrite(PWMA, motor_speed);
  lastInfo = "Jazda do przodu";
}

void Backward() {
  if (mode == MODE_DISABLED) { Stop(); lastInfo = "Sterowanie wyłączone"; return; }
  digitalWrite(BIN_1, LOW);
  digitalWrite(BIN_2, HIGH);
  analogWrite(PWMB, motor_speed);
  digitalWrite(AIN_1, HIGH);
  digitalWrite(AIN_2, LOW);
  analogWrite(PWMA, motor_speed);
  lastInfo = "Jazda do tyłu";
}

void TurnLeftRemote() {
  if (mode == MODE_DISABLED) { Stop(); lastInfo = "Sterowanie wyłączone"; return; }
  digitalWrite(BIN_1, LOW);
  digitalWrite(BIN_2, HIGH);
  analogWrite(PWMB, motor_speed);
  digitalWrite(AIN_1, LOW);
  digitalWrite(AIN_2, HIGH);
  analogWrite(PWMA, motor_speed);
  lastInfo = "Skręt w lewo (zdalnie)";
}

void TurnRightRemote() {
  if (mode == MODE_DISABLED) { Stop(); lastInfo = "Sterowanie wyłączone"; return; }
  digitalWrite(BIN_1, HIGH);
  digitalWrite(BIN_2, LOW);
  analogWrite(PWMB, motor_speed);
  digitalWrite(AIN_1, HIGH);
  digitalWrite(AIN_2, LOW);
  analogWrite(PWMA, motor_speed);
  lastInfo = "Skręt w prawo (zdalnie)";
}

void Stop() {
  digitalWrite(BIN_1, HIGH);
  digitalWrite(BIN_2, LOW);
  analogWrite(PWMB, 0);
  digitalWrite(AIN_1, LOW);
  digitalWrite(AIN_2, HIGH);
  analogWrite(PWMA, 0);
  lastInfo = (mode == MODE_DISABLED) ? "Sterowanie wyłączone" : "STOP";
}

// --- HANDLERY HTTP ---
void handleDrive() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  if (mode == MODE_DISABLED) {
    Stop();
    server.send(200, "text/plain", "Sterowanie wyłączone");
    return;
  }
  if (server.hasArg("dir") && mode == REMOTE) {
    String dir = server.arg("dir");
    if (dir == "forward") Forward();
    else if (dir == "backward") Backward();
    else if (dir == "left") TurnLeftRemote();
    else if (dir == "right") TurnRightRemote();
    else if (dir == "stop") Stop();
    server.send(200, "text/plain", lastInfo);
  } else {
    server.send(200, "text/plain", "Sterowanie wyłączone lub tryb autonomiczny");
  }
}

void handleSetMode() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  if (server.hasArg("mode")) {
    String m = server.arg("mode");
    if (m == "auto") { mode = AUTO; lastInfo = "Tryb autonomiczny"; }
    else if (m == "remote") { mode = REMOTE; lastInfo = "Tryb zdalny"; }
    else { mode = MODE_DISABLED; lastInfo = "Sterowanie wyłączone"; }
    Stop();
    lastInfo = (mode == AUTO) ? "Tryb autonomiczny" : (mode == REMOTE) ? "Tryb zdalny" : "Sterowanie wyłączone";
    server.send(200, "text/plain", lastInfo);
  } else {
    mode = MODE_DISABLED;
    Stop();
    lastInfo = "Sterowanie wyłączone";
    server.send(200, "text/plain", lastInfo);
  }
}

void handleInfo() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", lastInfo);
}

void handleSetSpeed() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  if (server.hasArg("val")) {
    int val = server.arg("val").toInt();
    if (val >= 0 && val <= 255) {
      motor_speed = val;
      lastInfo = "Ustawiono prędkość: " + String(val);
      server.send(200, "text/plain", lastInfo);
      return;
    }
  }
  server.send(400, "text/plain", "Błędna wartość");
}

void handleLidarData() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  String html = "<tr>";
  for (int x = 0; x < imageWidth; x++) {
    html += "<th>" + String(x) + "</th>";
  }
  html += "</tr>";

  if (myImager.getRangingData(&measurementData)) {
    for (int y = 0; y < imageWidth; y++) {
      html += "<tr>";
      for (int x = 0; x < imageWidth; x++) {
        int idx = x + y * imageWidth;
        html += "<td>" + String(measurementData.distance_mm[idx]) + "</td>";
      }
      html += "</tr>";
    }
  } else {
    html += "<tr><td colspan='8'>Błąd odczytu lidara</td></tr>";
  }
  server.send(200, "text/html", html);
}

void handleSetServer() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  if (server.hasArg("ip")) {
    serverIp = server.arg("ip");
    lastInfo = "Ustawiono IP serwera: " + serverIp;
    server.send(200, "text/plain", lastInfo);
  } else {
    server.send(400, "text/plain", "Brak parametru ip");
  }
}

// --- PRZERWANIE DLA LIDARA ---
void IRAM_ATTR interruptRoutine() {
  dataReady = true;
}

// --- AUTONOMIA: maszyna stanów ---
enum AutoState { AUTO_IDLE, AUTO_BACK, AUTO_TURN };
AutoState autoState = AUTO_IDLE;
unsigned long stateStart = 0;

// OBRÓT tylko po przeszkodzie, po obrocie od razu do przodu
void losowyObrot() {
  if (mode == MODE_DISABLED) { Stop(); lastInfo = "Sterowanie wyłączone"; return; }
  if (random(2) == 0) {
    TurnLeftRemote();
    lastInfo = "Obrót w lewo (autonomicznie, przeszkoda)";
    delay(3000);
  } else {
    TurnRightRemote();
    lastInfo = "Obrót w prawo (autonomicznie, przeszkoda)";
    delay(3000);
  }
  Forward();
}

void sendLidarToServer() {
  if (serverIp.length() < 7 || imageResolution == 0) return;
  String url = "http://" + serverIp + ":" + String(serverPort) + "/api/data";
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int minDist = 99999, maxDist = 0, avgDist = 0, cnt = 0;
  for (int i = 0; i < imageResolution; i++) {
    int d = measurementData.distance_mm[i];
    if (d > 0) {
      minDist = min(minDist, d);
      maxDist = max(maxDist, d);
      avgDist += d;
      cnt++;
    }
  }
  if (cnt > 0) avgDist /= cnt;
  String payload = "{\"yaw\":0,\"pitch\":0,\"roll\":0,\"distance\":" + String(avgDist) + "}";
  int httpResponseCode = http.POST(payload);
  http.end();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  ArduinoOTA.begin();

  pinMode(BIN_1, OUTPUT); digitalWrite(BIN_1, HIGH);
  pinMode(BIN_2, OUTPUT); digitalWrite(BIN_2, LOW);
  pinMode(PWMB, OUTPUT); analogWrite(PWMB, motor_speed);
  pinMode(AIN_1, OUTPUT); digitalWrite(AIN_1, LOW);
  pinMode(AIN_2, OUTPUT); digitalWrite(AIN_2, HIGH);
  pinMode(PWMA, OUTPUT); analogWrite(PWMA, motor_speed);

  pinMode(SPECIAL_PIN, OUTPUT);
  digitalWrite(SPECIAL_PIN, LOW);

  Wire.begin(8,7);
  Wire.setClock(200000);
  Serial.println("Inicjalizacja lidara...");
  if (myImager.begin() == false) {
    Serial.println(F("Sensor not found - check your wiring. Freezing"));
    while (1);
  }
  myImager.setResolution(8*8);
  imageResolution = myImager.getResolution();
  imageWidth = sqrt(imageResolution);
  myImager.setRangingFrequency(15);
  pinMode(INT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INT_PIN), interruptRoutine, FALLING);
  Serial.println(F("Interrupt pin configured."));
  myImager.startRanging();

  randomSeed(analogRead(0)); // zamiast 34, bo C3 nie ma GPIO34

  server.on("/drive", handleDrive);
  server.on("/setmode", handleSetMode);
  server.on("/info", handleInfo);
  server.on("/api/data", handleLidarData);
  server.on("/setspeed", handleSetSpeed);
  server.on("/setserver", handleSetServer);
  server.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();

  if (mode == MODE_DISABLED) {
    Stop();
    return;
  }

  if (mode == AUTO) {
    switch (autoState) {
      case AUTO_IDLE:
        if (dataReady) {
          dataReady = false;
          if (myImager.getRangingData(&measurementData)) {
            bool przeszkoda = false;
            for (int i = 0; i < imageResolution; i++) {
              if (measurementData.distance_mm[i] > 0 && measurementData.distance_mm[i] < 120) {
                przeszkoda = true;
                break;
              }
            }
            if (przeszkoda) {
              Backward();
              sendLidarToServer(); // Wysyłka danych tylko po przeszkodzie!
              stateStart = millis();
              autoState = AUTO_BACK;
              lastInfo = "Wykryto przeszkodę: cofanie";
            } else {
              Forward();
            }
          }
        }
        break;
      case AUTO_BACK:
        if (millis() - stateStart > 2000) {
          losowyObrot();
          stateStart = millis();
          autoState = AUTO_TURN;
          lastInfo = "Obrót po cofnięciu";
        }
        break;
      case AUTO_TURN:
        if (millis() - stateStart > 3000) {
          autoState = AUTO_IDLE;
          lastInfo = "Jazda do przodu po ominięciu";
        }
        break;
    }
  }
}