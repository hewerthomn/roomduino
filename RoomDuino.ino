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

/*
* Ethernet Shield Configs
*/
EthernetClient client;
byte mac[] = { 0xDE, 0xAD, 0xBA, 0xEF, 0xFE, 0xBA };
IPAddress ip(10, 1, 1, 25);
IPAddress gateway(10, 1, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress server(10, 1, 1, 8);
//  char server[] = "duino.hewertho.mn";
char apiPath[] = "/roomduino/api/";
char placename[] = "Room";

/* Sensor Temperature */
#define pinDHT11 9
dht11 DHT11;
int Temperature = 0;
int Humidity = 0;

/*
* Variable of contents
*/
char Date[] = "Dez, 31";
char Time[] = "00:00";

boolean hasStats = false; // until this is true default text will be displayed

long updateFrequency = .5 * 60000; // 6000 = 1 minute

void setup()
{
  Serial.begin(9600);  
  Serial.println("Starting....");
  
  Ethernet.begin(mac, ip, gateway, subnet);
  
  delay(5000);  
  Serial.print("IP local: "); Serial.println(Ethernet.localIP());
}

void loop()
{
  Serial.println("");Serial.println("------------------------------------");
  
  getTemperature();  
  connectServer();
  
    delay(updateFrequency);
}

void getTemperature()
{ 
  int chk = DHT11.read(pinDHT11);
  
  switch(chk)
  {
    case DHTLIB_OK:
      Temperature = DHT11.temperature;
      Humidity = DHT11.humidity;      
      Serial.print("Temperature "); Serial.print(Temperature); Serial.print("  Humidity "); Serial.print(Humidity); Serial.println("%");
    break;
    
    case DHTLIB_ERROR_CHECKSUM:
      Serial.println("# DHT11 Checksum Error");
    break;
    
    case DHTLIB_ERROR_TIMEOUT:
      Serial.println("# DHT11 Timeout");
    break;
    
    default:
      Serial.println("# DHT11 Unknown Error");
    break;
  }
}

void connectServer()
{
  Serial.print("Conectando....");
  int conn = client.connect(server, 80);
  if(conn)
  {
    Serial.println("Ok!");
    delay(500);
    sendTemperature();
    client.stop();
  }
  else
  {
    Serial.println("Erro!");
    return;
  }
}

void sendTemperature()
{
  String params = "T=";
  params = params + Temperature;
  params = params + "&H=" + Humidity;
  
  Serial.print("Enviando temperaturas....");  
  sendRequest("GET", "temperatures", params);
  Serial.println("Ok!");
}

void sendRequest(char method[], char path[], String params)
{
  client.print(method);
  client.print(" ");
  client.print(apiPath);
  client.print(path);
  client.print("?P=");
  client.print(placename);
  client.print("&");
  client.print(params);
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.println(server);
  client.println("User-Agent: Arduino");
  client.println("Connection: close");
  client.println("");
}

void extractData(EthernetClient client)
{
  char currentValue[16];
  boolean dataFlag = false; // true if data has started
  boolean endFlag = false; // true if data is reached
  int j = 0;
  int i = 0;
  
  Serial.println("Extraindo dados....");
   
  while(client.connected() && !endFlag)
  {
    char c = client.read();
    
   // Serial.print(c);
    
    if( c == '<')
    {
      dataFlag = true;
      hasStats = true;      
    }
    else if(dataFlag && c == '>') // end of data
    {
      setStatValue(j, currentValue);
      endFlag = true;
    }
    else if(dataFlag && c == '|') // next dataset
    {
      setStatValue(j++, currentValue);
      char currentValue[16];
      i = 0;
    }
    else if(dataFlag)
    {
      currentValue[i++] = c;
    }
  }
  Serial.println("Ok!");
}

void setStatValue(int order, char value[])
{
  int len = 0;  
  //Serial.print("order: ");Serial.print(order);Serial.print(" value: ");Serial.println(value);
  
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
  }
}
  
