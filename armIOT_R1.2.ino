#include <ModbusMaster.h>
#include <SoftwareSerial.h>


#define MAX485_DE      D5
#define MAX485_RE_NEG  D5
#define RX D7 
#define TX D6

/*
 * ESP822 temprature logging to Google Sheet
 * CircuitDigest(www.circuitdigest.com)
*/

#include <ESP8266WiFi.h>
#include "HTTPSRedirect.h"
#include "DebugMacros.h"
///#include <DHT.h>

#define DHTPIN D4                                                           // what digital pin we're connected to
#define DHTTYPE DHT11                                                       // select dht type as DHT 11 or DHT22
//DHT dht(DHTPIN, DHTTYPE);

float h;
float t;
String sheetHumid = "";
String sheetTemp = "";

//const char* ssid = "888";                //replace with our wifi ssid
//const char* password = "asdfghjkl";         //replace with your wifi password
const char* ssid = "Arm-etd";                //replace with our wifi ssid
const char* password = "vvvvvvvv";         //replace with your wifi password

const char* host = "script.google.com";
const char *GScriptId = "AKfycbxJw4Kr8WI7DkNTHCZNK8sry44aLmzEWr3pmArAQ_ocEdicgFw"; // Replace with your own google script id
const int httpsPort = 443; //the https port is same

// echo | openssl s_client -connect script.google.com:443 |& openssl x509 -fingerprint -noout
const char* fingerprint = "AB 47 65 C5 60 FB 56 08 C4 2C 5B 14 72 D8 B5 E9 F9 22 C9 B7";

//const uint8_t fingerprint[20] = {};

String url = String("/macros/s/") + GScriptId + "/exec?value=Temperature";  // Write Teperature to Google Spreadsheet at cell A1
// Fetch Google Calendar events for 1 week ahead
String url2 = String("/macros/s/") + GScriptId + "/exec?cal";  // Write to Cell A continuosly
String url3 = String("/mocros/s/") + GScriptId + "/exec?value=";

//replace with sheet name not with spreadsheet file name taken from google
String payload_base =  "{\"command\": \"appendRow\", \
                    \"sheet_name\": \"2020\", \
                       \"values\": ";
String payload = "";

HTTPSRedirect* client = nullptr;

// used to store the values of free stack and heap before the HTTPSRedirect object is instantiated
// so that they can be written to Google sheets upon instantiation

ModbusMaster node;
SoftwareSerial SoftSerial(RX,TX);

void setup() {
  delay(1000);
  Serial.begin(115200);
  SoftSerial.begin(9600);
  
  pinMode(MAX485_RE_NEG, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);

  node.begin(1, SoftSerial);
  node.setResponseTimeout(2000);
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  delay(10);
  Serial.println(" .. ");
  delay(500);
  Serial.println(" .. ");
  delay(500);
  Serial.println(" .. ");
  delay(500);
  Serial.println("complete config modbus communication");
  digitalWrite(D4,HIGH);
  delay(250);

  Serial.println();
  Serial.print("Connecting to wifi: ");
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

  // Use HTTPSRedirect class to create a new TLS connection
  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");
  Serial.print("Connecting to ");
  Serial.println(host);          //try to connect with "script.google.com"

  // Try to connect for a maximum of 5 times then exit
  bool flag = false;
  for (int i = 0; i < 5; i++) {
    int retval = client->connect(host, httpsPort);
    if (retval == 1) {
      flag = true;
      break;
    }
    else
      Serial.println("Connection failed. Retrying...");
  }

  if (!flag) {
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    Serial.println("Exiting...");
    return;
  }
// Finish setup() function in 1s since it will fire watchdog timer and will reset the chip.
//So avoid too many requests in setup()

  //Serial.println("\nWrite into cell 'A1'");
  //Serial.println("------>");
  // fetch spreadsheet data
  //client->GET(url, host);
  
  //Serial.println("\nGET: Fetch Google Calendar Data:");
  //Serial.println("------>");
  // fetch spreadsheet data
  //client->GET(url2, host);

 Serial.println("\nStart Sending Sensor Data to Google Spreadsheet");

  
  // delete HTTPSRedirect object
  delete client;
  client = nullptr;
}

void loop() {

 // h = random(0,199);//dht.readHumidity();                                              // Reading temperature or humidity takes about 250 milliseconds!
  //t = random(0,80);//dht.readTemperature();                                           // Read temperature as Celsius (the default)
  //if (isnan(h) || isnan(t)) {                                                // Check if any reads failed and exit early (to try again).
  //  Serial.println(F("Failed to read from DHT sensor!"));
  //  return;
 //}
  //Serial.print("Humidity: ");  Serial.print(h);
 // sheetHumid = String(h) + String("%");                                         //convert integer humidity to string humidity
 // Serial.print("%  Temperature: ");  Serial.print(t);  Serial.println("Â°C ");
  //sheetTemp = String(t) + String("Â°C");

  String meterdata=get_meter_data();
  
  static int error_count = 0;
  static int connect_count = 0;
  const unsigned int MAX_CONNECT = 20;
  static bool flag = false;

  payload = payload_base + "\"" + meterdata + "\"}";
  //payload = "Value1="+sheetTemp+"&Value2="+sheetHumid;
    //payload ="";
    //String url4=url3+sheetTemp;
    
  if (!flag) {
    client = new HTTPSRedirect(httpsPort);
    client->setInsecure();
    flag = true;
    client->setPrintResponseBody(true);
    client->setContentTypeHeader("application/json");
  }
  
  if (client != nullptr) {
    if (!client->connected()) {
      client->connect(host, httpsPort);
      //client->POST(url2, host, payload, false);
      Serial.println(" Connected ");  
      //Serial.println("Temp and Humid");
    }
  }
  else {
    DPRINTLN("Error creating client object!");
    error_count = 5;
  }

  if (connect_count > MAX_CONNECT) {
    connect_count = 0;
    flag = false;
    delete client;
    return;
  }

//  Serial.println("GET Data from cell 'A1':");
//  if (client->GET(url3, host)) {
//    ++connect_count;
//  }
//  else {
//    ++error_count;
//    DPRINT("Error-count while connecting: ");
//    DPRINTLN(error_count);
//  }

  Serial.println("POST or SEND Sensor data to Google Spreadsheet:");
  if (client->POST(url2, host, payload)) {
    flag = 0;
    delete client;
  }
  else {
    ++error_count;
    DPRINT("Error-count while connecting: ");
    DPRINTLN(error_count);
  }

  if (error_count > 3) {
    //Serial.println("Halting processor...");
    delete client;
    Serial.println("wait 10 minute");
    client = nullptr;
    flag = 0;
    Serial.printf("Final free heap: %u\n", ESP.getFreeHeap());
    Serial.printf("Final stack: %u\n", ESP.getFreeContStack());
    //Serial.flush();
    //ESP.deepSleep(0);
    delay(600000);
  }
  
  delay(300000);    // keep delay of minimum 2 seconds as dht allow reading after 2 seconds interval and also for google sheet
}


void preTransmission()
{
  digitalWrite(MAX485_RE_NEG, 1);
  digitalWrite(MAX485_DE, 1);
}

void postTransmission()
{
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
}


String get_meter_data(){ 
  int result;
  uint16_t data11[12];
  uint16_t data12[2];
  node.setSlaveID(1);
  delay(10);
  result = node.readHoldingRegisters(9, 12);
  //Serial.println("finish send request");
  
  if(result!=0){
    return("fail,to,connect,meter.");
  }
  for(int j=0;j<12;j++){
    data11[j]=node.getResponseBuffer(j);
  }
  //Serial.println("re array");
  
  node.clearResponseBuffer();
  delay(10);
  result = node.readHoldingRegisters(31, 2);
  //Serial.println("finish send request2");
  for(int k=0;k<2;k++){
    data12[k]=node.getResponseBuffer(k);
  }
  //Serial.println("re array2");
  
  node.clearResponseBuffer();
  delay(10);
  //uint16_t data21[12];
  //uint16_t data22[2];
  //node.setSlaveID(2);
  //delay(10);
  //result = node.readHoldingRegisters(9, 12);
  //for(int m=0;m<12;m++){
  //  data21[m]=node.getResponseBuffer(m);
  //}
  //node.clearResponseBuffer();
  
  //result = node.readHoldingRegisters(71, 2);
  //for(int n=0;n<2;n++){
  //  data22[n]=node.getResponseBuffer(n);
  //}
  //node.clearResponseBuffer();
  
  String payloadA;
  uint32_t temp32;
    
  for(int p=0;p<12;p+=2){
  temp32 = 0;
  temp32 |= data11[p+0];
  temp32 <<= 16;
  temp32 |= data11[p+1];    
  float tmpfloat = *(float*)&temp32;
  //Serial.println(tmpfloat,3);
  payloadA += (String)tmpfloat;
  payloadA += ",";
  }

  temp32 = 0;
  temp32 |= data12[0];
  temp32 <<= 16;
  temp32 |= data12[1];    
  float tmpfloat = *(float*)&temp32;
  payloadA += (String)tmpfloat;
  //payloadA += ",";

  //for(int q=0;q<12;q+=2){
  //temp32 = 0;
  //temp32 |= data21[q+0];
  //temp32 <<= 16;
  //temp32 |= data21[q+1];    
  //float tmpfloat = *(float*)&temp32;
  //payloadA += (String)tmpfloat;
  //payloadA += ",";
  //}

  //temp32 = 0;
  //temp32 |= data22[0];
  //temp32 <<= 16;
  //temp32 |= data22[1];    
  //tmpfloat = *(float*)&temp32;
  //payloadA += (String)tmpfloat;
  //payloadA += ",";
   
  return(payloadA);
}
