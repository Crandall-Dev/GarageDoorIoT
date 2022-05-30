/**
 * Garage door open/close IoT node
 * 
 * ESP8266 core
 * HC-SR04 distance sensor
 * 3D printed case
 * MQTT client - broadcasting state
 * 
 * Author: Aaron S. Crandall <crandall@gonzaga.edu>
 * Copyright: 2022
 * 
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// WiFi
//const char *ssid = "netssid"; // Enter your WiFi name
//const char *password = "passwd";  // Enter WiFi password

// MQTT Broker
const char *mqtt_broker = "10.0.0.4";
const char *topic = "test/topic";
//const char *mqtt_username = "emqx";
//const char *mqtt_password = "public";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

const int trigPin = 12;
const int echoPin = 14;

//define sound velocity in cm/uS
#define SOUND_VELOCITY 0.034
#define CM_TO_INCH 0.393701

long duration;
float distanceCm;
float distanceInch;

float samples[10];
#define SAMPLES_COUNT 10
#define INF_DISTANCE_CM 2800

enum DoorStatus {
  IS_CLOSED,
  IS_OPEN,
};

unsigned int last_report_epoch;
#define REPORT_TIMEOUT_MS 500


// ** ************************************************************
void setup() {
  // Set software serial baud to 115200;
  Serial.begin(115200);

  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  
  // connecting to a WiFi network
  //WiFi.begin(ssid, password);
  WiFi.begin();
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");

  //connecting to a mqtt broker
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  while (!client.connected()) {
      String client_id = "esp8266-client-";
      client_id += String(WiFi.macAddress());
      Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
      //if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      if (client.connect(client_id.c_str())) {
          Serial.println("MQTT broker connected");
      } else {
          Serial.print("failed with state ");
          Serial.print(client.state());
          delay(2000);
      }
  }
  // publish and subscribe
  client.publish(topic, "hello emqx try 2 first worked");
  client.subscribe(topic);

  last_report_epoch = millis();
}


// ** ************************************************************
void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
      Serial.print((char) payload[i]);
  }
  Serial.println();
  Serial.println("-----------------------");
}


// ** ************************************************************
void loop() {
  client.loop();

  if(last_report_epoch + REPORT_TIMEOUT_MS < millis()) {
    runReport();
  }
  
}


// ** ************************************************************
void runReport() {
  char message[30];

  if(isDoorOpen() == IS_OPEN) {
    strcpy(message, "DOOR OPEN");
  } else if(isDoorOpen() == IS_CLOSED) {
    strcpy(message, "DOOR CLOSED");
  } else {
    strcpy(message, "UNKNOWN STATUS");
  }

  client.publish(topic, message);
}


// ** ************************************************************
DoorStatus isDoorOpen() {
  takeSamples();

  int inf_samples_count = 0;
  for(int i = 0; i < SAMPLES_COUNT; i++) {
    if(samples[i] > INF_DISTANCE_CM) {
      inf_samples_count++;
    }
  }

  if( inf_samples_count >= 4 ) {
    return IS_CLOSED;
  } else {
    return IS_OPEN;
  }
}


// ** ************************************************************
float getDistanceSample() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  duration = pulseIn(echoPin, HIGH);  
  distanceCm = duration * SOUND_VELOCITY/2;
  distanceInch = distanceCm * CM_TO_INCH;

  return distanceCm;
}


// ** ************************************************************
void takeSamples() {
  for(int i = 0; i < SAMPLES_COUNT; i++) {
    samples[i] = getDistanceSample();
    delay(10);
  }
}
