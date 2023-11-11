#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <DHT.h>
#include <ArduinoJson.h>
#define DHTPIN D2 //04
#define DHTTYPE DHT11
#define Soil_Moisture A0 //17
#define Pump D8 //15
//Object untuk DHT
DHT dht(DHTPIN, DHTTYPE);

const char *ssid = "siaomi";
const char *password = "12kilogram";
const char *host = "https://monitoring-tanaman.glitch.me";

BearSSL::WiFiClientSecure client;
HTTPClient https;

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Try to connect wifi");
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  delay(500);
  pinMode(Pump, OUTPUT);
  Serial.begin(115200);
  dht.begin();
  client.setInsecure();
  connectWiFi();
}

void postStatus() {
  //KIRIM STATUS PERANGKAT
  String Status = "Online";
  postRequest("/status_perangkat", Status);
}

void postData() {
  //KIRIM DATA SENSOR DHT
  float temperature = dht.readTemperature();
  int humidity = dht.readHumidity();
  int soil = 0 ; //(100.00 - (analogRead(Soil_Moisture) / 10.23))*2;

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  String data = "dataSuhu=" + String(temperature) + "&dataKelembaban=" + String(humidity) + "%" + "&dataTanah=" + String(soil) + "%";
  postRequest("/dataSensor", data);
}

void postRequest(const char *path, String data) {
  String url = String(host) + path;
  if (https.begin(client, url.c_str())) {
    https.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpCode = https.POST(data);
    if (httpCode > 0) {
      String response = https.getString();
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, response);
      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
      } else {
        Serial.print("HTTP Response code: ");
        Serial.println(httpCode);
        Serial.println(doc["message"].as<char*>());
        Serial.println(" ");
      }
    } else {
      Serial.print("Gagal post HTTP request :");
      Serial.println(httpCode);
    }
    https.end();
  } else {
    Serial.println("Unable to connect web server");
  }
}

void getInstruksi () {
  //MINTA DATA INSTRUKSI PENYIRAMAN
  if (https.begin(client, "https://monitoring-tanaman.glitch.me/status")) {
    int httpCode = https.GET();
    if (httpCode > 0) {
      String runPump = https.getString();
      StaticJsonDocument<100> doc;
      DeserializationError error = deserializeJson(doc, runPump);

      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
      } else {
        if (String(doc["status"].as<char*>()) == "True") {
          Serial.println("Penyiraman Sedang Dilakukan");
          digitalWrite(Pump, HIGH);
          delay(1000);
          digitalWrite(Pump, LOW);

          https.begin(client, "https://monitoring-tanaman.glitch.me/delete_status");
          https.GET();
          String alertSiram = https.getString();
          StaticJsonDocument<100> doc;
          DeserializationError error = deserializeJson(doc, alertSiram);

          if (error) {
            Serial.print("deserializeJson() failed: ");
            Serial.println(error.c_str());
          } else {
            Serial.println(doc["message"].as<char*>());
          }
        }
      }
    }
  }
  else {
    Serial.println("Unable to connect web server");
  }
}
//--------------------------------------------------------------
// ===================== PROGRAM UTAMA ========================
//--------------------------------------------------------------

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }else{
  postStatus();
  getInstruksi();
  postData();
  delay(1000); }
}
