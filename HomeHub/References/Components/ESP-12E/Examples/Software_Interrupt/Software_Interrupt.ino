
1
2
3
4
5
6
7
8
9
10
11
12
13
14
15
16
17
18
19
20
21
22
23
24
25
26
27
28
29
30
31
32
33
34
35
36
37
38
39
/*
    Ticker ESP8266
    Hardware: NodeMCU
    Circuits4you.com
    2018
    LED Blinking using Ticker
*/
#include <ESP8266WiFi.h>
#include <Ticker.h>  //Ticker Library
 
Ticker blinker;
 
#define LED 2  //On board LED
 
//=======================================================================
void changeState()
{
  digitalWrite(LED, !(digitalRead(LED)));  //Invert Current State of LED  
}
//=======================================================================
//                               Setup
//=======================================================================
void setup()
{
    Serial.begin(115200);
    Serial.println("");
 
    pinMode(LED,OUTPUT);
 
    //Initialize Ticker every 0.5s
    blinker.attach(0.5, changeState); //Use <strong>attach_ms</strong> if you need time in ms
}
//=======================================================================
//                MAIN LOOP
//=======================================================================
void loop()
{
}
