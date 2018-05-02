#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "cert.h"

#define DEBUG
#ifdef DEBUG
  #define SERIALBEGIN(x) Serial.begin(x)
  #define SERIALPRINTLN(x) Serial.println(x)
  #define SERIALPRINT(x) Serial.print(x)
#else
  #define SERIALBEGIN(x)
  #define SERIALPRINTLN(x) 
  #define SERIALPRINT(x)
#endif

// ========= Konfigurasi Wifi =======
// Konfigurasi Wifi
// Ganti variabel di bawah ini 
// dengan konfigurasi wifi yang digunakan!
// -----------------------------------

#define wifi_ssid "WIFIDUMMY"
#define wifi_password "123456870"

// ========= END Konfigurasi Wifi =======

// ======= Konfigurasi mosquitto =======
// Ganti variabel di bawah ini 
// dengan konfigurasi mosquitto yang digunakan!
// -----------------------------------

#define mqtt_server "192.168.1.1"
#define mqtt_port 8883
#define mqtt_user "admin"
#define mqtt_password "adminpass"
const char* fingerprint = "34 CD FC 32 41 46 9B E0 D6 EE D4 1C 3D DD FD E7 E0 42 5D 25";

// ========= END Konfigurasi mosquitto ======

// ======= Konfigurasi wemos & topik mqtt =======
String wemos_id = "wemos-01";
String wemos_led_name = "wemos-01-led-01";
String wemos_sensor_name = "wemos-01-sensor-01";

String wemos_from_set_topic = "homebridge/from/set";
String wemos_from_get_topic = "homebridge/from/get";
String wemos_to_set_topic = "homebridge/to/set";
// ========= END Konfigurasi wemos & topik ======

#define STRBUFFNUM 500
char strbuff[STRBUFFNUM];

// sensor
uint16_t sensorValue;

// timer
uint64_t sensorTime, connect_ms;

// pin
uint8_t ledPin = D4;
uint8_t ledState;

WiFiClientSecure espClient;
PubSubClient client(espClient);

bool tls_verify_flag = false;


bool verifytls(void) {
  bool flag = false;
  
  SERIALPRINT("verify tls connecting to ");
  SERIALPRINTLN(mqtt_server);
  if (!espClient.connect(mqtt_server, mqtt_port)) {
    SERIALPRINTLN("connection failed");
    return false;
  }
  
  if (espClient.verify(fingerprint, mqtt_server)) {
    SERIALPRINTLN("certificate matches");
    return true;
  } else {
    SERIALPRINTLN("certificate doesn't match");
    return false;
  }
}

void loadcerts() {
  espClient.setCACert(ca_crt_der, ca_crt_der_len);
  espClient.setCertificate(wemos_crt_der, wemos_crt_der_len);
  espClient.setPrivateKey(wemos_key_der, wemos_key_der_len);
}

void setup_wifi() {
  SERIALPRINTLN();
  SERIALPRINT("Connecting to ");
  SERIALPRINTLN(wifi_ssid);
  
  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    SERIALPRINT(".");
  }
  
  SERIALPRINTLN("");
  SERIALPRINTLN("WiFi connected");
  SERIALPRINTLN("IP address: ");
  SERIALPRINTLN(WiFi.localIP());
}

void mqtt_reconnect(void) {
  if (client.connect(wemos_id.c_str(), mqtt_user, mqtt_password)) {
    client.subscribe(wemos_from_set_topic.c_str(), 0);
    client.subscribe(wemos_from_get_topic.c_str(), 0);
  }
  else SERIALPRINTLN("disconnected");
}

void callback(char* topic, byte* payload, unsigned int length) {
  String tpic_str = String(topic);
  
  StaticJsonBuffer<STRBUFFNUM> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(payload);

  if(tpic_str == wemos_from_set_topic) {
    if(root.success()) {
      if(root.containsKey("name")) {
        if(root["name"] == wemos_led_name) {
          if(root.containsKey("value")) {
            if(root["value"]) ledState = HIGH;
            else ledState = LOW;
          }
        }
      }
    }
  } else if(tpic_str == wemos_from_get_topic) {
    if(root.success()) {
      if(root.containsKey("name")) {
        if(root["name"] == wemos_led_name) {
          if (ledState == LOW) {
            root["value"] = false;
          } else {
            root["value"] = true;
          }
          root.printTo(strbuff, sizeof(strbuff));
          client.publish(wemos_to_set_topic.c_str(), strbuff);
        }
        else if(root["name"] == wemos_sensor_name) {
          root["value"] = sensorValue;
          root.printTo(strbuff, sizeof(strbuff));
          client.publish(wemos_to_set_topic.c_str(), strbuff);
        }
      }
    }
  }
}

void publish_sensor(void) {
  StaticJsonBuffer<STRBUFFNUM> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  
  sensorValue = 10 * analogRead(A0);
  SERIALPRINTLN("");
  SERIALPRINT("Sensor Value: ");
  SERIALPRINTLN(sensorValue);
  
  root["name"] = wemos_sensor_name;
  root["value"] = sensorValue;
    
  if(client.connected()) {
    root.printTo(strbuff, sizeof(strbuff));
    client.publish(wemos_to_set_topic.c_str(), strbuff);
  }
}

void setup() {
  pinMode(ledPin, OUTPUT);
  ledState = LOW;
  
  sensorTime = millis();
  connect_ms = millis();
  
  SERIALBEGIN(115200);

  setup_wifi();
  delay(500);
  loadcerts();
  delay(200);
  
  client.setServer(mqtt_server, 8883);
  client.setCallback(callback);
}

void loop() {
  if(!tls_verify_flag) {
    tls_verify_flag = verifytls();
    connect_ms = millis() - 6000;
  } else if (!client.connected()) {
    if((millis() - connect_ms) > 5000) {
      mqtt_reconnect();
      connect_ms = millis();
    }
  } else client.loop();
  
  //sensor
  if((millis() - sensorTime) > 500) {
    publish_sensor();
    sensorTime = millis();
  }

  digitalWrite(ledPin, ledState);
}
