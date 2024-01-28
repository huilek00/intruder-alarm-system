/*
INTRUDER ALARM SYSTEM using Google Cloud Platform and MongoDB Atlas for Assignment 2
*/
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#define NUMPIXELS 1
#define DELAYVAL 500
#define INTERVAL 500 

//Pins, change the pins according to your setup
const int buzzer = 12;         //at Pin 12
const int ledPin = 47;         //at Pin 48
const int doorSensor = 48;        //at Pin 47
const int motionSensor = 4;    //Left side Maker Port
const int neoPin = 46;         //onboard Neopixel
bool ledflag = 0;

//WIFI, change the SSID and password according to your setup
const char *WIFI_SSID = "<wifi SSID>";
const char *WIFI_PASSWORD = "<wifi password>";

//MQTT, change the MQTT server IP according to your GCP VM setup
const char *MQTT_SERVER = "<external_IP from GCP VM>"; //external IP from GCP VM
const int MQTT_PORT = 1883;
const char *MQTT_TOPIC = "intruder";
// Timer: Auxiliary variables
unsigned long now = millis();
unsigned long lastTrigger = 0;
boolean PIRvalue = false;
Adafruit_NeoPixel pixels(NUMPIXELS, neoPin, NEO_GRB + NEO_KHZ800);

// Checks if motion was detected, sets LED HIGH and starts a timer
void IRAM_ATTR detectsMovement() {
  PIRvalue = true;
  lastTrigger = millis();
}

//last message time
unsigned long lastMsgTime = 0;

//setup Wifi Client
WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {

  //sensor
  pinMode(ledPin, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(doorSensor,INPUT_PULLUP);
  pinMode(motionSensor, INPUT);
  digitalWrite(ledPin, LOW);
  // Connect to WiFi
  setup_wifi();
  // Connect to MQTT server
  client.setServer(MQTT_SERVER, MQTT_PORT);

  // Set motionSensor pin as interrupt, assign interrupt function and set RISING mode
  attachInterrupt(digitalPinToInterrupt(motionSensor), detectsMovement, RISING);
}

void mqttreconnect()
{
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("Connected to MQTT server");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void loop() {
  // Validation to make sure WiFi connection.
  if (!client.connected()) {
    mqttreconnect();
  }
  client.loop();

  unsigned long cur = millis();
  if (cur - lastMsgTime > INTERVAL) {
    lastMsgTime = cur;

    //Publish telemetry data
    int PIRvalue = digitalRead(motionSensor);
    int DoorValue = digitalRead(doorSensor);
    int ledValue = digitalRead(ledPin);
    
    if (ledValue == HIGH) {

      for (int i = 0; i < NUMPIXELS; i++) {  // For each pixel...
        pixels.setPixelColor(i, pixels.Color(0, 100, 0));
        pixels.show();  // Send the updated pixel colors to the hardware.
        delay(DELAYVAL);
        pixels.setPixelColor(i, pixels.Color(100, 0, 0));
        pixels.show();  // Send the updated pixel colors to the hardware.
        delay(DELAYVAL);
        pixels.setPixelColor(i, pixels.Color(0, 0, 100));
        pixels.show();    // Send the updated pixel colors to the hardware.
        delay(DELAYVAL);  // Pause before next pass through loop
        pixels.setPixelColor(i, pixels.Color(0, 0, 0));
        pixels.show();    // Send the updated pixel colors to the hardware.
        delay(DELAYVAL);  // Pause before next pass through loop
      }
    } else {
      pixels.clear();  // Set all pixel colors to 'off'
    }

    if (DoorValue == HIGH || PIRvalue == HIGH) {
      digitalWrite(ledPin, HIGH);
      Serial.println("Door is Opened");
      tone(buzzer, 300);
      delay(DELAYVAL);
      noTone(buzzer);
      // Format data into JSON format to store in database later
      char payload[100];
      sprintf(payload, "{\"PIR_Reading\":%d, \"DoorSensor_Reading\":%d}", PIRvalue, DoorValue);
      client.publish(MQTT_TOPIC, payload);
      // Print out the payload
      Serial.println(payload);
      // Some buffer time before performing detection again
      delay(500);
    }
    else
    {
      digitalWrite(ledPin, LOW);
    }
    delay(DELAYVAL);
  }
}