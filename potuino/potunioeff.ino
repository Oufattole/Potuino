#include <Wire.h> 
#include "RobotGeekLCD.h"
#include "RFIDuino.h"
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include <utility/debug.h>

#define WLAN_SSID "ARCHER_SLOW_2G"             
#define WLAN_PASS "smoothpiano245"

#define WEBSITE   "noufattole.interns.kit.cm"   
#define WEBPORT   80
#define WEBPAGE   "/"      

#define ADAFRUIT_CC3000_IRQ   3                     
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER);
boolean reading = false; 
String  get_request  = "";   

#define WLAN_SECURITY   WLAN_SEC_WPA2
#define IDLE_TIMEOUT_MS  3000     
String cookie = "";
String cookiep1 = "";
String cookiep2 = "";
uint32_t ip;
Adafruit_CC3000_Client www;

RFIDuino myRFIDuino(1.2);   

byte tagData[5]; 
char id[50] = {0};  
int connectTimeout = 3000;
int repeat_counter = 0; 

RobotGeekLCD lcd;

void setup(void)
{
  lcd.init();
  lcd.clear();
  lcd.print("Initializing");
  if (!cc3000.begin())
  {
    while(1);
  }

  listSSIDResults();
  lcd.clear();
   lcd.print("Connecting");
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) 
  {
    while(1);
  }
  
  myRFIDuino.successSound();
  
  while (!cc3000.checkDHCP())
  {
    delay(1000);                                    
  }  
  while (! displayConnectionDetails()) 
  {
    delay(1000);
  }
  lcd.clear();
    lcd.setCursor(0,0);
     lcd.print("Scan book:return");
     lcd.setCursor(0,1);
     lcd.print("Or tag+book:take");
}

void loop()
{
    
  myRFIDuino.scanForTag(tagData);
  if(myRFIDuino.scanForTag(tagData) == true)
  {
    digitalWrite(myRFIDuino.led2,HIGH);     
    digitalWrite(myRFIDuino.buzzer, HIGH);   
    delay(250);                             
    digitalWrite(myRFIDuino.buzzer, LOW);    
    digitalWrite(myRFIDuino.led2,LOW);      
   
    for(int n=0;n<5;n++)
    {
      if(n<4)
      {
        
      }
    }
    lcd.clear();
    lcd.setCursor(0,0);
     lcd.print("Successful Scan");
     lcd.setCursor(0,1);
     lcd.print("Processing");
    format_tag(tagData);
    txInput();
  }
}

void txInput()
{
  #ifdef WEBSITEIP
  ip = WEBSITEIP;
  #else
  ip = 0;
  #endif
  while (ip == 0) 
  {
    
    if (!cc3000.getHostByName(WEBSITE, &ip))
      {
      }
    delay(500);
  }
  cc3000.printIPdotsRev(ip);

  Adafruit_CC3000_Client www = cc3000.connectTCP(ip, WEBPORT);
  if (www.connected()) 
  {
    boolean sentContent = false;
    www.fastrprint(F("POST "));
    www.fastrprint(WEBPAGE);
    www.fastrprint(F(" HTTP/1.1\r\n"));
    www.fastrprint(F("Host: ")); 
    www.fastrprint(WEBSITE); 
    www.fastrprint(F("\r\n"));
    www.fastrprint(F("Accept: */*\r\n"));
    www.fastrprint(F("User-Agent: Potuino\r\n"));
    www.fastrprint(F("Accept-Encoding: gzip, deflate\r\n"));
    www.fastrprint(F("Content-Length: 30\r\n")); 
    www.fastrprint(F("Content-Type: application/json\r\n"));
    if(cookie!="")
    {
      www.fastrprint(F("Cookie: mojolicious="));
      www.fastrprint(cookie.c_str());
      www.fastrprint(cookiep1.c_str());
      www.fastrprint(cookiep2.c_str());
      www.fastrprint(F("\r\n"));
    }
    www.fastrprint(F("\r\n"));
    www.fastrprint(F("{\"RFID\":\""));
    www.fastrprint(id);
    www.fastrprint(F("\"}"));
    www.println();
    cookie = "";
    cookiep1 = "";
    cookiep2 = "";
  } 
  else 
  {
    return;
  }

  unsigned long lastRead = millis();
  String line = "";
  while (www.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) 
  {
    
    boolean currentLineIsBlank = true;
    get_request = "";     
    boolean sentContent = false;
    while (www.available()) 
    { 
      char c = www.read();
      line += c;
      
      lastRead = millis ();
 
      if(reading && c == '\n') 
      { 
        reading = false;  
        break;
      }        

      if(reading)
      { 
        get_request += c;
      }   
           
      if (reading && c=='\n')
      {
        break; 
      }  
           
      if (c == '\n' && currentLineIsBlank) 
      {
        reading = true; 
      }
      
      if (c == '\n') 
      {
        currentLineIsBlank = true;
      }
       
      else if (c != '\r') 
      {
        currentLineIsBlank = false;
      }
    }
  }
   www.close();
   int x = line.indexOf("Set-Cookie:");
   if(line.indexOf("logged in")>0)  
   {
    lcd.clear();
    lcd.print("logged in");
    cookie = line.substring(x+24,x+24+80);
    cookiep1 = line.substring(x+24+80,x+24+150);
    cookiep2 = line.substring(x+24+150,x+24+191);
   }
   else
   {
       int p2=line.lastIndexOf("\"");
       int p1 = line.substring(0,p2).lastIndexOf("\"")+1;
       String resp = line.substring(p1,p2);
       lcd.clear();
       lcd.print(resp);
   }
   line = "";

}

void format_tag(byte array[]){
  int i, temp;
  for (i = 0; i < 20; i++){
    id[i] = 0;
  }
  char temp1, temp2, temp3;
  for (i = 0; i < 5; i++){
    if(array[i] < 100){
      if(array[i] < 10){
        id[(i * 4)] = '0';
        id[(i * 4) + 1] = '0';
        id[(i * 4) + 2] = (array[i] + '0');
      }
      else {
        temp = array[i];
        id[(i * 4)] = '0';
        id[(i * 4) + 2] = (temp % 10) + '0';
        temp /= 10;
        id[(i * 4) + 1] = (temp % 10) + '0';
      } 
    }
    else {
      temp = array[i];
      id[(i * 4) + 2] = (temp % 10) + '0';
      temp /= 10;
      id[(i * 4) + 1] = (temp % 10) + '0';
      temp /= 10;
      id[(i * 4)] = (temp % 10) + '0';
    }
    if (i < 4){
      id[(i * 4) + 3] = '.';
    }
  }
}

bool displayConnectionDetails (void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    return false;
  }
  else
  {
    return true;
  }
}

String parseGetRequest (String &str) 
  {
  int led_index = str.indexOf ("led");
  int led_pin = str [led_index + 3] - '0';
  int led_val = str [led_index + 5] - '0';
  if ((led_pin == 8) && (led_val == 1)){
    digitalWrite(myRFIDuino.led1, HIGH);
  }
 }

void executeInstruction (int pin, int val)
{   
    pinMode(pin, OUTPUT);
    digitalWrite(pin, val);
    delay(5000);
    
}

void listSSIDResults(void)
{
  uint32_t index;
  uint8_t valid, rssi, sec;
  char ssidname[33]; 

  if (!cc3000.startSSIDscan(&index))
  {
    return;
  }


  while (index) 
  {
    index--;
    valid = cc3000.getNextSSID(&rssi, &sec, ssidname);
    
  }
  

  cc3000.stopSSIDscan();
}
