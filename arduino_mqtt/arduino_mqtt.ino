#include <Arduino_JSON.h>
#include <ArduinoBearSSL.h>
#include <ArduinoECCX08.h>
#include <ArduinoMqttClient.h>
#include <WiFiNINA.h>
#include "secrets.h"

WiFiClient    wifiClient;
BearSSLClient sslClient(wifiClient);
MqttClient    mqttClient(sslClient);

unsigned long brewBeginTime = 0;

const int relay_pin = 0;

// Bug fix #3: fixed-size receive buffer with a known safe upper bound
const int MAX_MSG_SIZE = 512;

void setup() {
  Serial.begin(115200);

  // Bug fix #2: don't block forever waiting for Serial on boards without
  // native USB — wait at most 3 seconds then continue
  unsigned long t = millis();
  while (!Serial && millis() - t < 3000);

  if (!ECCX08.begin()) {
    Serial.println("No ECCX08 present!");
    while (1);
  }

  pinMode(relay_pin, OUTPUT);
  digitalWrite(relay_pin, HIGH);  // relay off on boot

  ArduinoBearSSL.onGetTime(getTime);
  sslClient.setEccSlot(0, SECRET_CERTIFICATE);
  mqttClient.onMessage(onMessageReceived);
}

void connectWiFi() {
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(WIFI_SSID);

  // actually retries instead of spinning on the initial (failed) status
  while (WiFi.begin(WIFI_SSID, WIFI_PASSWORD) != WL_CONNECTED) {
    Serial.println("attempting...");
    delay(5000);
  }

  Serial.println("Connected to WiFi");
}

void connectMQTT() {
  Serial.print("Attempting to connect to MQTT broker: ");
  Serial.println(IOT_BROKER);

  while (!mqttClient.connect(IOT_BROKER, 8883)) {
    Serial.print("failed, error: ");
    Serial.println(mqttClient.connectError());
    delay(5000);
  }

  Serial.println("Connected to MQTT broker");

  mqttClient.subscribe("coffee/brew");
  mqttClient.subscribe("ping/pong");
}

unsigned long getTime() {
  return WiFi.getTime();
}

void onMessageReceived(int messageSize) {
  String topic = mqttClient.messageTopic();
  Serial.print("Message on topic: ");
  Serial.println(topic);

  // Bug fix #3: guard against oversized messages before allocating on the stack
  if (messageSize <= 0 || messageSize >= MAX_MSG_SIZE) {
    Serial.println("Message too large or empty — discarding");
    // Drain the client so the next message isn't corrupted
    while (mqttClient.available()) mqttClient.read();
    return;
  }

  char buf[MAX_MSG_SIZE];
  mqttClient.readBytes(buf, messageSize);
  buf[messageSize] = '\0';

  JSONVar payload = JSON.parse(buf);

  if (JSON.typeof(payload) == "undefined") {
    Serial.println("Failed to parse JSON");
    return;
  }

  Serial.println(payload);

  if (!payload.hasOwnProperty("message")) {
    return;
  }

  // Bug fix #5: extract to String first — direct strcmp cast can null-deref
  // if the JSONVar value is not a string type
  String msg = (const char*)payload["message"];

  if (topic == "coffee/brew") {
    if (msg == "start") {
      digitalWrite(relay_pin, LOW);
      brewBeginTime = millis();
      Serial.println("Relay ON — brewing started");
      publishBrewConfirmation("on");
    } else {
      digitalWrite(relay_pin, HIGH);
      brewBeginTime = 0;
      Serial.println("Relay OFF — brewing stopped");
      publishBrewConfirmation("off");
    }
  }

  if (topic == "ping/pong") {
    if (msg == "pong") {
      Serial.println("Pong received");
      return;
    }
    // Bug fix #6: publish response to a separate topic to avoid an echo loop
    Serial.println("Ping received — sending pong");
    publishPong();
  }
}

void publishBrewConfirmation(const char* state) {
  JSONVar response;
  response["state"] = state;

  mqttClient.beginMessage("coffee/status", false, 0, false);
  mqttClient.print(JSON.stringify(response));
  mqttClient.endMessage();

  Serial.print("Published brew confirmation: ");
  Serial.println(state);
}

void publishPong() {
  JSONVar response;
  response["message"] = "pong";

  // Respond on a dedicated reply topic, not the shared ping/pong topic,
  // so the Arduino doesn't react to its own outbound message
  mqttClient.beginMessage("ping/pong/response", false, 0, false);
  mqttClient.print(JSON.stringify(response));
  mqttClient.endMessage();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!mqttClient.connected()) {
    connectMQTT();
  }

  // Safety cutoff: turn the relay off after 1 hour regardless of app state
  // millis() - brewBeginTime is rollover-safe with unsigned arithmetic
  if (brewBeginTime != 0 && millis() - brewBeginTime > 3600000UL) {
    digitalWrite(relay_pin, HIGH);
    brewBeginTime = 0;
    Serial.println("Safety cutoff — relay forced OFF after 1 hour");
    publishBrewConfirmation("off");
  }

  mqttClient.poll();
}
