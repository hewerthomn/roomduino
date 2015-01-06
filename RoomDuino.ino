/*
* ----------------------------------------------------------------------------
* "THE BEER-WARE LICENSE" (Revision 42):
* <phk@FreeBSD.ORG> wrote this file. As long as you retain this notice you
* can do whatever you want with this stuff. If we meet some day, and you think
* this stuff is worth it, you can buy me a beer in return Poul-Henning Kamp
* ----------------------------------------------------------------------------
*/

/*
* RoomDuino
*/

#include <SPI.h>
#include <dht11.h>
#include <Ethernet.h>

String Placename = "Room";
//#define LOCAL 1

#ifdef LOCAL
IPAddress server(10, 1, 1, 8);
char apiPath[] = "/roomduino/api/";
#endif
#ifndef LOCAL
char server[] = "roomduino.hewertho.mn";
char apiPath[] = "/api/";
#endif

/*
* Ethernet Shield Configs
*/
EthernetClient client;
byte mac[] = { 0xDE, 0xAD, 0xBA, 0xEF, 0xFE, 0xBE };
IPAddress ip(10, 1, 1, 25);
IPAddress gateway(10, 1, 1, 1);
IPAddress subnet(255, 255, 255, 0);

int failedConnect = 0;
int failedConnectsRetry = 5;
boolean hasStats = false; // until this is true default text will be displayed

/* Sensor Temperature */
#define pinDHT11 9
dht11 DHT11;

/* Sensor Light */
#define pinLight 0
int Photocell;

/*
* Variable of sensors
*/
int Temperature = 0;
int Humidity = 0;
char Date[] = "Dez, 31";
char Time[] = "00:00";

char Debug[80];
long updateFrequency = 5 * 60000; // 60000 = 1 minute
String data;

void(* reset) (void) = 0;

void setup()
{
  Serial.begin(9600);  
  Serial.println("# RoomDuino starting....");
  
  int eth;
  #ifdef LOCAL
  eth = Ethernet.begin(mac, ip, gateway, subnet);
  #endif
  
  #ifndef LOCAL
  eth = Ethernet.begin(mac); //, ip, gateway, subnet);
  if(eth == 0)
  {
    Serial.println("# Failed DHCP connection. Restarting...");
    delay(3000); reset();
  }
  #endif
  
  delay(3000);  
  Serial.print("# IP local: "); Serial.println(Ethernet.localIP());
}

void loop()
{
  printSensorData();
    
  getTemperature();
  getPhotocell();
  
  connectServer();
  
  checkFailedsConnect();
  #ifdef LOCAL
  delay(3000);
  #endif
  
  #ifndef LOCAL
  delay(updateFrequency);
  #endif
}

void printSensorData()
{
  Serial.println("");Serial.println("# ------------------------------------");
  Serial.print("# Date "); Serial.print(Date); Serial.print("  Time "); Serial.println(Time);
  Serial.print("# Photocell: "); Serial.println(Photocell);
  Serial.print("# Temperature "); Serial.print(Temperature); Serial.print("  /  Humidity "); Serial.print(Humidity); Serial.println("%");  
  Serial.print("# Failed connects: "); Serial.println(failedConnect);
  Serial.println("");
}

void getTemperature()
{ 
  Serial.print("# DHT11 ");
  int chk = DHT11.read(pinDHT11);

  switch(chk)
  {
    case DHTLIB_OK:
      Temperature = DHT11.temperature;
      Humidity = DHT11.humidity;
      Serial.println("Ok!");
    break;
    
    case DHTLIB_ERROR_CHECKSUM:
      Serial.println("Checksum Error");
    break;
    
    case DHTLIB_ERROR_TIMEOUT:
      Serial.println("Timeout");
    break;
    
    default:
      Serial.println("Unknown Error");
    break;
  }
}

void getPhotocell()
{
  Photocell = analogRead(pinLight);
}

void connectServer()
{  
  Serial.print("# Ethernet Connect....");  
  int conn = client.connect(server, 80);
  if(conn)
  {
    Serial.println("Ok!"); delay(500);
    
    postSensorData();
    extractData();
    
    client.stop();
    
    Serial.println("Done!");
  }
  else
  {
    failedConnect++;
    Serial.println("Error!");
    Serial.print("# Ethernet Connect Error: "); Serial.println(conn);
  }
}

void postSensorData()
{
  data = "placename=" + Placename;
  data = data + "&temperature=" + Temperature;
  data = data +"&humidity=" + Humidity;
  data = data + "&photocell=" + Photocell;
  
  Serial.print("# Post data: "); Serial.println(data);
  
  client.print("POST ");
  client.print(apiPath);
  client.print("data");
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.println(server);
  client.println("User-Agent: Arduino/1.0");
  client.println("Connection: close");
  client.println("Content-type: application/x-www-form-urlencoded");
  client.print("Content-Length: ");
  client.println(data.length());
  client.println();
  client.println(data);
}

void clearClient()
{
  while(client.available()) client.read();  
}

void extractData()
{
  char currentValue[80];
  boolean dataFlag = false; // true if data has started
  boolean endFlag = false; // true if data is reached
  int j = 0;
  int i = 0;
  char c;
  
  Serial.print("# Extracting server data..."); 
  
  while(client.connected() && !endFlag)  
  {
    if(client.available())
    {
      char c = client.read();    
      
      if(c == '<')
      {
        dataFlag = true;
        hasStats = true;      
        Serial.print("o");
      }
      else if(dataFlag && c == '>') // end of data
      {
        setStatValue(j, currentValue);
        endFlag = true;
        Serial.print("!");
      }
      else if(dataFlag && c == '|') // next dataset
      {
        setStatValue(j++, currentValue);
        char currentValue[80];
        i = 0;
        Serial.print("o");
      }
      else if(dataFlag)
      {
        currentValue[i++] = c;
        Serial.print(".");
      }
    }    
  }
  
  Serial.println("Ok!");
}

void setStatValue(int order, char value[])
{
  int len = 0;  
  //Serial.print("# order: ");Serial.print(order);Serial.print(" value: ");Serial.println(value);
  
  switch(order)
  {
    case 0: // Date
      for(int i=0; i<7; i++) {
        Date[i] = value[i];
      }
      break;
      
    case 1: // Time
      for(int i=0; i<5; i++) {
        Time[i] = value[i];
      }
      break;
      
    case 2: // Debug
      for(int i=0; i<80; i++) {
        Debug[i] = value[i];
      }
      Serial.print("# DEBUG: "); Serial.println(Debug);
    break;
  }
}

void checkFailedsConnect()
{
  if(failedConnect == failedConnectsRetry)
  {
    Serial.println("# Connects failed reach limit. Restarting...");
    delay(3000); reset();
  }
}
