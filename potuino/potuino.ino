/*************************************************** 
  Designed specifically to work with the Adafruit WiFi products:
  ----> https://www.adafruit.com/products/1469

  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried & Kevin Townsend for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
  
  Modified by Peter Dalmaris tas part of Udemy's Arduino step by
  Step course and modified again by One of Peter's students.
  
 ****************************************************/
//include the I2C Wire library - needed for communication with the I2C chip attached to the LCD manual 
#include <Wire.h> 
// include the RobotGeekLCD library
#include "RobotGeekLCD.h"
#include "RFIDuino.h"
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include <utility/debug.h>

#define WLAN_SSID "ARCHER_SLOW_2G"             
#define WLAN_PASS "smoothpiano245"

#define WEBSITE   "noufattole.interns.kit.cm"   //Doesn't like an IP here
//#define WEBSITEIP IPAddress(172,16,100,186)
#define WEBPORT   80
#define WEBPAGE   "/"               //A file with "led8=1\n" as the only content

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
uint32_t ip;
Adafruit_CC3000_Client www;

RFIDuino myRFIDuino(1.2);   //initialize an RFIDuino object for hardware version 1.2

byte tagData[5]; //Holds the ID numbers from the tag
char id[50] = {0};  
int connectTimeout = 3000;
int repeat_counter = 0; 
// create a robotgeekLCD object named 'lcd' 
RobotGeekLCD lcd;

void setup(void)
{
  // initialize the lcd object
  lcd.init();
  lcd.clear();
  Serial.begin (9600);
  Serial.println ("Hello, CC3000!\n");
  lcd.print("Initializing");
  Serial.print(id);
  Serial.print ("Free RAM: "); 
  Serial.println (getFreeRam(), DEC);
  Serial.println ("\nInitializing...");
  if (!cc3000.begin())
  {
    Serial.println("Couldn't begin()! Check your wiring?");
    while(1);
  }

  listSSIDResults();
  lcd.clear();
   lcd.print("Connecting to");
   lcd.setCursor(0,1);
   lcd.print(WLAN_SSID);
  Serial.print("\nAttempting to connect to "); 
  Serial.println(WLAN_SSID);
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) 
  {
    Serial.println("Failed!");
    while(1);
  }
  
  Serial.println("Connected!");
  myRFIDuino.successSound();
  
  Serial.println("Request DHCP");
  lcd.clear();
   lcd.print("Request DHCP");
  while (!cc3000.checkDHCP())
  {
    delay(1000);                                    // ToDo: Insert a DHCP timeout!  Why??
  }  
  Serial.println("DHCP Success");
  while (! displayConnectionDetails()) 
  {
    delay(1000);
  }
  lcd.clear();
   lcd.print("Welcome!");
   lcd.setCursor(0,1);
   lcd.print("Please swipe tag");
  Serial.println("Welcome. Please swipe your RFID Tag.");
}

void loop()
{
  //scan for a tag - if a tag is sucesfully scanned, return a 'true' and proceed
  myRFIDuino.scanForTag(tagData);
  if(myRFIDuino.scanForTag(tagData) == true)
  {
    digitalWrite(myRFIDuino.led2,HIGH);     //turn green LED on
    digitalWrite(myRFIDuino.buzzer, HIGH);   //turn the buzzer on
    delay(250);                             //wait for 1 second
    digitalWrite(myRFIDuino.buzzer, LOW);    //turn the buzzer off
    digitalWrite(myRFIDuino.led2,LOW);      //turn the green LED off
    Serial.print("RFID Tag ID:"); //print a header to the Serial port.
    lcd.clear();
   lcd.print("RFID Tag ID");
   lcd.setCursor(0,1);
   
    for(int n=0;n<5;n++)
    {
      lcd.print(tagData[n],DEC);
      Serial.print(tagData[n],DEC);  //print the byte in Decimal format
      if(n<4)//only print the comma on the first 4 nunbers
      {
        Serial.print(",");
        lcd.print(",");
      }
    }
    Serial.print("\n\r");//return character for next line
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
  Serial.print(WEBSITE); 
  Serial.print(F(" -> "));
  while (ip == 0) 
  {
    //if (!isIP(WEBSITE, &ip) && !cc3000.getHostByName(WEBSITE, &ip))  // Something like this would allow WEBSITE to be an IP
    if (!cc3000.getHostByName(WEBSITE, &ip))
      {
        Serial.println(F("Couldn't resolve!"));
      }
    delay(500);
  }
  cc3000.printIPdotsRev(ip);

  Adafruit_CC3000_Client www = cc3000.connectTCP(ip, WEBPORT);
  
  if (www.connected()) 
  {
    Serial.print("POST ");
    Serial.print(WEBPAGE);
    Serial.print(" HTTP/1.1\r\n");
    Serial.print("Host: "); 
    Serial.print(WEBSITE); 
    Serial.print("\r\n");
    Serial.print("Accept: */*\r\n");
    Serial.print("User-Agent: Potuino\r\n");
    Serial.print("Accept-Encoding: gzip, deflate\r\n");
    Serial.print("Content-Length: 26\r\n"); //IMPORTANT
    Serial.print("Content-Type: application/json\r\n");
    Serial.print("\r\n");
    Serial.print("{\"RFID\":\"");
    Serial.print(id);
    Serial.print("\"}");
    Serial.println();
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
    www.fastrprint(F("Content-Length: 30\r\n")); //IMPORTANT
    www.fastrprint(F("Content-Type: application/json\r\n"));// www.fastrprint(F("Content-Type: application/text\r\n"));
    //www.fastrprint(F("Cookie: "+ COOKIE +"\r\n")); //sets cookie
    www.fastrprint(F("\r\n"));
    www.fastrprint(F("{\"RFID\":\""));
    www.fastrprint(id);
    www.fastrprint(F("\"}"));
    www.println();
    Serial.println(F("Request sent"));
  } 
  else 
  {
    Serial.println(F("\nConnection failed"));    
    return;
  }

  Serial.println(F("\n---------------------------------------------"));
  Serial.println(F("HTTP Response:"));
  
  unsigned long lastRead = millis();
  
  while (www.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) 
  {
    String line = "";
    boolean currentLineIsBlank = true;
    get_request = "";     
    boolean sentContent = false;
    //lcd.clear();
    while (www.available()) 
    { 
      char c = www.read();
      Serial.print(c);
      line += c;
      
   //lcd.print(c);
      lastRead = millis ();
 
      if(reading && c == '\n') 
      { 
        reading = false;  
        parseGetRequest(get_request);
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
   Serial.println(F("\n----------------------------------------------"));
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
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}

String parseGetRequest (String &str) 
  {
  Serial.print (F ("Parsing this string:"));
  Serial.println (str);
  int led_index = str.indexOf ("led");
  int led_pin = str [led_index + 3] - '0';
  int led_val = str [led_index + 5] - '0';
  if ((led_pin == 8) && (led_val == 1)){
    digitalWrite(myRFIDuino.led1, HIGH);
  }
  //executeInstruction (led_pin, led_val);
  //executeInstruction (13, led_val); // Code above looks to pull just one character for the pin
}

void executeInstruction (int pin, int val)
{   Serial.println(F("Executing instruction"));
    pinMode(pin, OUTPUT);
    digitalWrite(pin, val);
    delay(5000);
    Serial.println(F("Done!"));
}

void listSSIDResults(void)
{
  uint32_t index;
  uint8_t valid, rssi, sec;
  char ssidname[33]; 

  if (!cc3000.startSSIDscan(&index))
  {
    Serial.println(F("SSID scan failed!"));
    return;
  }

  Serial.print(F("Networks found: ")); Serial.println(index);
  Serial.println(F("================================================"));

  while (index) 
  {
    index--;
    valid = cc3000.getNextSSID(&rssi, &sec, ssidname);
    Serial.print(F("SSID Name    : ")); Serial.print(ssidname);
    Serial.println();
    Serial.print(F("RSSI         : "));
    Serial.println(rssi);
    Serial.print(F("Security Mode: "));
    Serial.println(sec);
    Serial.println();
  }
  Serial.println(F("================================================"));

  cc3000.stopSSIDscan();
}

