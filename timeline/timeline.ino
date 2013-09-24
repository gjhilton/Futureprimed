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
  
  Serial.begin(9600);
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
  Serial.print("run");
}

void goWait(){
  running = false;
  Serial.print("wait");
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
  if (client) {                             // if you get a client,
    Serial.println("new client");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        // Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {  
            writeStatus(client);
            // break out of the while loop:
            break;         
          } 
          else {      // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        }     
        else if (c != '\r') {    // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /go":
        if (currentLine.endsWith("GET /go")) {
          goRun();               // GET /go runs the timeline
        }
        if (currentLine.endsWith("GET /stop")) {
          goWait();               // GET /go stops the timeline
        }
      }
    }
    // close the connection:
    client.stop();

    Serial.println("client disonnected");
  }
}

void writeStatus(EthernetClient client){
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println("<meta http-equiv=\"refresh\" content=\"2; url='/'\">");

  client.println("<body>");
  
  
  if (!running){
    client.println("<h3>Waiting.</h3>");
    client.print("<a href=\"/go\">go</a><br>");
  } else {
    client.println("<h3 style='color:#f00'>Running</h3>");
    client.print("<a href=\"/stop\">stop</a><br>");
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
 client.println();
}

