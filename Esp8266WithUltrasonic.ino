#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

const char* ssid     = "Monojit_Airtel";
const char* password = "web2013techlink";

IPAddress ip(192,168,1, 55);
IPAddress gateway(192,168,1,1); // set gateway to match your network
IPAddress subnet(255, 255, 255, 0);

long duration;
int distance;

int high_level_addr = 20;
int low_level_addr = 40;
int master_control_addr = 3;
int pump_controll_mode_address = 2;
int select_pump_address = 4;
int time_address= 50;
int server_ip_addr = 51;
int current_pump_address = 65;

int pump_on_request = 0;
int pump_off_request = 0;
int high_level = 0;
int low_level  = 0;
int pump_controll_mode = 0;
int select_pump = 0;
int pump_start_height = 0;
int last_pump_on = 1;
int current_selected_pump = 1;
int check_water_counter = 0;
int water_check_condition = 0;
long current_time = 0;
String pump_mode_condition = "";
String server_ip = "";

#define TRIGGERPIN 5
#define ECHOPIN    4
#define RESERVER_INPUT 12
#define PUMP_1_ON_PIN 14
#define PUMP_2_ON_PIN 13

int master_pump_on;
int pump_on_condition = 0;
int reserver_water_lavel = 0;
int water_lavel_count = 0;

int master_status = 0;
String condition = "";
// Set web server port number to 80
ESP8266WebServer server(80);

void setup() {

  //Initialize out pins
  pinMode(TRIGGERPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);
  pinMode(RESERVER_INPUT,INPUT);
  pinMode(PUMP_1_ON_PIN,OUTPUT);
  pinMode(PUMP_2_ON_PIN,OUTPUT);

  //Initialize Server
  server.on("/status", handleStatus);
  server.on("/setMasterControl",masterControl);
  server.on("/getMasterControl",getMasterControl);
  server.on("/waterLavel",waterLavel);
  server.on("/setTankHighLevel",setTankHighLevel);
  server.on("/setTankLowLevel",setTankLowLevel);
  server.on("/set-pump-controll-mode",setPumpControllMode);
  server.on("/selectPump",selectPump);
  server.on("/setServerIp",setServerIp);
  server.on("/manual-pump-on",manualPumpOn);
  server.on("/debug",debug);
  server.begin();
  delay(500);
  
  //Trying to connect wifi router
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  digitalWrite(PUMP_1_ON_PIN,LOW);
  digitalWrite(PUMP_2_ON_PIN,LOW);
  
  Serial.begin(9600);
  EEPROM.begin(512);
  eepromOperations();
  setupOTA();

}

void setupOTA() {
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
  eepromOperations();
  getWaterLevel();
  //get resever water lavel
  reserver_water_lavel = digitalRead(RESERVER_INPUT);
  pumpOnOffCondition();
  pumpOn();
  ArduinoOTA.handle();

  delay(1000);
  server.handleClient();
}

/********** Start Loop Function Block **********/

void selectPump() {
    String post_data_string;
    int post_data;
    char post_data_char[2];
    post_data_string = server.arg("select_pump");
    post_data_string.toCharArray(post_data_char,2);
    post_data = atoi(post_data_char);
    current_selected_pump = post_data;
    EEPROM.write(current_pump_address, current_selected_pump);
    EEPROM.commit();
    String message;
    message += "{\"success\" : \"1\"}";
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain",message); 
}

void pumpOnOffCondition() {
  if(distance >= low_level && reserver_water_lavel == 0) {
    condition = "IF Part";
    water_lavel_count++;
    if(water_lavel_count > 100) {
      pump_on_condition = 1;
    }
  }
  else if(distance <= high_level || reserver_water_lavel == 1) {
    condition = "ELSE Part";
    pump_on_condition = 0;
    water_lavel_count = 0;
  }
  else {
    condition = "";
    water_lavel_count = 0;
  }
}

void pumpOn() {
    //check is master pump status
  if(master_pump_on == 1) {
    if(pump_on_condition == 1) {
      check_water_counter++;
      //temp code
      master_status =1;
      if(pump_controll_mode == 1) {
        if(select_pump == 1) {
          Serial.println("Pump 1 is On");
          digitalWrite(PUMP_1_ON_PIN,HIGH);
          digitalWrite(PUMP_2_ON_PIN,LOW);
          if(pump_start_height == 0) {
            pump_start_height = distance;
          }
          if(pump_on_request == 0) {
            updatePumpStatus(1,pump_start_height);
            pump_on_request = 1;
            pump_off_request = 0;
          }
        }
        if(select_pump == 2) {
          Serial.println("Pump 2 is On");
          digitalWrite(PUMP_2_ON_PIN,HIGH);
          digitalWrite(PUMP_1_ON_PIN,LOW);
          if(pump_start_height == 0) {
            pump_start_height = distance;
          }
          if(pump_on_request == 0) {
            updatePumpStatus(1,pump_start_height);
            pump_on_request = 1;
            pump_off_request = 0;
          }
        }
      }
      if(pump_controll_mode == 2) {
        if(current_selected_pump == 1) {
          Serial.println("Pump 2 is On");
          digitalWrite(PUMP_2_ON_PIN,LOW);
          digitalWrite(PUMP_1_ON_PIN,HIGH);
          if(pump_start_height == 0) {
            pump_start_height = distance;
          }
          if(pump_on_request == 0) {
            updatePumpStatus(1,pump_start_height);
            pump_on_request = 1;
            pump_off_request = 0;
          }
        }
        else if(current_selected_pump == 2) {
          Serial.println("Pump 1 is On");
          digitalWrite(PUMP_1_ON_PIN,LOW);
          digitalWrite(PUMP_2_ON_PIN,HIGH);
          if(pump_start_height == 0) {
            pump_start_height = distance;
          }
          if(pump_on_request == 0) {
            updatePumpStatus(1,pump_start_height);
            pump_on_request = 1;
            pump_off_request = 0;
          }
        }
      }
    }
    else {
      digitalWrite(PUMP_1_ON_PIN,LOW);
      digitalWrite(PUMP_2_ON_PIN,LOW);
      pump_start_height = 0;
      check_water_counter = 0;
      if(pump_off_request == 0 && pump_on_request == 1) {
        updatePumpStatus(0,distance);
        pump_off_request = 1;
        pump_on_request = 0;
      }
    }
  }
}

void pumpSafety() {
    if(check_water_counter >= 100) {
    check_water_counter = 0;
    if(distance ==  pump_start_height || distance > pump_start_height || distance >= (pump_start_height-5)) {
      water_check_condition = 1;
      if(pump_controll_mode == 2) {
        pump_mode_condition = "Mode 2 Condition";
        if(last_pump_on == 1) {
          last_pump_on = 2;
          pump_mode_condition = "Mode 2 Condition Last Pump 1 Condition";
        }
        else if(last_pump_on == 2) {
          last_pump_on = 1;
          pump_mode_condition = "Mode 2 Condition Last Pump 2 Condition";
        }
      }
      else {
        pump_mode_condition = "Mode 1 Condition";
        pump_on_condition = 0;
        water_lavel_count = 0;
      }
    }
    else {
      water_check_condition = 0;
    }
  }
}

void getWaterLevel() {
  digitalWrite(TRIGGERPIN, LOW);  
  delayMicroseconds(3); 
  
  digitalWrite(TRIGGERPIN, HIGH);
  delayMicroseconds(12); 
  
  digitalWrite(TRIGGERPIN, LOW);
  duration = pulseIn(ECHOPIN, HIGH);
  int temp_distance = duration*0.034/2;
  if(temp_distance != 0) {
    distance= temp_distance; 
  }
}

void updatePumpStatus(int pump_status,int water_level) {
  WiFiClient client;
  HTTPClient http;
  String url;
  int httpCode;
  url = "http://" + server_ip + "/room/public/api/pump-status?status=" + String(pump_status) + "&water_level="+ String(water_level) + "&pump=" + String(current_selected_pump);
    if (http.begin(client, url)) {
      httpCode = http.GET();
      if(httpCode == 200) {
        Serial.println("Updated");    
      }
    }
}

/********** End Loop Function Block **********/

/********** Start Manual Function Block **********/

//This method is used for return pin status
void handleStatus() {
  String message = "";
  String post_data_string = "";
  post_data_string = server.arg("type");
  if(post_data_string == "PUMP") {
    message = "";
    char pin_status = '0';
    if(digitalRead(PUMP_1_ON_PIN) == 1) {
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

void waterLavel() {
  String message = "";
   message = "";
    message = "{'water_level': ";
    message += distance;
    message += " }";
    server.sendHeader("Access-Control-Allow-Origin", "*");
    
    server.send(200, "text/plain", message);
}

void manualPumpOn() {
  String message = "{\"success\" : \"1\"}";
  pump_on_condition = 1;
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", message);
}


/********** End Manual Function Block **********/

/********** Start Settings Function Block **********/

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
    message = "{\"success\" : \"1\"}"; 
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", message);
}

void getMasterControl() {
  String message = "";
  int master_control;
  message = "{\"success\" : \"1\",\"master_control\" : \"" + String(master_pump_on) + "\"}"; 
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

void setPumpControllMode() {
    String message = "";
  if(server.arg("pump_mode") == "" || server.arg("select_pump") == "") {
    message = "{'err_msg' : 'Invalid Data'}";
  }
  else {
    String post_pump_mode = "";
    String post_select_pump = "";
    int pump_mode = 0;
    int int_select_pump = 0;
    byte data;
    post_pump_mode = server.arg("pump_mode");
    pump_mode = post_pump_mode.toInt();

    post_select_pump = server.arg("select_pump");
    int_select_pump = post_select_pump.toInt();
    int pump_controll_mode_address = 2;
    EEPROM.write(pump_controll_mode_address, pump_mode);
    EEPROM.write(select_pump_address, int_select_pump);
    EEPROM.commit();
    message = "{'success' : '1'}"; 
  }
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", message);
}

void setServerIp() {
   String post_data_string = "";
   post_data_string = server.arg("ip");
   for (int i = 0; i<13; i++) {
   char c = post_data_string[i];
   EEPROM.put(server_ip_addr+i,c);
   EEPROM.commit();
  }
  String message;
  message = "{\"success\" : \"1\"}";
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain",message); 
}

/********** End Settings Function Block **********/

/********** Start EEPROM Function Block **********/

long EEPROMReadlong(long address) {
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);
 
  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}

void EEPROMWritelong(int address, long value) {
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);
 
  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
}

void eepromOperations() {
  pump_controll_mode = EEPROM.read(pump_controll_mode_address);
  select_pump = EEPROM.read(select_pump_address);
  if(pump_controll_mode == 255 || pump_controll_mode == 0) {
     pump_controll_mode = 1;
  }

  if(select_pump == 255 || pump_controll_mode == 0) {
    select_pump = 1;
  }


  current_selected_pump = EEPROM.read(current_pump_address);
  if(current_selected_pump == 255 || current_selected_pump == 0) {
    current_selected_pump = 1;
  }

  low_level  = EEPROM.read(low_level_addr);
  high_level = EEPROM.read(high_level_addr);
  master_pump_on = EEPROM.read(master_control_addr);
  if(master_pump_on == 255) {
    master_pump_on = 0;
  }

  pump_controll_mode = EEPROM.read(pump_controll_mode_address);
  select_pump = EEPROM.read(select_pump_address);
  if(pump_controll_mode == 255 || pump_controll_mode == 0) {
     pump_controll_mode = 1;
  }

  if(select_pump == 255 || pump_controll_mode == 0) {
    select_pump = 1;
  }

  // Get server ip
  int server_ip_length = 13;
  char server_ip_char[server_ip_length];
  for(int i = 0; i<server_ip_length; i++) {
      server_ip_char[i]= EEPROM.read(server_ip_addr+i);
  }
  server_ip_char[server_ip_length] = '\0';
  server_ip     = server_ip_char;
  
}

/********** End EEPROM Function Block **********/

/********** Start Debug Function Block **********/

void debug() {
  String message = "";
    message = "";
    message = "{\"Condition\": \"" + String(condition) + "\"";
    message +=",\"Counter\" : \"" + String(water_lavel_count) + "\"";
    message += ",\"Master Status\" : \"" + String(master_status) + "\"";
    message += ",\"Pump1 Pin Status\" : \"" + String(digitalRead(PUMP_1_ON_PIN)) + "\"";
    message +=",\"Pump On Condition\" : \"" + String(pump_on_condition) +"\"";
    message += ",\"Master Control\" : \"" + String(master_pump_on) + "\"";
    message += ",\"Pump2 Pin Status\" : \"" + String(digitalRead(PUMP_2_ON_PIN)) + "\"";
    message += ",\"Pump Controll Mode\" : \"" + String(pump_controll_mode) + "\"";
    message += ",\"Select Pump\" : \"" + String(select_pump) + "\"";
    message += ",\"Pump Start Height\" : \"" + String(pump_start_height) + "\""; 
    message += ",\"High Level\" : \"" + String(high_level) + "\"";
    message += ",\"Low Level\" : \"" + String(low_level) + "\""; 
    message += ",\"check_water_counter\" : \"" + String(check_water_counter) + "\"";
    message += ",\"pump_mode_condition\" : \"" + String(pump_mode_condition) + "\"";
    message += ",\"Water Check Condition\" : \"" + String(water_check_condition) + "\"";
    message += ",\"Current Time\" : \"" + String(current_time) +"\"";      
    message += ",\"Current Time +24h\" : \"" + String((current_time + 86400000)) +"\"";
    message += ",\"Millis\" : \"" + String(millis()) +"\"";
    message += ",\"Current Selected Pump\" : \"" + String(current_selected_pump) +"\"";
    message += ",\"Reserver Level\" : \"" + String(reserver_water_lavel) +"\"";
    message += ",\"Water Level\" : \"" + String(distance) +"\"";
    message += ",\"Server IP\" : \"" + server_ip +"\"";
    message += ",\"Pump On Request\" : \"" + String(pump_on_request) +"\"";
    message += ",\"Pump off Request\" : \"" + String(pump_off_request) +"\"";
    message += " }";
    
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", message);
}

/********** End Debug Function Block **********/
