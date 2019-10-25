#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

const char* ssid     = "Monojit_Airtel";
const char* password = "web2013techlink";

IPAddress ip(192,168,1, 52);
IPAddress gateway(192,168,1,1); // set gateway to match your network
IPAddress subnet(255, 255, 255, 0);

long duration;
int distance;
int high_level_addr = 20;
int low_level_addr = 40;
int master_control_addr = 3;
int high_level = 0;
int low_level  = 0;

#define TRIGGERPIN D1
#define ECHOPIN    D2
#define RESERVER_INPUT D6
#define PUMP_ON_PIN D5

int master_pump_on;
int pump_on_condition = 0;
int reserver_water_lavel = 0;
int water_lavel_count = 0;
// Set web server port number to 80
ESP8266WebServer server(80);

void setup() {

  //Initialize out pins
  pinMode(TRIGGERPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);
  pinMode(RESERVER_INPUT,INPUT);
  pinMode(PUMP_ON_PIN,OUTPUT);

  //Initialize Server
  server.on("/status", handleStatus);
  server.on("/processRequest",processRequest);
  server.on("/masterControl",masterControl);
  server.on("/waterLavel",waterLavel);
  server.on("/setTankHighLevel",setTankHighLevel);
  server.on("/setTankLowLevel",setTankLowLevel);
  server.begin();
  delay(500);
  
  //Trying to connect wifi router
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  //digitalWrite(2,HIGH);
  //digitalWrite(0,HIGH);
  digitalWrite(PUMP_ON_PIN,LOW);
  Serial.begin(9600);
  EEPROM.begin(512);
  //digitalWrite(2,LOW);
  /**OTA CODE START **/

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
/** OTA CODE END **/
}

void loop() {
  ArduinoOTA.handle();
  digitalWrite(TRIGGERPIN, LOW);  
  delayMicroseconds(3); 
  
  digitalWrite(TRIGGERPIN, HIGH);
  delayMicroseconds(12); 
  
  digitalWrite(TRIGGERPIN, LOW);
  duration = pulseIn(ECHOPIN, HIGH);
  distance= duration*0.034/2;
  //Serial.print(distance);
  //Serial.print("Distance: ");
  //Serial.println(distance);

  low_level  = EEPROM.read(low_level_addr);
  high_level = EEPROM.read(high_level_addr);
  master_pump_on = EEPROM.read(master_control_addr);
  if(master_pump_on == 255) {
    master_pump_on = 0;
  }
  Serial.println("Low Level: "+ String(low_level));
  Serial.println("High Level: "+ String(high_level));
  Serial.print(high_level);
  
  //get resever water lavel
  reserver_water_lavel = digitalRead(RESERVER_INPUT);

  //check is master pump status
  if(master_pump_on == 1) {
    if(pump_on_condition == 1) {
      digitalWrite(PUMP_ON_PIN,HIGH);
    }
    else {
      digitalWrite(PUMP_ON_PIN,LOW);
    }
  }
  if(distance > low_level && reserver_water_lavel == 0) {
    //Serial.print("IF Part");
    water_lavel_count++;
    if(water_lavel_count > 100) {
      pump_on_condition = 1;
    }
  }
  if(distance < high_level || reserver_water_lavel == 1) {
    //Serial.print("ELSE part");
    pump_on_condition = 0;
    water_lavel_count = 0;
  }
  //Serial.println("TESTing");
  //Serial.print("Count: ");
  //Serial.println(water_lavel_count);
  delay(1000);
  server.handleClient();
}

//This method is used for return pin status
void handleStatus() {
  String message = "";
  String post_data_string = "";
  post_data_string = server.arg("type");
  if(post_data_string == "PUMP") {
    message = "";
    char pin_status = '0';
    if(digitalRead(PUMP_ON_PIN) == 1) {
      pin_status = '1';
    }
    //pin_status = !digitalRead(post_data);
    message = "{'pump_on_status': ";
    message += pin_status;
    message += " }";
  }
  else {
    message = "";
    char pin_status = '0';
    if(digitalRead(RESERVER_INPUT) == 1) {
      pin_status = '1';
    }
    message = "{'reserver_status': ";
    message += pin_status;
    message += " }";
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain",message); 
}

//This method is used to On/Off pin as per request
void processRequest() {
  String message = "";
  if(server.arg("item_no") == "") {
    message = "{'err_msg' : 'Invalid Item No'}";
  }
  else {
    String post_data_string = "";
    char post_data_char[2];
    int post_data = 0;
    char pin_status = '0';
    post_data_string = server.arg("item_no");
    post_data_string.toCharArray(post_data_char,2);
    post_data = atoi(post_data_char);
    if(post_data == 1) {
      pump_on_condition = 1;
      digitalWrite(PUMP_ON_PIN,HIGH);
    }
    else {
      pump_on_condition = 0;
    }
    message = "{'success' : '1'}"; 
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", message);
}

//This method is used for permanent off/on master switch
void masterControl() {
  String message = "";
  if(server.arg("item_no") == "") {
    message = "{'err_msg' : 'Invalid Item No'}";
  }
  else {
    String post_data_string = "";
    char post_data_char[2];
    int post_data = 0;
    char pin_status = '0';
    post_data_string = server.arg("item_no");
    post_data_string.toCharArray(post_data_char,2);
    post_data = atoi(post_data_char);
    if(post_data == 1) {
      master_pump_on = 1;
    }
    else {
      master_pump_on = 0;
    }
    EEPROM.write(master_control_addr, master_pump_on);
    EEPROM.commit();
    message = "{'success' : '1'}"; 
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", message);
}

void waterLavel() {
  String message = "";
   message = "";
    message = "{'water_level': ";
    message += distance;
    message += " }";
    server.sendHeader("Access-Control-Allow-Origin", "*");
    
    server.send(200, "text/plain", message);
}

void setTankHighLevel() {
  String message = "";
  if(server.arg("level") == "") {
    message = "{'err_msg' : 'Invalid level'}";
  }
  else {
    String post_data_string = "";
    int post_data = 0;
    byte data;
    post_data_string = server.arg("level");
    post_data = post_data_string.toInt();
    data = (int) post_data;
    EEPROM.write(high_level_addr, data);
    EEPROM.commit();
    message = "{'success' : '1'}"; 
    Serial.println("Level : "+ String(post_data));
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", message);
}

void setTankLowLevel() {
  String message = "";
  if(server.arg("level") == "") {
    message = "{'err_msg' : 'Invalid level'}";
  }
  else {
    String post_data_string = "";
    int post_data = 0;
    byte data;
    post_data_string = server.arg("level");
    post_data = post_data_string.toInt();
    data = (int) post_data;
    EEPROM.write(low_level_addr, post_data);
    EEPROM.commit();
    message = "{'success' : '1'}"; 
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", message);
}
