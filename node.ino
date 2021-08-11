/* This is a simple program to connect the LinkIt ONE Based Propane-O-Meter
 * to Ubidots IoT to share system status info and propane tank content quantity.
 * uses GPRS data connection, and is Solar Powered and gps enabled.
***********************************************************************************/
#include <LTask.h>
#include <LWiFi.h>
#include <LWiFiClient.h>
#include <LGPS.h>            // LinkIt GPS Library
#include <LBattery.h>  // Want to be able to read the battery charge level
#include <LGPRS.h>      //include the base GPRS library
#include <LGPRSClient.h>  //include the ability to Post and Get information using HTTP
#include "DHT.h"           //Library to read DHT22 Temp and Humm sensor.

#define WIFI_AP "xxxxxxx"
#define WIFI_PASSWORD "xxxxxxx"
#define WIFI_AUTH LWIFI_WPA  // choose from LWIFI_OPEN, LWIFI_WPA, or LWIFI_WEP.






#define DHTPIN 8     // what pin we're connected to temp and humm sensor
#define DHTTYPE DHT22   // DHT 22  (AM2302) sensor. required by library
DHT dht(DHTPIN, DHTTYPE); // declare temo and hum sensor type object


// These are the variables you will want to change based on your IOT data streaming account / provider
char action[] = "POST ";  // Edit to build your command - "GET ", "POST ", "HEAD ", "OPTIONS " - note trailing space
char server[] = "things.ubidots.com";
char path[] = "/api/v1.6/collections/values";  // Edit Path to include you source key
char token[] = "joS8W4xxLtT8lSdIsQnb4CE8eLPaR7";  // Edit to insert you API Token
int port = 80; // HTTP


// Here are the program variables
int num;                            // part of the length calculation
String le;                         // length of the payload in characters
String var;                        // This is the payload or JSON request
unsigned long ReportingInterval = 300000;  // How often do you want to update the IOT site in milliseconds
unsigned long LastReport = 0;      // When was the last time you reported
const int ledPin = 13;                 // Light to blink when program terminates
String Location = "";          // Will build the Location string here


int solarcurrent = 0;
int solarvoltage = 0;
long rawtankweight = 0;


float h = 0;    //Store dht22 sensor hummydity
float t = 0;    //Sotore dht22 sensor Temperature

// Create instantiations of the GPRS and GPS functions
//LGPRSClient globalClient;  // See this support topic from Mediatek - http://labs.mediatek.com/forums/posts/list/75.page
gpsSentenceInfoStruct info;  // instantiate
LWiFiClient globalClient;                //Create a object for use of wifi

void setup()
{
  Serial.begin(115200);             // setup Serial port

  LWiFi.begin();


  LGPS.powerOn();                  // Start the GPS first as it takes time to get a fix
  Serial.println("GPS Powered on, and waiting ...");
  /*Serial.println("Attach to GPRS network");   // Attach to GPRS network - need to add timeout
  while (!LGPRS.attachGPRS("web.vmc.net.co","Virgin Mobile","")) { //attachGPRS(const char *apn, const char *username, const char *password);
    Serial.print(".");
    delay(500);
  }
  Serial.println("Connected to Virgin Mobile Colombia GPRS Network");

  LGPRSClient client;    //Client has to be initiated after GPRS is established with the correct APN settings - see above link
  globalClient = client;  // Again this is a temporary solution described in support forums
*/
  pinMode(DHTPIN,INPUT); // define dht22 sensor pin as input

    dht.begin();         // initialilize dht22 sensor

  pinMode(4, OUTPUT); // data line
  pinMode(5, OUTPUT);//clock line
  pinMode(13, OUTPUT);//clock line
  digitalWrite(4,LOW);
  digitalWrite(5,LOW);

  Serial.println("Connecting to AP");
  while (0 == LWiFi.connect(WIFI_AP, LWiFiLoginInfo(WIFI_AUTH, WIFI_PASSWORD)))
  {
    Serial.println("Re-try Connecting AP");
    delay(1000);
  }

  Serial.println("connected to AP ");
  Serial.println(WIFI_AP);

}

void loop(){




  LGPS.getData(&info);                     // Get a GPS fix
       if (ParseLocation((const char*)info.GPGGA)) {  // This is where we break out needed location information
         //Serial.print("Location is: ");
         //Serial.println(Location);                 // This is the format needed by Ubidots
       }


  if (millis() >= LastReport + ReportingInterval) {  // Section to report - will convert to a function on next rev
    LGPS.powerOff();                  // Start the GPS first as it takes time to get a fix
    Serial.println("GPS Powered off, to avoid signals shift and load cell reading error");
    delay(1000);



    digitalWrite(13,HIGH);
    delay(1000);
    digitalWrite(13,LOW);
    delay(500);
    rawtankweight = 0;        // To store hx711 ADC lecture of load cell
    solarcurrent = 0;
    solarvoltage = 0;
    for(int k=0;k<10;k++){
      rawtankweight=rawtankweight+read1();//read load cell data
       solarcurrent = solarcurrent + analogRead(A1);
       solarvoltage = solarvoltage + analogRead(A0);

    }

    rawtankweight=rawtankweight/10;
    solarcurrent = solarcurrent/10;
    Serial.print("corriente");
    Serial.println(solarcurrent);
    float solarcurrentmili = (5*float(solarcurrent))/1024;
    Serial.print("corriente en volt");
    Serial.println(solarcurrentmili);
    solarcurrentmili = ((solarcurrentmili-2.5)/45)*1000;
    Serial.print("corriente en miliamps");
    Serial.println(solarcurrentmili);
    solarvoltage = solarvoltage/10;
    solarvoltage = (solarvoltage*10)/1024;

    while(!dht.readHT(&t,&h)){
      delay(2000);
    }

    String value = String(LBattery.level());     //Linkit One Battery Status
    String chargestatus = String(LBattery.isCharging());     //Linkit One Battery Status

    Serial.print("loadcel=");
    Serial.println(rawtankweight);
    Serial.print("Solar Panel Volt:");
    Serial.println(solarvoltage);
    Serial.print("Solar Panel Current:");
    Serial.println(solarcurrentmili);
    Serial.print("Linkit battery charge level:");
    Serial.println(value);
    Serial.print("Linkit battery charge status:");
    Serial.println(chargestatus);
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.println(" *C");








    var="[{\"variable\":\"562ef0c37625427dad377d5f\",\"value\":" +  String(t) +"},";
    var=var +  "{\"variable\":\"562ef0f67625427de23bf516\",\"value\":" +  String(h) +"},";
    var=var +  "{\"variable\":\"562ef14d7625427eea154a21\",\"value\":" +  String(value) +"},";
    var=var +  "{\"variable\":\"562ef17d76254201ed8d3f27\",\"value\":" +  String(chargestatus) +"},";
    var=var +  "{\"variable\":\"562ef196762542024600de71\",\"value\":" +  String(solarvoltage) +"},";
    var=var +  "{\"variable\":\"563011ae762542683080b6bd\",\"value\":" +  String(solarcurrentmili) +"},";
    var=var +  "{\"variable\":\"562ef1b27625420302ec1c98\",\"value\":" +  String(rawtankweight) +"},";

    if (Location.length()==0) {
         var=var +  "{\"variable\":\"5630159a76254274497d0d2e\",\"value\":" +  String(0) + "}]"; //Build the JSON packet without GPS info
       } else {
      var=var +  "{\"variable\":\"5630159a76254274497d0d2e\",\"value\":" +  String(1) + ", \"context\":"+ Location + "}]";  // with GPS info
       }



     num=var.length();               // How long is the payload
     le=String(num);                 //this is to calcule the length of var
     Serial.print("Connect to ");    // For the console - show you are connecting
     Serial.println(server);

    if (globalClient.connect(server, port)){  // if you get a connection, report back via serial:
      Serial.println("connected");  // Console monitoring
      Serial.print(action);                   // These commands build a JSON request for Ubidots but fairly standard
      Serial.print(path);                     // specs for this command here: http://ubidots.com/docs/api/index.html
      Serial.println(" HTTP/1.1");
      Serial.println(F("Content-Type: application/json"));
      Serial.print(F("Content-Length: "));
      Serial.println(le);
      Serial.print(F("X-Auth-Token: "));
      Serial.println(token);
      Serial.print(F("Host: "));
      Serial.println(server);
      Serial.println();
      Serial.println(var);  // The payload defined above
      Serial.println();
      Serial.println((char)26); //This terminates the JSON SEND with a carriage return
      globalClient.print(action);                   // These commands build a JSON request for Ubidots but fairly standard
      globalClient.print(path);                     // specs for this command here: http://ubidots.com/docs/api/index.html
      globalClient.println(" HTTP/1.1");
      globalClient.println(F("Content-Type: application/json"));
      globalClient.print(F("Content-Length: "));
      globalClient.println(le);
      globalClient.print(F("X-Auth-Token: "));
      globalClient.println(token);
      globalClient.print(F("Host: "));
      globalClient.println(server);
      globalClient.println();
      globalClient.println(var);  // The payload defined above
      globalClient.println();
      globalClient.println((char)26); //This terminates the JSON SEND with a carriage return
      LastReport = millis();
    }
    delay(2000); // wait for server response
     while (globalClient){
      Serial.print((char)globalClient.read());
     }
     globalClient.stop();
     LGPS.powerOn();                  // Start the GPS first as it takes time to get a fix
     Serial.println("GPS Powered on, get a new fix. load cell is not working");
  }
}

boolean ParseLocation(const char* GPGGAstr)
// Refer to http://www.gpsinformation.org/dale/nmea.htm#GGA
// Sample data: $GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47
{
  char latarray[6];
  char longarray[6];
  int index = 0;
  //Serial.println(GPGGAstr);
  //Serial.print("Fix Quality: ");
  //Serial.println(GPGGAstr[43]);
  if (GPGGAstr[43]=='0') {        //  This is the place in the sentence that shows Fix Quality 0 means no fix
    //Serial.println("No GPS Fix");
    Location = "";           // No fix then no Location string
    return 0;
  }
  String GPSstring = String(GPGGAstr);
  for (int i=20; i<=26; i++) {         // We have to jump through some hoops here
    latarray[index] = GPGGAstr[i];     // we need to pick out the minutes from the char array
    index++;
  }


  float latdms = atof(latarray);        // and convert them to a float
  float lattitude = latdms/60;          // and convert that to decimal degrees
  String lattstring = String(lattitude);// Then put back into a string
  Location = "{\"lat\":";
  if(GPGGAstr[28] == 'S') Location = Location + "-";
  Location += GPSstring.substring(19,20) + "." + lattstring.substring(2,4);//If lattude beggins with 0 ubidots response is error. the code of latitude fix latidude if beggins with 0
  index = 0;
  for (int i=33; i<=38; i++) {         // And do the same thing for longitude
    longarray[index] = GPGGAstr[i];     // the good news is that the GPS data is fixed column
    index++;
  }
  float longdms = atof(longarray);        // and convert them to a float
  float longitude = longdms/60;          // and convert that to decimal degrees
  String longstring = String(longitude);// Then put back into a string
  Location += " ,\"lng\":";
  if(GPGGAstr[41] == 'W') Location = Location + "-";
  if(GPGGAstr[30] == '0') {
    Location = Location + GPSstring.substring(31,33) + "." + longstring.substring(2,4) + "}";
  }
  else {
    Location = Location + GPSstring.substring(30,33) + "." + longstring.substring(2,4) + "}";
  }
  return 1;
}


long read1() {
  // wait for the chip to become ready
  while (!(digitalRead(4) == LOW)){
    Serial.println("no listo");
  }

  byte data[3];

  // pulse the clock pin 24 times to read the data
  for (byte j = 3; j--;) {
    for (char i = 8; i--;) {
      digitalWrite(5, HIGH);
      bitWrite(data[j], i, digitalRead(4));
      digitalWrite(5, LOW);
    }
  }

   //set the channel and the gain factor for the next reading using the clock pin
  for (int i = 0; i < 1; i++) {
    digitalWrite(5, HIGH);
    digitalWrite(5, LOW);
  }

  data[2] ^= 0x80;

  return ((uint32_t) data[2] << 16) | ((uint32_t) data[1] << 8) | (uint32_t) data[0];
}