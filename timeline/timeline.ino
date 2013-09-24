#include <SPI.h>
#include <Ethernet.h>
#include <ArdOSC.h>
// #include <SoftEasyTransfer.h>
#include <SoftwareSerial.h>

/////////////////////////////////////////////////////////////////////////////////////////////////
// HARDCODED CUES
////////////////////////////////////////////////////////////////////////////////////////////////

#define N_CUES 5

void initCues(){
  setCue(0,1,0);
  setCue(1,10,10);
  setCue(2,5,30);
  setCue(3,60,30);
  setCue(4,5,0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// NETWORK CONFIGURATION
////////////////////////////////////////////////////////////////////////////////////////////////

byte mac[] = {0x90, 0xA2, 0xDA, 0x0E, 0x9B, 0x7F};
IPAddress ip(192, 168, 1, 12);
EthernetServer server(80);

/////////////////////////////////////////////////////////////////////////////////////////////////
// GLOBALS
////////////////////////////////////////////////////////////////////////////////////////////////

int cueSeconds[N_CUES];
int cueValues[N_CUES];

boolean running;
int currentCue;
unsigned long currentTime;
SoftwareSerial mySerial(16,17);
// SoftEasyTransfer ET; 
OSCServer listener;

/////////////////////////////////////////////////////////////////////////////////////////////////
// ARDUINO LIFESYCLE
////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  initCues();
  goWait();
  Ethernet.begin(mac, ip);
  server.begin();
  //Serial.begin(9600);
  //Serial.print("server is at ");
  //Serial.println(Ethernet.localIP());
}


void loop() {
  currentTime = millis();
  webserver();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// STATE
////////////////////////////////////////////////////////////////////////////////////////////////

void goRun(){
  running = true;
}

void goWait(){
  running = false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// CUE SETTING
////////////////////////////////////////////////////////////////////////////////////////////////

void setCue(int index, int seconds, int value){
  cueSeconds[index] = seconds;
  cueValues[index] = value;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// STATUS REPORTING
////////////////////////////////////////////////////////////////////////////////////////////////

void webserver(){
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        if (c == '\n' && currentLineIsBlank) {
          displayStatus(client);
          break;
        }
        if (c == '\n') {
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }
    // delay(1);
    client.stop();
    Serial.println("client disonnected");
  }
}

void displayStatus(EthernetClient client){
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println("<meta http-equiv=\"refresh\" content=\"2\">");

  client.println("<body>");
  
  
  if (!running){
    client.println("<h3>Waiting</h3>");
  } else {
    client.println("<h3 style='color:#f00'>Running</h3>");
  }
 
  client.println("<table>");
  for (int i= 0; i< N_CUES; i++) {
    client.print("<tr><td>");
    client.print(cueSeconds[i]);
    client.print("</td><td>");
    client.print(cueValues[i]);
    client.println("</td><tr>"); 
  }
  client.println("</table>");
  client.println("</body></html>");

}

