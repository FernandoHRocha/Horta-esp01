#include <DHT.h>

#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
#include <MySQL_Encrypt_Sha1.h>
#include <MySQL_Packet.h>

#include <ESP8266WiFi.h>
#include "environment.h"

#define MINUTE (60000UL)
#define DHT_PIN 2
#define DHT_TYPE DHT11

char INSERT_SQL[] = "INSERT INTO esp07207.ape207 (temperature, humidity) VALUES ('%.2f', '%.2f')";
char query[128];

float humi = 0;
float temp = 0;

DHT dht(DHT_PIN, DHT_TYPE);

IPAddress SERVER_IP(85, 10, 205, 173);

WiFiClient client;
MySQL_Connection conn((Client *)&client);

void setup() {
  Serial.begin(115200);

  Serial.println("Iniciando.");
  dht.begin();

  setupWifi();
}

void loop() {
  if ((WiFi.status() == WL_CONNECTED)) {
    getData();
    if(!isnan(humi) && !isnan(temp)) {
      sendData();
      WiFi.forceSleepBegin();
      delay(MINUTE * 5);
      WiFi.forceSleepWake();
    }
  } else {
    setupWifi();
  }
}

void setupWifi() {
  
  WiFi.begin(SSID, PASS);
  
  Serial.println("Conectando ao Wifi.");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  while(!conn.connect(SERVER_IP, 3306, USER_DB, PASS_DB)) {
    conn.close();
    delay(500);
    Serial.print(".");
  }
  Serial.println("Sucesso ao conectar com o banco de dados.");
}

void sendData() {
  sprintf(query, INSERT_SQL, temp, humi);
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  cur_mem->execute(query);
  delete cur_mem;
  Serial.println("Informações enviadas.");
}

void getData() {
  humi = dht.readHumidity();
  temp = dht.readTemperature();  
}

