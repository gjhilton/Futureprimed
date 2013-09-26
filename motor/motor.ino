/////////////////////////////////////////////////////////////////////////////////////////////////
// 
// KAZIMIER: FUTUREPRIMED MOTOR CONTROLLER
//
// Motor system is in two halves, runing on two Arduinos.
// This half runs on the board with the motor shield.
// 
////////////////////////////////////////////////////////////////////////////////////////////////

#include <SoftEasyTransfer.h>
#include <SoftwareSerial.h>
#include "DualVNH5019MotorShield.h"

/////////////////////////////////////////////////////////////////////////////////////////////////
// SOFT SERIAL TRANSFER
////////////////////////////////////////////////////////////////////////////////////////////////

SoftwareSerial mySerial(16,17);
SoftEasyTransfer ET; 
struct RECEIVE_DATA_STRUCTURE{
  int theSpeed;
  int theDuration;
};
RECEIVE_DATA_STRUCTURE mydata;

/////////////////////////////////////////////////////////////////////////////////////////////////
// MOTOR SHIELD
////////////////////////////////////////////////////////////////////////////////////////////////

DualVNH5019MotorShield md;
const int BRAKEPIN = 3;
const int MAX_SPEED = 110;

/////////////////////////////////////////////////////////////////////////////////////////////////
// STATE
////////////////////////////////////////////////////////////////////////////////////////////////

const boolean USE_BRAKE = 1; 
boolean brakeIsOn = true;
int currentSpeed = 0;

unsigned long cueEndTime;
boolean cueRunning = false;
boolean shouldCue = false;

/////////////////////////////////////////////////////////////////////////////////////////////////
// ARDUINO LIFECYCLE
////////////////////////////////////////////////////////////////////////////////////////////////

void setup(){
  Serial.begin(9600);
  mySerial.begin(9600);
  ET.begin(details(mydata), &mySerial); 
  md.init();
  pinMode(BRAKEPIN,OUTPUT);
  brakeOn();
}

void loop(){
  cueCheckEnd();
  if(ET.receiveData()){
    Serial.println("Got cue");
    cue(mydata.theSpeed, mydata.theDuration);
  }
  delay(250);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// MOTOR DRIVING
////////////////////////////////////////////////////////////////////////////////////////////////

void setMotorSpeed(int s){
  Serial.print("Setting speed to ");
  Serial.println(s);
  // constrain to safe speed range
  if (s >= MAX_SPEED) s = MAX_SPEED;
  if (s <= 0-MAX_SPEED) s = 0-MAX_SPEED;
  if ((s != 0) && brakeIsOn) brakeOff();
  // set speed
  md.setM1Speed(s);
  // apply brake if required
  if ((s == 0) && !brakeIsOn) brakeOn();
}

void cue(int s, int d){
  Serial.print("Got cue: will drive at ");
  Serial.print(s);
  Serial.print(" for ");
  Serial.print(d);
  Serial.println(" milliseconds");
  cueRunning = true;
  cueEndTime = millis() + d;

  setMotorSpeed(s);
}

void cueEnd(){
  setMotorSpeed(0);
  cueRunning = false;
}

void cueCheckEnd(){
  if (cueRunning){
    if (millis() > cueEndTime){
      cueEnd();
    }   
  }
}

void brakeOff(){
  if (USE_BRAKE){
    Serial.println("brake off");
    digitalWrite(3,HIGH);
    brakeIsOn = 0;
  }
}

void brakeOn(){
  if (USE_BRAKE){
    Serial.println("brake on");
    digitalWrite(3,LOW);
    brakeIsOn = 1;
  }
}


