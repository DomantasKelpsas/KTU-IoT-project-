#include "FS.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Servo.h>


// Update these with values suitable for your network.

const char* ssid = "POCOPHONE";
const char* password = "********";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

Servo Window_Servo;

///////////////////////////////////////////////////////////////////////////////////
#include <Wire.h>
#include <Adafruit_BME280.h>
Adafruit_BME280 bme; // I2C
bool BME_Status=0; // Flag, used o check if there is a error reading sensor


float Air_temp =-444;  // Variable to store air temp in degree C
float Humidity =-444;  // Variable to store Humidity in Percent
float Pressure =-444; 
/////////////////////////////////////////////////////////////////////////////////////

const char* AWS_endpoint = "********-ats.iot.us-east-2.amazonaws.com"; //MQTT broker ip

int Servo_Max_Speed =20;  // How quick the motor turns in degree's per second, reduce this if you have serial conection problems
int Servo_Pos_Closed = 90; // Servo possition coresponding to a closed window
int Servo_Pos_Open = 150;  //Servo possition coresponding to Open window

int Max_Possition =150;   // Variable to limit the max angle the servo can travel
int Min_Possition =30;    // Variable to limit the min angle the servo can travel
int Servo_Possition=20;   //Variable used for motor speed control

String tempBool = "";

void callback(char* topic, byte* payload, unsigned int length) {
Serial.print("Message arrived [");
Serial.print(topic);
Serial.print("] ");
for (int i = 0; i < length; i++) {
Serial.print((char)payload[i]);

}
Serial.println();
tempBool = String((char*)payload);


}
WiFiClientSecure espClient;
PubSubClient client(AWS_endpoint, 8883, callback, espClient); //set MQTT port number to 8883 as per //standard
long lastMsg = 0;
char msg[500];
int value = 0;

void setup_wifi() {

delay(10);
// We start by connecting to a WiFi network
espClient.setBufferSizes(512, 512);
Serial.println();
Serial.print("Connecting to ");
Serial.println(ssid);

WiFi.begin(ssid, password);

while (WiFi.status() != WL_CONNECTED) {
delay(500);
Serial.print(".");
}

Serial.println("");
Serial.println("WiFi connected");
Serial.println("IP address: ");
Serial.println(WiFi.localIP());

timeClient.begin();
while(!timeClient.update()){
timeClient.forceUpdate();
}

espClient.setX509Time(timeClient.getEpochTime());

}

void reconnect() {
// Loop until we're reconnected
while (!client.connected()) {
Serial.print("Attempting MQTT connection...");
// Attempt to connect
if (client.connect("ESPthing")) {
Serial.println("connected");
// Once connected, publish an announcement...
client.publish("outTopic", "hello world");
// ... and resubscribe
client.subscribe("inTopic");
} else {
Serial.print("failed, rc=");
Serial.print(client.state());
Serial.println(" try again in 5 seconds");

char buf[256];
espClient.getLastSSLError(buf,256);
Serial.print("WiFiClientSecure SSL error: ");
Serial.println(buf);

// Wait 5 seconds before retrying
delay(5000);
}
}
}

void Read_BME() {


BME_Status= bme.begin(0x76);

if (BME_Status==1){   

//Setting values to distinct numbers so corrupt data can be seen easily  
Air_temp =-444;  
Humidity =-444;  
Pressure =-444; 

  Air_temp= bme.readTemperature();
  Humidity= bme.readHumidity();
  Pressure= bme.readPressure();

//Calculating VDP or how well a plant is breathing
//VPsat = 610.7 * pow(10, (7.5 * Air_temp / (237.3 + Air_temp)));// Saturation vapor pressure in Pascals
//VPactual = (Humidity * VPsat) / 100.0;  // Actual vapor pressure in Pascals
//VPD = ((100.0 - Humidity) /100.0) * VPsat;  // Vapor Pressure Deficit in Pascals
//VPD=VPD/1000.0; // Greenhouse folk like to use the unit of kPa
  
    Serial.print("Temperature = ");
    Serial.print(Air_temp);
    Serial.println(" *C");

    Serial.print("Pressure = ");
    Serial.print(Pressure);
    Serial.println(" hPa");

    Serial.print("Humidity = ");
    Serial.print(Humidity);
    Serial.println(" %");

    
//    Serial.print("VPD = ");
//    Serial.print(VPD);

    Serial.println();
}
 else Serial.println("BME Error");
 //bme.writeMode(smSleep);    
}

void setup() {

Serial.begin(9600);
Window_Servo.attach(D8);
Wire.begin(D6,D5);
Read_BME();
Serial.setDebugOutput(true);
// initialize digital pin LED_BUILTIN as an output.
pinMode(LED_BUILTIN, OUTPUT);
setup_wifi();
delay(1000);
if (!SPIFFS.begin()) {
Serial.println("Failed to mount file system");
return;
}

Serial.print("Heap: "); Serial.println(ESP.getFreeHeap());

// Load certificate file
File cert = SPIFFS.open("/cert.der", "r"); //replace cert.crt eith your uploaded file name
if (!cert) {
Serial.println("Failed to open cert file");
}
else
Serial.println("Success to open cert file");

delay(1000);

if (espClient.loadCertificate(cert))
Serial.println("cert loaded");
else
Serial.println("cert not loaded");

// Load private key file
File private_key = SPIFFS.open("/private.der", "r"); //replace private eith your uploaded file name
if (!private_key) {
Serial.println("Failed to open private cert file");
}
else
Serial.println("Success to open private cert file");

delay(1000);

if (espClient.loadPrivateKey(private_key))
Serial.println("private key loaded");
else
Serial.println("private key not loaded");

// Load CA file
File ca = SPIFFS.open("/ca.der", "r"); //replace ca eith your uploaded file name
if (!ca) {
Serial.println("Failed to open ca ");
}
else
Serial.println("Success to open ca");

delay(1000);

if(espClient.loadCACert(ca))
Serial.println("ca loaded");
else
Serial.println("ca failed");

Serial.print("Heap: "); Serial.println(ESP.getFreeHeap());
}

void Move_Servo(int new_position){

//angle sanity check
if(new_position>Max_Possition) new_position=Max_Possition;
if(new_position<Min_Possition) new_position=Min_Possition;



// Incrimenting the servo possition one degree at a time with a pause inbetween
while(Servo_Possition!=new_position){


    if(Servo_Possition>new_position){
      Servo_Possition--;
      Window_Servo.write(Servo_Possition);   
                                    }
                                    
    if(Servo_Possition<new_position){
      Servo_Possition++;
      Window_Servo.write(Servo_Possition);
                                      }
delay(1000/Servo_Max_Speed); //   
  
}
delay(500); // when the motor stops moving it can cause voltage spikes, this delay stops it interfering with serial coms

  
}

void loop() {

if (!client.connected()) {
reconnect();
}
client.loop();

long now = millis();
if (now - lastMsg > 2000) {
lastMsg = now;
//Read_BME();
//++value;
//snprintf (msg, 75, "{\"temp\": \"#%ld\"}", Air_temp);
//Serial.print("Publish message: ");
//Serial.println(msg);
//client.publish("temp", msg);
//Serial.print("Heap: "); Serial.println(ESP.getFreeHeap()); //Low heap can cause problems
Serial.println(tempBool);
}
if(tempBool!="false") Move_Servo(Servo_Pos_Closed);
else Move_Servo(Servo_Pos_Open);
digitalWrite(LED_BUILTIN, HIGH); // turn the LED on (HIGH is the voltage level)
delay(100); // wait for a second
digitalWrite(LED_BUILTIN, LOW); // turn the LED off by making the voltage LOW
delay(100); // wait for a second
}
