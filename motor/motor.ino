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
#include "interpolation.h"

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
const int MAX_SPEED = 200;

/////////////////////////////////////////////////////////////////////////////////////////////////
// STATE
////////////////////////////////////////////////////////////////////////////////////////////////

boolean brakeIsOn = true;
int currentSpeed = 0;

boolean rampRunning = false;
float rampStartSpeed;
float rampSpeedChange;

unsigned long rampStartMillis;
float rampDurationSeconds;

/////////////////////////////////////////////////////////////////////////////////////////////////
// ARDUINO LIFECYCLE
////////////////////////////////////////////////////////////////////////////////////////////////

void setup(){
  //Serial.begin(9600);
  mySerial.begin(9600);
  ET.begin(details(mydata), &mySerial); 
  md.init();
  pinMode(BRAKEPIN,OUTPUT);
  brakeOn();
}

void loop(){
  if (rampRunning){
    unsigned long elapsedMillis = millis() - rampStartMillis;
    float elapsedSeconds = elapsedMillis/1000.0;
    //Serial.print(elapsedSeconds);
    //Serial.print(":");
    if (elapsedSeconds >= rampDurationSeconds){
      setMotorSpeed(rampStartSpeed + rampSpeedChange);
      rampRunning = 0;
    } else {
      setMotorSpeed(int(interpolate(elapsedSeconds, rampStartSpeed, rampSpeedChange, rampDurationSeconds, LINEAR)));
    }
  }
  
  if(ET.receiveData()){
    cue(mydata.theSpeed, mydata.theDuration);
  }
  
  delay(50);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// COMMUNICATIONS
////////////////////////////////////////////////////////////////////////////////////////////////

void cue(int targetSpeed, int duration){
  rampDurationSeconds = float(duration);
  rampStartSpeed = float(currentSpeed);
  rampSpeedChange = float(targetSpeed - rampStartSpeed);
  rampStartMillis = millis();
  rampRunning = true;
  /*
  Serial.print("Will accelerate from ");
  Serial.print(rampStartSpeed);
  Serial.print(" to ");
  Serial.print(rampStartSpeed + rampSpeedChange);
  Serial.print(" over ");
  Serial.print(rampDurationSeconds);
  Serial.print(" seconds, beginning ");
  Serial.println(rampStartMillis);
  */
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// MOTOR DRIVING
////////////////////////////////////////////////////////////////////////////////////////////////

void setMotorSpeed(int s){
  //Serial.print("Setting speed to ");
  //Serial.println(s);
  
  // constrain to safe speed range
  if (s >= MAX_SPEED) s = MAX_SPEED;
  if (s <= 0-MAX_SPEED) s = 0-MAX_SPEED;
  if ((s != 0) && brakeIsOn) brakeOff();
  
  md.setM1Speed(s);
  currentSpeed = s;
  if ((s == 0) && !brakeIsOn) brakeOn();
}

void brakeOff(){
    digitalWrite(3,HIGH);
    brakeIsOn = 0;
}

void brakeOn(){
    digitalWrite(3,LOW);
    brakeIsOn = 1;
}


