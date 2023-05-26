#include <Arduino.h>
#include "DBH12-bridge.hpp"
/*
  DBH-12 H-Bridge Demo
  dbh-12-demo.ino
  Demonstrates operation of DBH-12 Dual H-Bridge Motor Driver
    
  DroneBot Workshop 2022
  https://dronebotworkshop.com
*/

// Motor Connections (All must use PWM pins)
#define IN1_A 12
#define IN2_A 13
#define CT_A 36
#define EN_A 14



#define IN1_B 2
#define IN2_B 17

// Define a fixed speed - do not exceed 250
int fixedSpeed = 80;

DBH12Bridge motor = { IN1_A, IN2_A, EN_A, CT_A};


void setup() {

  Serial.begin(115200);
  printf("start setup\n");
  motor.init();
   printf("Set motor & enable connections as outputs\n");
  pinMode(IN1_A, OUTPUT);
  pinMode(IN1_B, OUTPUT);
  pinMode(IN2_A, OUTPUT);
  pinMode(IN2_B, OUTPUT);

   printf("Stop motors\n");
  analogWrite(IN1_A, 0);
  analogWrite(IN1_B, 0);
  analogWrite(IN2_A, 0);
  analogWrite(IN2_B, 0);
  printf("finish setup\n");


}
void loop() {
  printf("start motors\n");
  motor.SetSmoothSpeed((float)1,(float)1, DBH12Bridge_Direction::CW);
  printf("wait 3s\n");
  delay(3000);
  printf("decelerate motors\n");
  motor.SetSmoothSpeed((float)0,(float)0.25,DBH12Bridge_Direction::CW);
  printf("wait 3s\n");
  delay(3000);
  printf("start motors\n");
  motor.SetSmoothSpeed((float)1,(float)1, DBH12Bridge_Direction::CCW);
  printf("wait 3s\n");
  delay(3000);
  motor.smoothBrake(1);
   printf("wait 3s\n");
  delay(3000);
}
void loop2() {

   printf("Accelerate both forward\n");
  digitalWrite(IN1_A, LOW);
  digitalWrite(IN1_B, LOW);
  for (int i = 0; i < 200; i++) {
    analogWrite(IN2_A, i);
    analogWrite(IN2_B, i);
    delay(20);
  }

  delay(500);

   printf("Decelerate both forward\n");
  for (int i = 200; i >= 0; i--) {
    analogWrite(IN2_A, i);
    analogWrite(IN2_B, i);
    delay(20);
  }

  delay(500);

   printf("Accelerate both reverse\n");
  digitalWrite(IN2_A, LOW);
  digitalWrite(IN2_B, LOW);
  for (int i = 0; i < 200; i++) {
    analogWrite(IN1_A, i);
    analogWrite(IN1_B, i);
    delay(20);
  }

  delay(500);

   printf("Decelerate both reverse\n");
  for (int i = 200; i >= 0; i--) {
    analogWrite(IN1_A, i);
    analogWrite(IN1_B, i);
    delay(20);
  }

  delay(500);

   printf("Move in opposite directions at fixed speed\n");
  digitalWrite(IN1_A, LOW);
  digitalWrite(IN2_B, LOW);
  analogWrite(IN1_B, fixedSpeed);
  analogWrite(IN2_A, fixedSpeed);

  delay(3000); 

   printf("Stop\n");
  analogWrite(IN1_A, 0);
  analogWrite(IN1_B, 0);
  analogWrite(IN2_A, 0);
  analogWrite(IN2_B, 0);

  delay(2000);
}