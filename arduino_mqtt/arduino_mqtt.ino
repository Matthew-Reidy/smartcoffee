#include <Arduino_JSON.h>
#include <ArduinoBearSSL.h>
#include <ArduinoECCX08.h>
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h> 
#include "secrets.h"


WiFiClient    wifiClient;            // Used for the TCP socket connection
BearSSLClient sslClient(wifiClient); // Used for SSL/TLS connection, integrates with ECC508
MqttClient    mqttClient(sslClient);

unsigned long brewBeginTime = 0;
const int relay_pin = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!ECCX08.begin()) {
    Serial.println("No ECCX08 present!");
    while (1);
  }
  
  pinMode(relay_pin, OUTPUT);

  digitalWrite(relay_pin, HIGH);

  ArduinoBearSSL.onGetTime(getTime);

  sslClient.setEccSlot(0, SECRET_CERTIFICATE);

  mqttClient.onMessage(onMessageReceived);
}

void connectWiFi() {

  Serial.println("Attempting to connect to SSID: ");
  Serial.println(WIFI_SSID);
  
  int status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (status != WL_CONNECTED) {
    // failed, retry
    Serial.println("attempting...");
    delay(5000);
  }
 
  Serial.println("You're connected to the network");
}

void connectMQTT() {

  Serial.println("Attempting to connect to MQTT broker: ");
  Serial.println(IOT_BROKER);
  Serial.println(" ");

  while (!mqttClient.connect(IOT_BROKER, 8883)) {
    Serial.print("failed to connect to MQTT broker");
    Serial.print(mqttClient.connectError());
    Serial.println(" ");
    delay(5000);
  }

  Serial.println();
  Serial.println("You're connected to the MQTT broker");
  Serial.println();

  //topic that handles brew
  mqttClient.subscribe("coffee/brew");
  //topic that handles confirmation that arduino is connected to iot core 
  mqttClient.subscribe("ping/pong");
}

unsigned long getTime() {
  return WiFi.getTime();
}

void onMessageReceived(int messageSize) {

  String topic = mqttClient.messageTopic();
  Serial.println(topic);

  char buf[messageSize + 1];

  mqttClient.readBytes(buf, messageSize);

  buf[messageSize] = '\0';

  JSONVar payload = JSON.parse(buf);

  Serial.println(payload);
  Serial.println(payload["message"]);

  if(payload.hasOwnProperty("message")){
    
    if(topic == "coffee/brew"){

        if(strcmp((const char*)payload["message"], "start") == 0){
          digitalWrite(relay_pin, LOW);
          brewBeginTime = millis();
          return;
        }

        digitalWrite(relay_pin, HIGH);
        brewBeginTime = 0;
    }


    if(topic == "ping/pong"){

        if(strcmp((const char*)payload["message"], "pong") == 0){

          Serial.println("pong recieved. breaking out");
          return;

        }

        Serial.println("ping/pong topic message recieved. sending message");
        publishMessage(topic);
      
    }

  }

}

void publishMessage(String topic){

  JSONVar response; 

  response["message"] = "pong";

  mqttClient.beginMessage(topic, false, 0, false);
  
  mqttClient.print(JSON.stringify(response));

  mqttClient.endMessage();

}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!mqttClient.connected()) {
    // MQTT client is disconnected, connect
    connectMQTT();
  }

  if(brewBeginTime != 0){
    if(millis() - brewBeginTime > 3600000){
        
        digitalWrite(relay_pin, HIGH);
        brewBeginTime = 0;
    }
  }
  // poll for new MQTT messages and send keep alive
  mqttClient.poll();

}


