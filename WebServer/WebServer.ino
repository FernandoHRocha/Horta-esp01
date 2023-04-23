#include <NTPClient.h>

#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
#include <MySQL_Encrypt_Sha1.h>
#include <MySQL_Packet.h>

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include "WiFiManager.h"

#include "webpage.h"
#include "environment.h"

#define MINUTE (60000UL)

const long utcOffsetInSeconds = -10800;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

int lastDataHour = 0;
int lastDataMinute = 0;
int updateInterval = 1;

float wateringTime = 0.1;

int initialWateringTime = 19;
int finishWateringTime = 22;

std::unique_ptr<ESP8266WebServer> server;

IPAddress SERVER_IP(85, 10, 205, 173);

WiFiClient client;
MySQL_Connection conn((Client *)&client);

String idleState = "ocioso";
String wateringState = "regando";
String manualState = "manual";

String state = idleState;

int valve1 = 0;
int valve2 = 0;

String timestamp = "";
String temperature = "";
String humidity = "";
String moisture = "";

void setup() {
  Serial.begin(115200);
  setupWifi();
}

void setupWifi() {
  //reset saved settings
  //wifiManager.resetSettings();
  WiFiManager wifiManager;
  wifiManager.autoConnect();

  setupServer();
  timeClient.begin();
  timeClient.update();
  setupDatabaseConnection();
}

void setupServer() {
  server.reset(new ESP8266WebServer(WiFi.localIP(), 80));

  server->on("/", [](){
    server->send(200, "text/html", webpage);
  });

  server->on("/get_data", serverGetData);

  server->on("/operate_valve", serverOperateValve);

  server->begin();
  Serial.println("Servidor iniciado em: ");
  Serial.println(WiFi.localIP());
}

void setupDatabaseConnection() {
  while(!conn.connect(SERVER_IP, 3306, USER_DB, PASS_DB)) {
    conn.close();
    delay(500);
  }
  Serial.println("Conectado ao banco de dados.");
}

void manageUpdates() {
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();
  int lastData = (lastDataHour*60) + lastDataMinute;
  int currentTime = (currentHour*60) + currentMinute;
  if(lastData > currentTime){
    lastDataHour = currentHour;
    lastDataMinute = currentMinute;
    return;
  }
  if(currentTime >= (lastData + updateInterval)) {
    if(conn.connected()) {
      getData();
    } else {
      conn.close();
      setupDatabaseConnection();
    }
  }
}

void loop() {
  timeClient.update();
  server->handleClient();
  manageUpdates();
  handleBehavior();
  delay(500);
}

void changeState(String targetState) {
  if(targetState == idleState){
    closeValve(0);
    delay(MINUTE * wateringTime);
    getData();
  }
  if(targetState == manualState) {
    delay(MINUTE * wateringTime);
  }
  if(targetState == wateringState) {
    Serial.println("Iniciando a rega.");
    openValve(0);
    delay(MINUTE * wateringTime);
  }
  state = targetState;
}

void operateValve(int valve) {
  switch (valve) {
    case 1:
      if(valve1 == 1) {
        Serial.write(rel1OFF, sizeof(rel1OFF));
        valve1 = 0;
      } else {
        Serial.write(rel1ON, sizeof(rel1ON));
        valve1 = 1;
      }
      break;
    case 2:
      if(valve2 == 1) {
        Serial.write(rel2OFF, sizeof(rel2OFF));
        valve2 = 0;
      } else {
        Serial.write(rel2ON, sizeof(rel2ON));
        valve2 = 1;
      }
      break;
    default:
      break;
  }
}

void getData() {
  Serial.println("---");
  Serial.println("executing query");
  row_values *row = NULL;
  char* head_count[32];
    
  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  cur_mem->execute(SELECT_SQL);
  column_names *columns = cur_mem->get_columns();

  do {
    row = cur_mem->get_next_row();
    if (row != NULL) {
      timestamp = String(row->values[0]);
      temperature = String(row->values[1]);
      humidity = String(row->values[2]);
      moisture = String(row->values[3]);
    }
  } while (row != NULL);
  delete cur_mem;
  lastDataHour = timeClient.getHours();
  lastDataMinute = timeClient.getMinutes();
}

void serverGetData() {
  char response[500];
  DynamicJsonDocument doc(1024);
  doc["timestamp"] = timestamp;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["moisture"] = moisture;
  doc["valve1"] = String(valve1);
  doc["valve2"] = String(valve2);
  doc["state"] = state;

  serializeJson(doc, response);
  server->send(200, "text/json; charset=utf-8", response);
}

void serverOperateValve() {
  int valve = server->arg("valve").toInt();
  if(isnan(valve)) {
    server->send(304);
    return;
  }
  operateValve(valve);
  serverGetData();
  if(valve1 || valve2) {
    changeState(manualState);
  }
}

void closeValve(int valve) {
  if(valve == 1) {
    Serial.write(rel1OFF, sizeof(rel1OFF));
    valve1 = 0;
    return;
  }
  if(valve == 2) {
    Serial.write(rel2OFF, sizeof(rel2OFF));
    valve2 = 0;
    return;
  }
  delay(200);
  Serial.write(rel1OFF, sizeof(rel1OFF));
  delay(200);
  Serial.write(rel2OFF, sizeof(rel2OFF));
  valve1 = 0;
  valve2 = 0;
}

void openValve(int valve) {
  if(valve == 1) {
    Serial.write(rel1ON, sizeof(rel1ON));
    valve1 = 1;
    return;    
  }
  if(valve == 2) {
    Serial.write(rel2ON, sizeof(rel2ON));
    valve2 = 1;
    return;
  }
  delay(200);
  Serial.write(rel1ON, sizeof(rel1ON));
  delay(200);
  Serial.write(rel2ON, sizeof(rel2ON));
  valve1 = 1;
  valve2 = 1;
}

void handleBehavior() {
  if(state == idleState) {
    handleIdle();
    return;
  }
  if(state == manualState){
    changeState(idleState);
    return;
  }
  if(state = wateringState){
    Serial.println("Parando a rega.");
    changeState(idleState);
    return;
  }
}

void handleIdle() {
  int currentTime = timeClient.getHours();  
  if((moisture == "1") && (currentTime >= initialWateringTime) && (currentTime < finishWateringTime)) {
    changeState(wateringState);
  }
}








