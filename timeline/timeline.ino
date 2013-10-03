/////////////////////////////////////////////////////////////////////////////////////////////////
// 
// KAZIMIER: FUTUREPRIMED MOTOR CONTROLLER
//
// Motor system is in two halves, runing on two Arduinos.
// This half runs on the board with the Ethernet shield, and provides a web interface to a timeline
// of cues, sending pairs of (duration,speed) to the motor board via SoftEasyTransfer.
// 
////////////////////////////////////////////////////////////////////////////////////////////////

#include <SPI.h>
#include <Ethernet.h>
#include <SoftEasyTransfer.h>
#include <SoftwareSerial.h>

/////////////////////////////////////////////////////////////////////////////////////////////////
// HARDCODED CUE LIST
////////////////////////////////////////////////////////////////////////////////////////////////

#define N_CUES 5

void initCues(){
  // setCue(cue number,  duration in seconds,   final motor speed - max 100);
  setCue(0,  20,  25);
  setCue(1,  120,  25);
  setCue(2,  5,  150);
  setCue(3,  120,   150);
  setCue(4,  30,   25);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// NETWORK CONFIGURATION
////////////////////////////////////////////////////////////////////////////////////////////////

byte mac[] = {0x90, 0xA2, 0xDA, 0x0E, 0x9B, 0x7F};
IPAddress ip(192, 168, 1, 12);
EthernetServer server(80);

/////////////////////////////////////////////////////////////////////////////////////////////////
// SOFT SERIAL TRANSFER
////////////////////////////////////////////////////////////////////////////////////////////////

SoftwareSerial mySerial(16,17);
SoftEasyTransfer ET;

struct SEND_DATA_STRUCTURE{
  int theSpeed;
  int theDuration;
};
SEND_DATA_STRUCTURE mydata;

/////////////////////////////////////////////////////////////////////////////////////////////////
// GLOBALS
////////////////////////////////////////////////////////////////////////////////////////////////

#define IDLE_SPEED 25

int cueSeconds[N_CUES];
int cueValues[N_CUES];
int currentCue, currentSpeed;
long nextCueTime;

typedef enum {runStateStopped, runStateIdle, runStateRunning} runState ;
runState currentRunState = runStateStopped;

/////////////////////////////////////////////////////////////////////////////////////////////////
// ARDUINO LIFESYCLE
////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  Ethernet.begin(mac, ip);                // start ethernet interface
  server.begin();                         // start webserver
  mySerial.begin(9600);                   // start soft serial
  ET.begin(details(mydata), &mySerial);   // start easy soft transfer
  Serial.begin(9600);                     // start serial

initCues();
  goIdle();
}

void loop() {
  webserver();
  if (currentRunState == runStateRunning){
    if (millis() >= nextCueTime) {
      goToCue(currentCue +1); 
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// STATE
////////////////////////////////////////////////////////////////////////////////////////////////

void rampMotor(int speed, int duration){
  /*
  Serial.print("setting speed to ");
  Serial.print(speed);
  Serial.print(" and duration ");
  Serial.print(duration);
  */
  mydata.theSpeed = speed;
  mydata.theDuration = duration;
  ET.sendData();
}

void goToCue(int cue){
  currentCue = cue;
  if (currentCue >= N_CUES){
    goIdle(); // timeline has finished
  } 
  else {
    unsigned long now = millis();
    nextCueTime = now + (cueSeconds[currentCue] * 1000.0); // it's important this is a float, else you end up with integer maths - which overfloweth...
    rampMotor(cueValues[currentCue],cueSeconds[currentCue]);
    /*
    Serial.print(now);
    Serial.print("+");
    Serial.print(cueSeconds[currentCue] * 1000.0);
    Serial.print("=");
    Serial.println(nextCueTime);
    
    Serial.print("Beginning cue:");
    Serial.print(currentCue);
    Serial.print(" at ");
    Serial.print(now);
    Serial.print(" Duration: ");
    Serial.print(cueSeconds[currentCue]);
    Serial.print(" seconds. Next cue at: ");
    Serial.println(nextCueTime);
    */
  }
}

void goRun(){
  Serial.println("run");
  currentRunState = runStateRunning;
  goToCue(0);
}

void goIdle(){
  currentRunState = runStateIdle;
  rampMotor(IDLE_SPEED,1);
  currentSpeed = IDLE_SPEED;
  Serial.println("idle");
}

void goStop(){
  currentRunState = runStateStopped;
  rampMotor(0,1);
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

void webserver(){
  EthernetClient client = server.available();
  if (client) {                             // if you get a client,
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        if (c == '\n') {                    // if the byte is a newline character
          if (currentLine.length() == 0) {  
            servePage(client);
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
        if (currentLine.endsWith("GET /idle")) {
          goIdle();               // GET /idle stops the timeline
        }
        if (currentLine.endsWith("GET /stop")) {
          goStop();               // GET /stop stops the timeline
        }
      }
    }
    client.stop();
  }
}

void servePage(EthernetClient client){
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");
  client.println("<head><meta http-equiv=\"refresh\" content=\"2; url='/'\">");
  client.println("<style>body{background:#333;color:#fff;font-family:arial}a,td,th{padding:10px;text-align:center}a,a:visited {font-weight:bold;display:block;width:inherit;color:#fff;text-decoration:none;}table{width:100%}a,table{border:2px solid #000}.g{background:#0b0}.r{background:#e00}</style></head>");
  client.println("<body>");

  if (currentRunState == runStateStopped){
    printStatus(client, "STOPPED");
    printIdle(client);
  } 
  else if (currentRunState == runStateRunning) {
    printStatus(client, "RUNNING");
    printIdle(client);
    printStop(client);
  } else {
    // ie currentRunState == runStateIdle
    printStatus(client, "READY");
    printRun(client);
    printStop(client);
  }

  client.println("<table cellspacing=0>");
  client.print("<tr style='background:#000;'><th>duration</th><th>end speed</th></tr>");
  for (int i= 0; i< N_CUES; i++) {
    client.print("<tr");
    if (thisRowActive(i)){
      client.print(" class=\"g\" ");
    }
    client.print("><td>");
    if (thisRowActive(i)){
      client.print((nextCueTime - millis())/1000);
      client.print("s remaining");
    } else {
      client.print(cueSeconds[i]);
    }
    client.print("</td><td>");
    client.print(cueValues[i]);
    client.println("</td><tr>"); 
  }
  client.println("</table></body></html>");
  client.println();
}

void printStatus(EthernetClient client, String s){
  client.println("<h3>Status:");
  client.println(s);
  client.println("</h3>");
}

void printRun(EthernetClient client){
  client.print("<p><a class=\"g\" href=\"/go\">go</a></p>");
}

void printIdle(EthernetClient client){
  client.print("<p><a href=\"/idle\">ready</a></p>");
}

void printStop(EthernetClient client){
  client.print("<p><a class=\"r\" href=\"/stop\">stop</a></p>");
}

boolean thisRowActive(int i){
  return ((currentRunState == runStateRunning) && (i==currentCue));
}


