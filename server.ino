#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

unsigned int localPort = 2390;
IPAddress timeServerIP;
const char* ntpServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48;
byte packetBuffer[ NTP_PACKET_SIZE];
WiFiUDP udp;
unsigned int hour = 0;
unsigned int minute = 0;
unsigned int second = 0;
unsigned int timediff = 4; // set to 4 or 5 depending on Day lighy savings
bool nightMode, nightMode_prev;
unsigned int nightMode_on_h = 19;
unsigned int nightMode_on_m = 0;

unsigned int nightMode_off_h = 23;
unsigned int nightMode_off_m = 0;

const char* ssid = "The Love Shack";
const char* password = "111broce";

int r;
int g;
int b;
int sw;

int r_night;
int g_night;
int b_night;

int pR;
int pG;
int pB;
int pSw;
int blue_led;
int red_led;
// Substitute "your-ssid" and "your-password" with your home connection ssid and password.
WiFiServer server(80);

void setup() {
  r = r_night = 0;
  g = g_night = 50;
  b = b_night = 0;
  sw = 0;
  
  pR = 4;
  pG = 5;
  pB = 12;
  pSw = 0;
  blue_led = 2;
  red_led = 0;
  nightMode = false;
  nightMode_prev = false;
  Serial.begin(9600);
  delay(10);

  // prepare GPIO2
  pinMode(pSw, OUTPUT);
  //digitalWrite(pSw, 1);

  pinMode(pR, OUTPUT);
  //digitalWrite(pR, 1);

  pinMode(pG, OUTPUT);
  //digitalWrite(pG, 1);

  pinMode(pB, OUTPUT);
  //digitalWrite(pB, 1);
  
  // Connect to WiFi network
  Serial.println();
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
  
  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());

  // setup the UDP Connection
  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
}

void loop() {

  //get the time
  GetTime(hour,minute,second);
  Serial.print("Time is: ");
  Serial.print(hour);
  Serial.print(":");
  Serial.print(minute);
  Serial.print(":");
  Serial.println(second);


  if((hour >= nightMode_on_h) && (hour <= nightMode_off_h))
  {
    // if it is the hour we start, we will need to check the minutes
    if(hour == nightMode_on_h)
    {
      //if we are are within the minutes
      if(minute >= nightMode_on_m)
        SetNightMode();
      else
        StopNightMode();
    }
    // if it is the hour we stop, we will need to check the minutes
    if(hour == nightMode_off_h)
    {
      //if we are are within the minutes
      if(minute <= nightMode_off_m)
        SetNightMode();
      else
        StopNightMode();
    }
    else
      SetNightMode();
  }
  else
    StopNightMode();
  
  
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  
  // Wait until the client sends some data
  Serial.println("new client");
  while(!client.available()){
    delay(1);
  }

  
  
  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();

  int command = req.substring(0,1).toInt();
  switch(command)
  {
	  case 0: //change color
		  if(req.length() == 15)
		  {
			r = req.substring(2,5).toInt();
			g = req.substring(6,9).toInt();
			b = req.substring(10,13).toInt();
			sw = req.substring(14).toInt();
			r_night = r;
			g_night = g;
			b_night = b;
			SetPins(); 
		  }
	  break;
	  case 1: //update night mode hours
		if(req.length() == 14)
		{
			nightMode_on_h = req.substring(2,4).toInt();
			nightMode_on_m = req.substring(5,7).toInt();
			nightMode_off_h = req.substring(8,10).toInt();
			nightMode_off_m = req.substring(11).toInt();
		}
		break;
		case 2: //update daylight savings
		if(req.length() == 4)
		{
			if(req.substring(2).toInt())
				timediff = 4;
			else
				timediff = 5;
		}
	  
  }
  

  SetPins();
  
    

}
void StopNightMode()
{
  nightMode = false;
  if(nightMode != nightMode_prev) 
  {
    r = 0;
    g = 0;
    b = 0;
    SetPins();
    Serial.print("stopping nightmode, hour is ");
    Serial.println(hour);
  }
  nightMode_prev = nightMode;
    
}
void SetNightMode()
{
  nightMode = true;
  if(nightMode != nightMode_prev)
  {
    r = r_night;
    g = g_night;
    b = b_night;
    SetPins();
    Serial.print("setting nightmode, hour is ");
    Serial.println(hour);
  }
  nightMode_prev = nightMode;
}

void SetPins()
{
  Serial.println("setting pins:");
  Serial.print("R is ");
  Serial.print(r);
  Serial.print("\n");
  Serial.print("G is ");
  Serial.print(g);
  Serial.print("\n");
  Serial.print("B is ");
  Serial.print(b);
  Serial.print("\n");
  
  digitalWrite(pSw,sw);
  analogWrite(pR,r);
  analogWrite(pG,g);
  analogWrite(pB,b);  
}

void GetTime(unsigned int &_hour,unsigned int &_minute,unsigned int &_second)
{
   WiFi.hostByName(ntpServerName, timeServerIP); 

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);
  
  int cb = udp.parsePacket();
  if (cb)
  {
    //Serial.print("packet received, length=");
    //Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    //Serial.print("Seconds since Jan 1 1900 = " );
    //Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    //Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    const unsigned long yearSeconds = 31557600UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    unsigned long jan_1_2016 = 1451606400UL;
    unsigned long epoch2 = epoch - jan_1_2016;
    
    // print Unix time:
    //Serial.println(epoch);
    unsigned long year = (epoch2/yearSeconds) + 2016;
    //month = ((year - 1970)*year - epoch)

    // print the hour, minute and second:
    //Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    //Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    hour = ((epoch  % 86400L) / 3600);
    unsigned int overlap_fixer;
    switch(timediff-4)
    {
      case 0:
        overlap_fixer = 20;
      break;
      case 1:
        overlap_fixer = 19;
      break;
    }
    if(hour >= timediff)
        hour = hour - timediff;
    else
        hour = hour + overlap_fixer;
         
    //Serial.print(':');
    
    //Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    minute = (epoch  % 3600) / 60;
    //Serial.print(':');
    second = epoch % 60;
    //Serial.println(epoch % 60); // print the second
  }
  
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  //Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}
