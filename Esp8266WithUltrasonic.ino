#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

const char* ssid     = "Monojit_Airtel";
const char* password = "web2013techlink";

IPAddress ip(192,168,1, 55);
IPAddress gateway(192,168,1,1); // set gateway to match your network
IPAddress subnet(255, 255, 255, 0);

long duration;
int distance;
int high_level_addr = 20;
int low_level_addr = 40;
int high_level = 0;
int low_level  = 0;
int pump_controll_mode = 0;
int select_pump = 0;
int pump_controll_mode_address = 2;
int select_pump_address = 4;
int pump_start_height = 0;
int last_pump_on = 1;
int check_water_counter = 0;

#define TRIGGERPIN D1
#define ECHOPIN    D2
#define RESERVER_INPUT D6
#define PUMP_1_ON_PIN D5
#define PUMP_2_ON_PIN D7

int master_pump_on = 1;
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
  server.on("/processRequest",processRequest);
  server.on("/masterControl",masterControl);
  server.on("/waterLavel",waterLavel);
  server.on("/setTankHighLevel",setTankHighLevel);
  server.on("/setTankLowLevel",setTankLowLevel);
  server.on("/set-pump-controll-mode",setPumpControllMode);
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
  pump_controll_mode = EEPROM.read(pump_controll_mode_address);
  select_pump = EEPROM.read(select_pump_address);
  if(pump_controll_mode == 255 || pump_controll_mode == 0) {
     pump_controll_mode = 1;
  }

  if(select_pump == 255 || pump_controll_mode == 0) {
    select_pump = 1;
  }
  
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


  low_level  = EEPROM.read(low_level_addr);
  high_level = EEPROM.read(high_level_addr);

  Serial.println("Low Level: "+ String(low_level));
  Serial.println("High Level: "+ String(high_level));
  Serial.print(high_level);

  pump_controll_mode = EEPROM.read(pump_controll_mode_address);
  select_pump = EEPROM.read(select_pump_address);
  if(pump_controll_mode == 255 || pump_controll_mode == 0) {
     pump_controll_mode = 1;
  }

  if(select_pump == 255 || pump_controll_mode == 0) {
    select_pump = 1;
  }
  
  //get resever water lavel
  reserver_water_lavel = digitalRead(RESERVER_INPUT);
  if(check_water_counter >= 200) {
    check_water_counter = 0;
    if(distance ==  pump_start_height || distance > pump_start_height) {
      if(pump_controll_mode == 2) {
        if(select_pump == 1) {
          select_pump = 2;
        }
        if(select_pump == 2) {
          select_pump = 1;
        }
      }
      else {
        pump_on_condition = 0;
      }
    }
  }
  
  //check is master pump status
  if(master_pump_on == 1) {
    if(pump_on_condition == 1) {
      check_water_counter++;
      //temp code
      master_status =1;
      if(pump_controll_mode == 1) {
        if(select_pump == 1) {
          digitalWrite(PUMP_1_ON_PIN,HIGH);
          digitalWrite(PUMP_2_ON_PIN,LOW);
          if(pump_start_height == 0) {
            pump_start_height = distance;
          }
        }
        if(select_pump == 2) {
          digitalWrite(PUMP_2_ON_PIN,HIGH);
          digitalWrite(PUMP_1_ON_PIN,LOW);
          if(pump_start_height == 0) {
            pump_start_height = distance;
          }
        }
      }
      if(pump_controll_mode == 2) {
        if(last_pump_on == 1) {
          digitalWrite(PUMP_2_ON_PIN,HIGH);
          digitalWrite(PUMP_1_ON_PIN,LOW);
          if(pump_start_height == 0) {
            pump_start_height = distance;
          }
          last_pump_on = 2;
        }
        if(last_pump_on == 2) {
          digitalWrite(PUMP_1_ON_PIN,HIGH);
          digitalWrite(PUMP_2_ON_PIN,LOW);
          if(pump_start_height == 0) {
            pump_start_height = distance;
          }
          last_pump_on = 1;
        }
      }
      //digitalWrite(PUMP_1_ON_PIN,HIGH);
    }
    else {
      master_status = 2;
      digitalWrite(PUMP_1_ON_PIN,LOW);
      digitalWrite(PUMP_2_ON_PIN,LOW);
      pump_start_height = 0;
      check_water_counter = 0;
    }
  }
  if(distance > low_level && reserver_water_lavel == 0) {
    //Serial.print("IF Part");
    condition = "IF Part";
    water_lavel_count++;
    if(water_lavel_count > 100) {
      pump_on_condition = 1;
    }
  }
  if(distance < high_level || reserver_water_lavel == 1) {
    //Serial.print("ELSE part");
    condition = "ELSE Part";
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
      digitalWrite(PUMP_1_ON_PIN,HIGH);
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

void debug() {
  String message = "";
     message = "";
    message = "{\"Condition\": ";
    message += "\""+String(condition)+"\"";
    message +=",\"Counter\" :";
    message += "\""+String(water_lavel_count)+"\"";
    message += ",\"master_status\" :";
    message += "\""+String(master_status)+"\"";
    message += ",\"Pump1 Pin Status\" :";
    message += "\""+String(digitalRead(PUMP_1_ON_PIN))+"\"";
    message +=",\"Pump On Condition\" : ";
    message += "\""+String(pump_on_condition)+"\"";
    message += ",\"Master Control\" :";
    message += "\""+String(master_pump_on)+"\"";
    message += ",\"Pump2 Pin Status\" :";
    message += "\""+String(digitalRead(PUMP_2_ON_PIN))+"\"";
    message += ",\"Pump Controll Mode\" :";
    message += "\""+String(pump_controll_mode)+"\"";
    message += ",\"Select Pump\" :";
    message += "\""+String(select_pump)+"\"";
    message += ",\"Pump Start Height\" :";
    message += "\""+String(pump_start_height)+"\"";
    message += ",\"Last Pump On\" :";
    message += "\""+String(last_pump_on)+"\"";    
    message += " }";
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

