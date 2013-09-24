#include <SPI.h>
#include <Ethernet.h>
#include <ArdOSC.h>
// #include <SoftEasyTransfer.h>
#include <SoftwareSerial.h>

/////////////////////////////////////////////////////////////////////////////////////////////////
// HARDCODED CUES
////////////////////////////////////////////////////////////////////////////////////////////////

#define N_CUES 4

void initCues(){
  setCue(0,  20,   5);
  setCue(1,  5,   30);
  setCue(2,  30,  30);
  setCue(3,  5,   0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// NETWORK CONFIGURATION
////////////////////////////////////////////////////////////////////////////////////////////////

byte mac[] = {
  0x90, 0xA2, 0xDA, 0x0E, 0x9B, 0x7F};
IPAddress ip(192, 168, 1, 12);
EthernetServer server(80);
SoftwareSerial mySerial(16,17);

/////////////////////////////////////////////////////////////////////////////////////////////////
// GLOBALS
////////////////////////////////////////////////////////////////////////////////////////////////

int cueSeconds[N_CUES];
int cueValues[N_CUES];
boolean running;
int currentCue, currentSpeed;
unsigned long nextCueTime;
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
}

void loop() {
  webserver();
  if (running){
    unsigned long timeNow = millis();
    if (timeNow >= nextCueTime) {
      goToCue(currentCue +1); 
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// STATE
////////////////////////////////////////////////////////////////////////////////////////////////

void setMotorSpeed(int speed){
  Serial.print("setting speed to ");
  Serial.print(speed);
}

void goToCue(int cue){
  currentCue = cue;
  if (currentCue >= N_CUES){
    goWait(); // timeline has finished
  } 
  else {
    unsigned long now = millis();
    nextCueTime = now + (cueSeconds[currentCue] * 1000);
    Serial.print("Beginning cue:");
    Serial.print(currentCue);
    Serial.print(" at ");
    Serial.print(now);
    Serial.print(" Duration: ");
    Serial.print(cueSeconds[currentCue]);
    Serial.print(" seconds. Next cue at: ");
    Serial.println(nextCueTime);
  }
}

void goRun(){
  Serial.println("run");
  running = true;
  goToCue(0);
}

void goWait(){
  running = false;
  currentSpeed = 0;
  Serial.println("wait");
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

void serialStatus(){
  Serial.print("Time:");
  Serial.println(millis());
}

void webserver(){
  EthernetClient client = server.available();
  if (client) {                             // if you get a client,
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        if (c == '\n') {                    // if the byte is a newline character
          if (currentLine.length() == 0) {  
            writeStatus(client);
            break;         
          } 
          else {
            currentLine = "";
          }
        }     
        else if (c != '\r') {    // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
        if (currentLine.endsWith("GET /go")) {
          goRun();               // GET /go runs the timeline
        }
        if (currentLine.endsWith("GET /stop")) {
          goWait();               // GET /stop stops the timeline
        }
      }
    }
    client.stop();
  }
}

void writeStatus(EthernetClient client){
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println("<head><meta http-equiv=\"refresh\" content=\"2; url='/'\"><style>body{font-family:helvetica;}td,th{padding:10px;text-align:center;}</style></head>");
  client.println("<body>");

  if (!running){
    client.println("<h3>Status: ready</h3>");
    client.print("<p><a href=\"/go\">go</a></p>");
  } 
  else {
    client.println("<h3>Status: <span style='color:#0c0'>running</span></h3>");
    client.print("<p><a href=\"/stop\">stop</a></p>");
    /*
    client.println("<h4>Motor:");
     client.println(currentSpeed);
     client.println("</h4>");
     
     client.println("<h4>Cue:");
     client.println(currentCue);
     client.println("</h4>");
     
     client.println("<h4>Time:");
     client.println(millis());
     client.println("</h4>");
     
     client.println("<h4>Next cue at:");
     client.println(nextCueTime);
     client.println("</h4>");
     */
  }

  client.println("<table cellspacing=0 cellpadding=0 style='width:100%; border:1px solid grey;'>");
  client.print("<tr style='background:#eee;'><th>duration</th><th>end speed</th></tr>");
  for (int i= 0; i< N_CUES; i++) {
    client.print("<tr");
    if (running && i==currentCue){
      client.print(" style='background:#aaa;' ");
    }
    client.print("><td>");
    client.print(cueSeconds[i]);
    client.print("</td><td>");
    client.print(cueValues[i]);
    client.println("</td><tr>"); 
  }
  client.println("</table>");

  client.println("</body></html>");
  client.println();
}



