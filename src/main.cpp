// DHT Temperature & Humidity Sensor
// Unified Sensor Library Example
// Written by Tony DiCola for Adafruit Industries
// Released under an MIT license.

// REQUIRES the following Arduino libraries:
// - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
// - Adafruit Unified Sensor Lib: https://github.com/adafruit/Adafruit_Sensor


#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Servo.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h> //Librairie pour la gestion Mqtt
#include <Buzzer.h>
#include "main.h"

#define DHTPIN 14     // Digital pin connected to the DHT sensor
#define LEDPIN 5      // Digital pin connected to the led
#define BUZZERPIN 12  // Digital pin connected to the buzzer
#define SERVOPIN 4    // Digital pin connected to the servo
// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// Pin 15 can work but DHT must be disconnected during program upload.

// Uncomment the type of sensor in use:
#define DHTTYPE    DHT11     // DHT 11

// See guide for details on sensor wiring and usage:
//   https://learn.adafruit.com/dht/overview

DHT_Unified dht(DHTPIN, DHTTYPE);

uint32_t delayMS;

//WIFI
const char* ssid = "XXXXXXXXXX";
const char* password = "XXXXXXXXXXX";
//MQTT
const char* mqtt_server = "test.mosquitto.org";//Adresse IP du Broker Mqtt
const int mqtt_port = 1883; //port utilisé par le Broker
long tps=0;

#define MQTT_BROKER "local mosquitto"
#define MQTT_BROKER_PORT 1883

ESP8266WiFiMulti WiFiMulti;
WiFiClient espClient;
PubSubClient client(espClient);

// Variables declaration:
unsigned long previousMillis = 0;        // will store last time LED was updated
int ledState = LOW;             // ledState used to set the LED
float sensorTemp, sensorHum;
Buzzer buzzer(BUZZERPIN);
Servo myServo;  // create servo object to control a servo
// twelve servo objects can be created on most boards

void setup() {
  Serial.begin(115200);
  setup_wifi();
  setup_mqtt();
  client.publish("EPSI/DHT11/-1/Temperature", "Hello from ESP8266");
  client.publish("EPSI/DHT11/-1/Humidity", "Hello from ESP8266");
  // Initialize device.
  dht.begin();
  Serial.println(F("DHT11 Unified Sensor"));
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;
  pinMode(LEDPIN, OUTPUT);
  myServo.attach(SERVOPIN);  // attaches the servo on GIO$ to the servo object
}

void loop() {
  // Get temperature event and print its value.
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    sensorTemp = event.temperature;
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    Serial.println(F("°C"));
  }

  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    sensorHum = event.relative_humidity;
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    Serial.println(F("%"));
  }
  reconnect();
  client.loop();

  //servo_motor();
  //On utilise pas un delay pour ne pas bloquer la réception de messages
  if (millis()-tps>2000){
     tps=millis();
     mqtt_publish("EPSI/DHT11/-1/Temperature", sensorTemp);
     mqtt_publish("EPSI/DHT11/-1/Humidity", sensorHum);
  }
  led_light();
  buzzer_sound();
  servo_motor();
}

void led_light() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 1000) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
    // set the LED with the ledState of the variable:
    digitalWrite(LEDPIN, ledState);
  }
}

void buzzer_sound() {
  buzzer.begin(100);

  buzzer.sound(NOTE_E7, 80);
  buzzer.sound(NOTE_E7, 80);

  buzzer.end(500);
}

void servo_motor(){
  int pos;

  for (pos = 0; pos <= 90; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    myServo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
  for (pos = 90; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
    myServo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
}

void setup_wifi(){
  //connexion au wifi
  WiFiMulti.addAP(ssid, password);
  while ( WiFiMulti.run() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }
  Serial.println("");
  Serial.println("WiFi connecté");
  Serial.print("MAC : ");
  Serial.println(WiFi.macAddress());
  Serial.print("Adresse IP : ");
  Serial.println(WiFi.localIP());
}

void setup_mqtt(){
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);//Déclaration de la fonction de souscription
  reconnect();
}

//Callback doit être présent pour souscrire a un topic et de prévoir une action
void callback(char* topic, byte *payload, unsigned int length) {
   Serial.println("-------Nouveau message du broker mqtt-----");
   Serial.print("Canal:");
   Serial.println(topic);
   Serial.print("donnee:");
   Serial.write(payload, length);
   Serial.println();
   if ((char)payload[0] == '1') {
     Serial.println("LED ON");
      digitalWrite(2,HIGH);
   } else {
     Serial.println("LED OFF");
     digitalWrite(2,LOW);
   }
 }

 void reconnect(){
  while (!client.connected()) {
    Serial.println("Connection au serveur MQTT ...");
    if (client.connect("LNIFONPAPHIVOPZ")) {
      Serial.println("MQTT connecté");
    }
    else {
      Serial.print("echec, code erreur= ");
      Serial.println(client.state());
      Serial.println("nouvel essai dans 2s");
    delay(2000);
    }
  }
  client.subscribe("EPSI/DHT11/-1/Temperature");//souscription au topic led pour commander une led
}

//Fonction pour publier un float sur un topic
void mqtt_publish(String topic, float t){
  client.publish(topic.c_str(), String(t).c_str());
}
