#include <Adafruit_GFX.h>     //for LCD display
#include <Adafruit_PCD8544.h> //for LCD display
#include <HX711.h>        //to measure load cells
#include "QuickStats.h"   //to calculate median
QuickStats stats;         //initialize an instance of this class
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <EEPROM.h>

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(OLED_RESET);

#define MAG1 A1
#define MAG2 A0


//USE THIS TO CHANGE MACHINE NUMBER, UNCOMMENT AND CHANGE NUMBER 1-10
#define CHANGEMACHINENUM 9

//UNCOMMENT TO DO A NETWORK TEST WITH NO MACHINE FUNCTIONS
//#define NETWORKTEST

//#define LIFTER1 A15
//#define LIFTER2 A14

//#define MOTOR1 A3
//#define MOTOR2 A2

#define LIFTER1 A2
#define LIFTER2 A3

#define LIFTER3 14
#define LIFTER4 15

#define MOTOR1 A14
#define MOTOR2 A15

#define TRIG1 A4
#define ECHO1 A5
#define TRIG2 A6
#define ECHO2 A7

#define ROWS 10
#define POTS 5

#define BUZZER 7

#define SAMPLESIZE 20
#define CHECKCELLAMOUNT 10

#define ENABLESCREEN 0

//Top 6.7
//Bottom 4.5
#define THRESH 0.5
#define STHRESH 1.5

//Calibration Height Threshholds
#define LTHRESH_cal 5.25 //Change ME!
#define HTHRESH_cal 1.5

#define ACTTIME  150000
#define HOMETIME 1500000 //25 Minutes
#define SKIPMAG  5000    // 5 Seconds
#define RSTPOS   300000   //20 Seconds

#define MOTORDELAY 2000
#define ACTDELAY   1000

#define BOTHMAG (!digitalRead(MAG1) && !digitalRead(MAG2))
#define TWOMAG (!digitalRead(MAG1) || !digitalRead(MAG2))

#define ONEMAG  (!digitalRead(MAG1))

#define KNOWNWEIGHT 400
#define BUFFERWEIGHT 3
#define WATERAMOUNT 150

#define SECONDSUP 20
#define SECONDSDN 20

int sol[POTS] = {8, 9, 10, 11, 12}; //Pins to Turn on selenoid on each load cell

//Load Cells
HX711 L5(53,51);
HX711 L4(49,47);
HX711 L3(45,43);
HX711 L2(41,39);
HX711 L1(37,35);

//first -> back
//int targetWeight[POTS] = {1489, 1375, 1262, 1149, 1036}
//int targetWeight[POTS] = {1036, 1149, 1262, 1375, 1489};
//int targetWeight[POTS] = {1200, 1200, 1200, 1200, 1200};
int targetWeight[POTS] = {601, 803, 1005, 1207, 1409};
float originWeight[POTS], tarWeight[POTS], ratioVals[POTS], weight[POTS]; 

bool flag = 0;
unsigned long time1, time2;

unsigned long watertime;
bool first, reWeigh;

float h_thresh1, l_thresh1, h_thresh2, l_thresh2;

int flagCell = 0;

byte MACHINENUM;


void displaySetup(){
  if(ENABLESCREEN){
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x64)
    display.setTextColor(WHITE);
    display.clearDisplay();
    display.setTextSize(0);
    display.setTextColor(0);
    display.setTextColor(1);
    display.setCursor(0,0);
    //display.flip();
    display.display();
  }
}

void displayUpdate(){
  //display.flip();
  if(ENABLESCREEN){
    display.display();
    display.clearDisplay();
    
    display.setCursor(0,0);
  }
  
}


void distanceSetup(int trigger, int echo){
  pinMode(trigger, OUTPUT);
  pinMode(echo, INPUT);
}

float getDistance(int trigger, int echo){
  Serial.print("Measuring distance - ");
  unsigned long int startTime, endTime;
  float finaltime;
  digitalWrite(trigger, 1);
  delayMicroseconds(10);
  digitalWrite(trigger, 0);

  while(!digitalRead(echo));
  startTime = micros();
  while(digitalRead(echo));
  endTime = micros();

  finaltime = (float)(endTime - startTime)/148;
  Serial.print("Current Distance: ");
  Serial.println(finaltime);
  return finaltime;
  Serial.println("Measured");
}

void moveUp(int secondsup){
  time1 = millis();
  while(millis() - time1 < (secondsup * 1000)){
    digitalWrite(LIFTER1, 1);
    digitalWrite(LIFTER3, 1);
    
    digitalWrite(LIFTER2, 0);
    digitalWrite(LIFTER4, 0);
  }
  digitalWrite(LIFTER1, 0);
  digitalWrite(LIFTER3, 0);
  delay(500);
}

void moveDown(int secondsdown){
  time1 = millis();
    while(millis() - time1 < secondsdown * 1000){
      digitalWrite(LIFTER1, 0);
      digitalWrite(LIFTER3, 0);
        
      digitalWrite(LIFTER2, 1);
      digitalWrite(LIFTER4, 1);
    }
  digitalWrite(LIFTER2, 0);
  digitalWrite(LIFTER4, 0);
  delay(500);
}

void actUp(){
  int samples = 10;
  float distance1[samples], distance2[samples], d1, d2, deltaD;
  bool firstUp = 1;

  while((d1 < h_thresh1 - THRESH || d2 < h_thresh2 - THRESH) || firstUp){
    firstUp = 0;

    Serial.println("Going Up");
    if(ENABLESCREEN){
      display.println(F("Going Up..."));
      displayUpdate();
    }
  
    moveUp(SECONDSUP);
  
    if(ENABLESCREEN){
    //display.println("123456789012345678901");
      display.println(F("Went up."));
      display.println(F("Measuring Distance."));
      displayUpdate();
    }
    
    for(int i = 0; i < samples; i++){distance1[i] = getDistance(TRIG1, ECHO1) - l_thresh1; distance2[i] = getDistance(TRIG2, ECHO2) - l_thresh2; delay(100);}
    d1 = stats.median(distance1, samples);
    d2 = stats.median(distance2, samples);
    deltaD = d1 - d2;
    deltaD = abs(deltaD);
    
    Serial.print("Sensor1 must go above (inches): ");
    Serial.println(h_thresh1);
    
    Serial.print("Sensor 1 Current Distance: ");
    Serial.println(d1);

    Serial.print("Sensor2 must go above (inches): ");
    Serial.println(h_thresh2);
    
    Serial.print("Sensor 2 Current Distance: ");
    Serial.println(d2);

    Serial.print("Difference: ");
    Serial.print(deltaD);
    Serial.print(" Must be within: ");
    Serial.println(STHRESH);


    if(ENABLESCREEN){
    //display.println("123456789012345678901");
      display.println(F("Ultrasonics Measuring"));
      display.print(F("Dist 1: "));
      display.print(d1);
      display.println(F("in"));

      display.print(F("Dist 2: "));
      display.print(d2);
      display.println(F("in"));

      display.print(F("Diff: "));
      display.print(deltaD);
      display.println(F("in"));
      
      displayUpdate();
    }
    
    while(deltaD > STHRESH){
      Serial.println(F("Going Up again"));
      if(ENABLESCREEN){
        display.println(F("Going Up again..."));
        display.println(F("Threshold not met"));
        displayUpdate();
      }
      
      moveUp(5);

      if(ENABLESCREEN){
        display.println(F("Went up."));
        displayUpdate();
      }
      
      for(int i = 0; i < samples; i++){distance1[i] = getDistance(TRIG1, ECHO1)  - l_thresh1; distance2[i] = getDistance(TRIG2, ECHO2) - l_thresh2; delay(100);}
    
      d1 = stats.median(distance1, samples);
      d2 = stats.median(distance2, samples);
      deltaD = d1 - d2;
      deltaD = abs(deltaD);

      if(ENABLESCREEN){
      //display.println("123456789012345678901");
        display.println(F("Ultrasonics Measuring"));
        display.print(F("Dist 1: "));
        display.print(d1);
        display.println(F("in"));
  
        display.print(F("Dist 2: "));
        display.print(d2);
        display.println(F("in"));
  
        display.print(F("Diff: "));
        display.print(deltaD);
        display.println(F("in"));
        
        displayUpdate();
      }

      delay(500);
    }
  }
  digitalWrite(LIFTER1, 0);
  digitalWrite(LIFTER3, 0);
  
  digitalWrite(LIFTER2, 0);
  digitalWrite(LIFTER4, 0);
}

void actDn(){
  int samples = 10;
  float distance1[samples], distance2[samples], d1, d2, deltaD;
  int pasttime;
  bool firstDn = 1;
  
  Serial.println("Going Down");
  
  while((d1 > THRESH || d2 > THRESH) || firstDn){
    firstDn = 0;

    if(ENABLESCREEN){
        display.println("Going down...");
        displayUpdate();
    }
    
    moveDown(20);

    if(ENABLESCREEN){
      display.println(F("Went down."));
      display.println(F("Measuring Distance."));
      displayUpdate();      
    }
    
    for(int i = 0; i < samples; i++){distance1[i] = getDistance(TRIG1, ECHO1) - l_thresh1; distance2[i] = getDistance(TRIG2, ECHO2) - l_thresh2; delay(100);}
    
    d1 = stats.median(distance1, samples);
    d2 = stats.median(distance2, samples);
    deltaD = d1 - d2;
    deltaD = abs(deltaD);
    
    Serial.print("Sensor1 must go under (inches): ");
    Serial.println(0.5);
    
    Serial.print("Sensor 1 Current Distance: ");
    Serial.println(d1);

    Serial.print("\nSensor2 must go under (inches): ");
    Serial.println(0.5);
    
    Serial.print("Sensor 2 Current Distance: ");
    Serial.println(d2);

    Serial.print("\nDifference: ");
    Serial.print(deltaD);
    Serial.print(" Must be within: ");
    Serial.println(STHRESH);
    Serial.println("---------------------------");


    if(ENABLESCREEN){
    //display.println("123456789012345678901");
      display.println(F("Ultrasonics Measuring"));
      display.print(F("Dist 1: "));
      display.print(d1);
      display.println(F("in"));

      display.print(F("Dist 2: "));
      display.print(d2);
      display.println(F("in"));

      display.print(F("Diff: "));
      display.print(deltaD);
      display.println(F("in"));
      
      displayUpdate();
    }


    while(deltaD > STHRESH){
      Serial.println(F("Going Dn again"));
      if(ENABLESCREEN){
        display.println(F("Going Dn again..."));
        display.println(F("Threshold not met"));
        displayUpdate();
      }
      moveDown(5);

      if(ENABLESCREEN){
        display.println(F("Went down."));
        display.println(F("Measuring Distance."));
        displayUpdate();      
      }
      
      for(int i = 0; i < samples; i++){distance1[i] = getDistance(TRIG1, ECHO1)  - l_thresh1; distance2[i] = getDistance(TRIG2, ECHO2) - l_thresh2; delay(100);}
    
      d1 = stats.median(distance1, samples);
      d2 = stats.median(distance2, samples);
      deltaD = d1 - d2;
      deltaD = abs(deltaD);

      if(ENABLESCREEN){
      //display.println("123456789012345678901");
        display.println(F("Ultrasonics Measuring"));
        display.print(F("Dist 1: "));
        display.print(d1);
        display.println(F("in"));
  
        display.print(F("Dist 2: "));
        display.print(d2);
        display.println(F("in"));
  
        display.print(F("Diff: "));
        display.print(deltaD);
        display.println(F("in"));
        
        displayUpdate();
      }

      delay(500);
    }
  }
  digitalWrite(LIFTER1, 0);
  digitalWrite(LIFTER3, 0);
  digitalWrite(LIFTER2, 0);
  digitalWrite(LIFTER4, 0);
}

void goHome(){
  if(ENABLESCREEN){
  //display.println("123456789012345678901");
    display.println(F("Going home"));
    displayUpdate();
  }
  Serial.println("Going Home");
  time1 = millis();
  if(BOTHMAG)return;
  goRight();
  while(millis() - time1 < HOMETIME){
    if(BOTHMAG){goNeut(); return;}
  }
  Serial.println("Missed the homing magnets");
  if(ENABLESCREEN){
  //display.println("123456789012345678901");
    display.println(F("Missed Magnets"));
    display.println(F("May want to check"));
    display.println(F("machine for s."));
    displayUpdate();
  }
  time1 = millis();
  goLeft();
  while(millis() - time1 < 60000);
  goHome();
}

void awayHome(){
  Serial.println("Moving From Home");
  goLeft();
  while(millis() - time1 < RSTPOS){
    if(!(!digitalRead(MAG1) || !digitalRead(MAG2)))break;
  }
  delay(1000);
  Serial.println("Past Both Magnets");
  while(millis() - time1 < RSTPOS){
    if(!digitalRead(MAG2))break;
  }
  delay(1000);
  Serial.println("Past Active Range");
  while(millis() - time1 < RSTPOS){
    if(digitalRead(MAG2))break;
  }
  delay(1000);
  goNeut();
}


bool goNext(){
  if(ENABLESCREEN){
  //display.println("123456789012345678901");
    display.println(F("Going forward"));
    displayUpdate();
  }

  
  bool x = goForward();
  time1 = millis();
  if(!x){
    goRight();
    while(TWOMAG){if(millis() - time1 >= RSTPOS){goNeut(); return 0;}}
    
    time2 = millis();
    while(millis() - time2 < 20000);
  }
  goNeut();
  return x;
}

bool goForward(){
  time1 = millis();
  goLeft();
  while(ONEMAG){
    if(BOTHMAG)return 0;
    if(millis() - time1 >= RSTPOS)return 0;
  }

  time2 = millis();
  while(millis() - time2 < 2000){if(BOTHMAG)return 0;}

  while(!ONEMAG){
    if(BOTHMAG)return 0;
    if(millis() - time1 >= RSTPOS)return 0;
  }

  time2 = millis();
  while(millis() - time2 < 2000){if(BOTHMAG)return 0;}
  
  return 1;
}

/*
bool goNext(){
  Serial.println("Going Next");
  if(BOTHMAG)return 0;

  int magS, magF;
  flag = 0;
  time1 = millis();
  
  goLeft();
  while(ONEMAG && millis() - time1 < RSTPOS){
    if(BOTHMAG){return 0;}
  }

  delay(2000);

  while(millis() - time1 < RSTPOS){
    if(BOTHMAG){return 0;}
    else if(ONEMAG)break;
  }

  Serial.println("Sensed magnet");
  
  magS = millis();
  
  while(millis() - time1 < RSTPOS){
    if(BOTHMAG){return 0;}
    else if(!ONEMAG)break;
  }
  
  magF = millis();
 
  while(millis() - time1 < RSTPOS){
    while(millis() - magF < (magF - magS)/2 && millis() - time1 < RSTPOS){
      if(BOTHMAG){return 0;}
      goRight();
    }

    if(!ONEMAG)break;
  } 
  
  time2 = millis();
  while(millis() - time1 < RSTPOS){
    while(millis() - time2 < (magF - magS)/2)goLeft();
    if(BOTHMAG){return 0;}
    if(!ONEMAG)break;
  }
  
  goNeut();

  if(millis() - time1 < RSTPOS)return 1;
  else return 0;
}
*/

void goRight(){
  digitalWrite(MOTOR1, LOW);   // turn the LED on (HIGH is the voltage level)
  digitalWrite(MOTOR2, LOW);
  digitalWrite(MOTOR2, HIGH);    // turn the LED off by making the voltage LOW 
}

void goLeft(){
  digitalWrite(MOTOR1, LOW);
  digitalWrite(MOTOR1, HIGH);   // turn the LED on (HIGH is the voltage level)
  digitalWrite(MOTOR2, LOW);    // turn the LED off by making the voltage LOW 
}

void goNeut(){
  digitalWrite(MOTOR1, LOW);   // turn the LED on (HIGH is the voltage level)
  digitalWrite(MOTOR2, LOW);    // turn the LED off by making the voltage LOW 
}

void nullWeights(){
  for(int i = 0; i < POTS; i++)weight[i] = 0;
}

void powerUpCells(){
  L1.power_up();
  L2.power_up();
  L3.power_up();
  L4.power_up();
  L5.power_up();
}

void calibrateCells(){
  Serial.println("Calibrating Cells");

  if(ENABLESCREEN){
  //display.println(F("123456789012345678901"));
    display.println(F("Calibrating load cell"));
    displayUpdate();
  }
  
  float m1[SAMPLESIZE];
  float m2[SAMPLESIZE];
  float m3[SAMPLESIZE];
  float m4[SAMPLESIZE];
  float m5[SAMPLESIZE];
  //Collect the data points for loads:
  for(int i = 0; i < SAMPLESIZE; i++){
      //Get reading from the specified load cell in the function
      m1[i] = L1.get_units(1);
      m2[i] = L2.get_units(1);
      m3[i] = L3.get_units(1);
      m4[i] = L4.get_units(1);
      m5[i] = L5.get_units(1);
  }  
  originWeight[0] = stats.median(m1, SAMPLESIZE); // calculates median value of weeight
  originWeight[1] = stats.median(m2, SAMPLESIZE); // calculates median value of weeight
  originWeight[2] = stats.median(m3, SAMPLESIZE); // calculates median value of weeight
  originWeight[3] = stats.median(m4, SAMPLESIZE); // calculates median value of weeight
  originWeight[4] = stats.median(m5, SAMPLESIZE); // calculates median value of weeight

  for(int i = 0; i < 5; i++)ratioVals[i] = (originWeight[i] - tarWeight[i])/KNOWNWEIGHT;
  
  Serial.println("Origin of Pots");
  for(int i = 0; i < POTS; i++){Serial.print("Origin "); Serial.print(i+1); Serial.print(": "); Serial.println(originWeight[i]);}
}

void tareCells(){
  Serial.println("Taring Cells");

  if(ENABLESCREEN){
  //display.println(F("123456789012345678901"));
    display.println(F("Taring load cell"));
    displayUpdate();
  }

  
  float m1[SAMPLESIZE];
  float m2[SAMPLESIZE];
  float m3[SAMPLESIZE];
  float m4[SAMPLESIZE];
  float m5[SAMPLESIZE];
  //Collect the data points for loads:
  for(int i = 0; i < SAMPLESIZE; i++){
      //Get reading from the specified load cell in the function
      m1[i] = L1.get_units(1);
      Serial.println("Got 1 Tare");
      m2[i] = L2.get_units(1);
      Serial.println("Got 2 Tare");
      m3[i] = L3.get_units(1);
      Serial.println("Got 3 Tare");
      m4[i] = L4.get_units(1);
      Serial.println("Got 4 Tare");
      m5[i] = L5.get_units(1);
      Serial.println("Got 5 Tare");
  }  
  tarWeight[0] = stats.median(m1, SAMPLESIZE); // calculates median value of weeight
  tarWeight[1] = stats.median(m2, SAMPLESIZE); // calculates median value of weeight
  tarWeight[2] = stats.median(m3, SAMPLESIZE); // calculates median value of weeight
  tarWeight[3] = stats.median(m4, SAMPLESIZE); // calculates median value of weeight
  tarWeight[4] = stats.median(m5, SAMPLESIZE); // calculates median value of weeight
  Serial.println("Tare of Pots");
  for(int i = 0; i < POTS; i++){
    Serial.print("Tare "); Serial.print(i+1); Serial.print(": "); Serial.println(tarWeight[i]);
    if(tarWeight[i] == 0)errorAlert(4, i);
  }
}

int checkCells(){
  Serial.println("Testing Cells");
  flagCell = 0;
  for(int j = 0; j < 5; j++){
    weighCells();
    for(int i = 0; i < 5; i++){
      if(weight[i] < KNOWNWEIGHT - BUFFERWEIGHT || weight[i] > KNOWNWEIGHT + BUFFERWEIGHT){
        flagCell = i+1;
        if(flagCell != 0){
          Serial.println("Cell calibration was not a success");
          Serial.print("Maybe check cell #");
          Serial.println(flagCell);
          return flagCell;
        }
      }
    }
  }

  Serial.println("Cell calibration was a success");
  delay(2500);
  
  return flagCell;
}

void weighCells(){
  Serial.println("Weighing Cells");
  
  if(ENABLESCREEN){
  //display.println(F("123456789012345678901"));
    display.println(F("Weighing load cells"));
    displayUpdate();
  }
  
  float m1[SAMPLESIZE];
  float m2[SAMPLESIZE];
  float m3[SAMPLESIZE];
  float m4[SAMPLESIZE];
  float m5[SAMPLESIZE];
  //Collect the data points for loads:
  for(int i = 0; i < SAMPLESIZE; i++){
      //Get reading from the specified load cell in the function
      m1[i] = L1.get_units(1);
      m2[i] = L2.get_units(1);
      m3[i] = L3.get_units(1);
      m4[i] = L4.get_units(1);
      m5[i] = L5.get_units(1);
  }  

  weight[0] = (stats.median(m1, SAMPLESIZE) - tarWeight[0]) / ratioVals[0]; // calculates median value of weeight
  weight[1] = (stats.median(m2, SAMPLESIZE) - tarWeight[1]) / ratioVals[1]; // calculates median value of weeight
  weight[2] = (stats.median(m3, SAMPLESIZE) - tarWeight[2]) / ratioVals[2]; // calculates median value of weeight
  weight[3] = (stats.median(m4, SAMPLESIZE) - tarWeight[3]) / ratioVals[3]; // calculates median value of weeight
  weight[4] = (stats.median(m5, SAMPLESIZE) - tarWeight[4]) / ratioVals[4]; // calculates median value of weeight
  Serial.println("Weight of Pots");
  for(int i = 0; i < POTS; i++){Serial.print("Pot "); Serial.print(i+1); Serial.print(": "); Serial.println(weight[i]);}
  if(ENABLESCREEN){
  //display.println(F("123456789012345678901"));
    display.println(F("Weight of pots (g):"));
    display.print(F("1: "));
    display.print(weight[0]);
    display.print(F(" 2: "));
    display.println(weight[1]);
    display.print(F("3: "));
    display.print(weight[2]);
    display.print(F(" 4: "));
    display.println(weight[3]);
    display.print(F("5:"));
    display.print(weight[4]);
    
    displayUpdate();
  }

  delay(2500);
}

void weighSingleCell(int x){
  Serial.print("Weiging Cell #"); Serial.println(x+1);
  float m1[SAMPLESIZE];
  float m2[SAMPLESIZE];
  float m3[SAMPLESIZE];
  float m4[SAMPLESIZE];
  float m5[SAMPLESIZE];
  //Collect the data points for loads:
  for(int i = 0; i < SAMPLESIZE; i++){
      //Get reading from the specified load cell in the function
      if(x == 0)       m1[i] = L1.get_units(1);
      else if (x == 1) m2[i] = L2.get_units(1);
      else if (x == 2) m3[i] = L3.get_units(1);
      else if (x == 3) m4[i] = L4.get_units(1);
      else if (x == 4) m5[i] = L5.get_units(1);
  }  

  if(x == 0)       weight[0] = (stats.median(m1, SAMPLESIZE) - tarWeight[0]) / ratioVals[0]; // calculates median value of weeight
  else if (x == 1) weight[1] = (stats.median(m2, SAMPLESIZE) - tarWeight[1]) / ratioVals[1]; // calculates median value of weeight
  else if (x == 2) weight[2] = (stats.median(m3, SAMPLESIZE) - tarWeight[2]) / ratioVals[2]; // calculates median value of weeight
  else if (x == 3) weight[3] = (stats.median(m4, SAMPLESIZE) - tarWeight[3]) / ratioVals[3]; // calculates median value of weeight
  else if (x == 4) weight[4] = (stats.median(m5, SAMPLESIZE) - tarWeight[4]) / ratioVals[4]; // calculates median value of weeight
}


//ONE AT A TIME
void waterPotOAT(){  

  float lastWeight = 0;
  float firstWeight = 1;
  int iChanged = -1;

  for (int i = 0; i < POTS; i++){
    if(iChanged != i){
      iChanged = i;
      firstWeight = weight[i];
      Serial.print("Start Weight #"); Serial.print(i+1); Serial.print(": ");
      Serial.println(firstWeight);
    }
    lastWeight = weight[i];
    
    Serial.print("Current Weight: ");
    Serial.print(weight[i]);
    Serial.print(" Target Weight: ");
    Serial.println(targetWeight[i]);

    watertime = millis();
    
    while(weight[i] < targetWeight[i]){
      Serial.print("Watering... ");
      
      if(weight[i] < targetWeight[i] - 20){
        digitalWrite(sol[i], 1);
        delay(3000); 
        digitalWrite(sol[i], 0);
        delay(500);
        Serial.println("Adding extra water due to extra low weight");
      }
  
      else if(weight[i] < targetWeight[i] - 5){
        digitalWrite(sol[i], 1);
        delay(1000); 
        digitalWrite(sol[i], 0);
        delay(500);
        Serial.println("Adding extra water due to low weight");
      }
      
      else if(weight[i] < targetWeight[i]){
        digitalWrite(sol[i], 1);
        delay(WATERAMOUNT); 
        digitalWrite(sol[i], 0);
        delay(500);
        Serial.println("Adding drop");
      }
      else Serial.println("No water needed");
  
      weighSingleCell(i);
      Serial.print("New Weight: ");
      Serial.println(weight[i]);

      if(weight[i] < lastWeight - 250)errorAlert(2, i);
  
      for(int i = 0; i < POTS; i++)digitalWrite(sol[i], 0); 
      
      if(millis() - watertime >= 300000 && firstWeight >= weight[i] + 3)errorAlert(1, i); 
      if(millis() - watertime >= 1200000)errorAlert(1, i); 
    }

    

    Serial.println("Watering finished");
  }

}

void errorAlert(int x, int potnum){
  for(int i = 0; i < POTS; i++)digitalWrite(sol[i], 0); \

  Serial.print("Pot: ");
  Serial.print(potnum);
  Serial.print(" - ");
  
  if(x == 1)Serial.println("ERROR WATERING FAILED... GOING TO LONG");
  else if(x == 2)Serial.println("ERROR WATERING FAILED... POT GOT LIGHTER. CHECK LOAD CELLS OR DO NOT TOUCH POT");
  else if(x == 3)Serial.println("ERROR POT CALIBRATION FAILED. CHECK ORIGIN CELL AND LOAD CELLS");
  


  /*
  Serial2.print("Machine=");
  Serial2.print(machineNumber);

  Serial2.print("&Pot=");
  Serial2.print(potNum);
  Serial2.print("&Type=Weight&value=");
  Serial2.print(weightPot);
  Serial2.println("");
  */

  //Beeps Pot Number then Error Code.
  while(1){
    for(int i = 0; i < potnum+1; i++){
      digitalWrite(BUZZER, 1);
      delay(500);
      digitalWrite(BUZZER, 0);
      delay(500);
    }
    delay(2500);
    for(int i = 0; i < x; i++){
      digitalWrite(BUZZER, 1);
      delay(250);
      digitalWrite(BUZZER, 0);
      delay(500);
    }
    delay(60000);
  }
}

void warningAlert(int x, int potnum){
  for(int i = 0; i < POTS; i++)digitalWrite(sol[i], 0); 

  Serial.print("Pot: ");
  Serial.print(potnum);
  Serial.print(" - ");
  
  if(x == 1)Serial.println("ERROR WATERING FAILED... GOING TO LONG");
  else if(x == 2)Serial.println("ERROR WATERING FAILED... POT GOT LIGHTER. CHECK LOAD CELLS OR DO NOT TOUCH POT");
  else if(x == 3)Serial.println("ERROR POT CALIBRATION FAILED. CHECK ORIGIN CELL AND LOAD CELLS");
  

  //Beeps Pot Number then Error Code.
  for(int i = 0; i < potnum+1; i++){
    digitalWrite(BUZZER, 1);
    delay(500);
    digitalWrite(BUZZER, 0);
    delay(500);
  }
  delay(2500);
  for(int i = 0; i < x; i++){
    digitalWrite(BUZZER, 1);
    delay(250);
    digitalWrite(BUZZER, 0);
    delay(500);
  }
  delay(5000);
}


//WATER POT ALL SAME TIME
void waterPots(){
  Serial.print("Current Weight #1: ");
  Serial.print(weight[0]);
  Serial.print(" Target Weight #1: ");
  Serial.println(targetWeight[0]);
  
  Serial.print("Current Weight #2: ");
  Serial.print(weight[1]);
  Serial.print(" Target Weight #2: ");
  Serial.println(targetWeight[1]);
  
  Serial.print("Current Weight #3: ");
  Serial.print(weight[2]);
  Serial.print(" Target Weight #3: ");
  Serial.println(targetWeight[2]);
  
  Serial.print("Current Weight #4: ");
  Serial.print(weight[3]);
  Serial.print(" Target Weight #4: ");
  Serial.println(targetWeight[3]);
  
  Serial.print("Current Weight #5: ");
  Serial.print(weight[4]);
  Serial.print(" Target Weight #5: ");
  Serial.println(targetWeight[4]);

  watertime = millis();
  
  while((weight[0] < targetWeight[0] || weight[1] < targetWeight[1] || weight[2] < targetWeight[2] ||
         weight[3] < targetWeight[3] || weight[4] < targetWeight[4]) && millis() - watertime < 300000){
    
    for(int i = 0; i < POTS; i++){
      Serial.print("Watering Cell #");
      Serial.print(i+1);
      if(weight[i] < targetWeight[i] - 20){
        digitalWrite(sol[i], 1);
        delay(3000); 
        digitalWrite(sol[i], 0);
        delay(500);
        Serial.println(" - Adding extra water due to low weight");
      }

      else if(weight[i] < targetWeight[i] - 3){
        digitalWrite(sol[i], 1);
        delay(1000); 
        digitalWrite(sol[i], 0);
        delay(500);
        Serial.println(" - Adding extra water due to low weight");
      }
      
      else if(weight[i] < targetWeight[i]){
        digitalWrite(sol[i], 1);
        delay(WATERAMOUNT); 
        digitalWrite(sol[i], 0);
        delay(500);
        Serial.println(" - Adding drop");
      }
      else Serial.println(" - No water needed");
    }



    for(int i = 0; i < POTS; i++)digitalWrite(sol[i], 0);

    for(int i = 0; i < POTS; i++){if(weight[i] < targetWeight[i])weighSingleCell(i);}
    Serial.print("Current Weight #1: ");
    Serial.print(weight[0]);
    Serial.print(" Target Weight #1: ");
    Serial.println(targetWeight[0]);
    
    Serial.print("Current Weight #2: ");
    Serial.print(weight[1]);
    Serial.print(" Target Weight #2: ");
    Serial.println(targetWeight[1]);
    
    Serial.print("Current Weight #3: ");
    Serial.print(weight[2]);
    Serial.print(" Target Weight #3: ");
    Serial.println(targetWeight[2]);
    
    Serial.print("Current Weight #4: ");
    Serial.print(weight[3]);
    Serial.print(" Target Weight #4: ");
    Serial.println(targetWeight[3]);
    
    Serial.print("Current Weight #5: ");
    Serial.print(weight[4]);
    Serial.print(" Target Weight #5: ");
    Serial.println(targetWeight[4]);
  }

  if(millis() - watertime >= 300000){
    while(1){
      moveUp(1);
      moveDown(1);
      moveUp(1);
      moveDown(1);
      moveUp(1);
      moveDown(1);
      moveUp(1);
      moveDown(1);
      delay(30000);
    }
  }
}

void calibrateUltraSonicDOWN(){
  int samples = 20;
  float distance1[samples], distance2[samples], d1, d2, deltaD;

  
  digitalWrite(LIFTER1, 0);
  digitalWrite(LIFTER3, 0);
  
  digitalWrite(LIFTER2, 1);
  digitalWrite(LIFTER4, 1);
  delay(20000);
  digitalWrite(LIFTER2, 0);
  digitalWrite(LIFTER4, 0);

  for(int i = 0; i < samples; i++){distance1[i] = getDistance(TRIG1, ECHO1); distance2[i] = getDistance(TRIG2, ECHO2); delay(100);}
  l_thresh1 = stats.median(distance1, samples);
  l_thresh2 = stats.median(distance2, samples);

  while(l_thresh1 > LTHRESH_cal || l_thresh2 > LTHRESH_cal){
    digitalWrite(LIFTER1, 0);
    digitalWrite(LIFTER3, 0);
    
    digitalWrite(LIFTER2, 1);
    digitalWrite(LIFTER4, 1);
    delay(20000);
    digitalWrite(LIFTER2, 0);
    digitalWrite(LIFTER4, 0);

    for(int i = 0; i < samples; i++){distance1[i] = getDistance(TRIG1, ECHO1); distance2[i] = getDistance(TRIG2, ECHO2); delay(100);}
    l_thresh1 = stats.median(distance1, samples);
    l_thresh2 = stats.median(distance2, samples);
  }

  Serial.println("Lower Thresholds");
  Serial.println(l_thresh1);
  Serial.println(l_thresh2);

}

void calibrateUltraSonicUP(){
  int samples = 20;
  float distance1[samples], distance2[samples], d1, d2, deltaD;

  
  //Go Up
  digitalWrite(LIFTER1, 1);
  digitalWrite(LIFTER3, 1);
  
  digitalWrite(LIFTER2, 0);
  digitalWrite(LIFTER4, 0);
  delay(20000);
  digitalWrite(LIFTER1, 0);
  digitalWrite(LIFTER3, 0);

  for(int i = 0; i < samples; i++){distance1[i] = getDistance(TRIG1, ECHO1); distance2[i] = getDistance(TRIG2, ECHO2); delay(100);}
  h_thresh1 = stats.median(distance1, samples) - l_thresh1;
  h_thresh2 = stats.median(distance2, samples) - l_thresh2;

  while(h_thresh1 < HTHRESH_cal || h_thresh2 < HTHRESH_cal){
    //Go Up
    digitalWrite(LIFTER1, 1);
    digitalWrite(LIFTER3, 1);
    
    digitalWrite(LIFTER2, 0);
    digitalWrite(LIFTER4, 0);
    delay(20000);
    digitalWrite(LIFTER1, 0);
    digitalWrite(LIFTER3, 0);

    for(int i = 0; i < samples; i++){distance1[i] = getDistance(TRIG1, ECHO1); distance2[i] = getDistance(TRIG2, ECHO2); delay(100);}
    h_thresh1 = stats.median(distance1, samples) - l_thresh1;
    h_thresh2 = stats.median(distance2, samples) - l_thresh2;
  }

  Serial.println("Higher Thresholds");
  Serial.println(h_thresh1);
  Serial.println(h_thresh2);
}

void calibrationWeightSet(int rowCount){
  if(rowCount >= 0 && rowCount <= 2){
    for(int i = 0; i < 5; i++)targetWeight[i] = 500;
  }
  
  if(rowCount >= 3 && rowCount <= 5){
    for(int i = 0; i < 5; i++)targetWeight[i] = 1000;
  }
  
  if(rowCount >= 6 && rowCount <= 8){
    for(int i = 0; i < 5; i++)targetWeight[i] = 1500;
  }
}


void webPrint(int machineNumber, char reportSys[]){
  //int x = sizeof(reportSys) - 1;
  Serial2.print("Machine=");
  Serial2.print(machineNumber);
  Serial2.print("&Type=System");
  Serial2.print("&value=");
  for (byte i = 0; i < 10; i++) {
    Serial2.print(reportSys[i]);
  }
  Serial2.println("");
}

void webPrintPot(int machineNumber, int potNum, float weightPot){
  
  Serial2.print("Machine=");
  Serial2.print(machineNumber);

  Serial2.print("&Pot=");
  Serial2.print(potNum);
  Serial2.print("&Type=Weight&value=");
  Serial2.print(weightPot);
  Serial2.println("");
  /*
  for (byte i = 0; i < sizeof(type) - 1; i++) {
    Serial.print(type[i]);
  }
  */


  /*
      Serial2.print("Machine=1&Pot=");
      Serial2.print((i*5)+(j+1));
      Serial2.print("&Type=Weight&value=");
      Serial2.println(weight[j]);
      */
}

void setup() {
  // put your setup code here, to run once:
  delay(1000);

  #ifdef CHANGEMACHINENUM
    EEPROM.update(0, CHANGEMACHINENUM);
  #endif

  MACHINENUM = EEPROM.read(0);
  if(MACHINENUM > 10)MACHINENUM = 0;
    
  Serial.begin(9600);
  Serial2.begin(74880);
  delay(1000);


  webPrint(MACHINENUM, "Started   ");
  Serial.println("Arduino was reset");
  Serial.print("Starting Machine ");
  Serial.println(MACHINENUM);

  if(ENABLESCREEN){
    displaySetup();
    display.println(F("IPS Starting up..."));
    displayUpdate();
  }

  pinMode(LIFTER1, OUTPUT);
  pinMode(LIFTER2, OUTPUT);
  pinMode(LIFTER3, OUTPUT);
  pinMode(LIFTER4, OUTPUT);
  

  pinMode(MOTOR1, OUTPUT);
  pinMode(MOTOR2, OUTPUT);

  pinMode(BUZZER, OUTPUT);

  for(int i = 0; i < POTS; i++)pinMode(sol[i], OUTPUT);

  pinMode(MAG1, INPUT_PULLUP);
  pinMode(MAG2, INPUT_PULLUP);

  distanceSetup(TRIG1, ECHO1);
  distanceSetup(TRIG2, ECHO2);

  Serial.println("Inputs setup");

  #ifdef NETWORKTEST
    while(1){
      webPrint(MACHINENUM, "NetwrkTest");
      
      digitalWrite(BUZZER, 1);
      delay(100);
      digitalWrite(BUZZER, 0);
      
      delay(60000);
    }
  #endif


  for(int i = 0; i < MACHINENUM; i++){
    digitalWrite(BUZZER, 1);
    delay(200);
    digitalWrite(BUZZER, 0);
    delay(300);
  }
      
  

  powerUpCells();

  Serial.println("Load cells powered");
  
  //moveUp(20);
  //moveDown(20);


  Serial.println("Calibrating Ultrasonic Down");

  webPrint(MACHINENUM, "Calib Down");
  calibrateUltraSonicDOWN();

  Serial.println("Calibrated");  
  
  //Serial2.println("Machine=1&Pot=1&Type=System&value=TaringCell");
  
  webPrint(MACHINENUM, "1st Homing");
  goHome();
  delay(MOTORDELAY);
  
  webPrint(MACHINENUM, "TaringCell");
  tareCells();


  webPrint(MACHINENUM, "Calib Up  ");
  calibrateUltraSonicUP();
  //moveUp(20);

  first = 1;
}

void loop() {
  //Run this if its not the first time going through the loop.
  if(!first){
    if(ENABLESCREEN)displayUpdate();
    webPrint(MACHINENUM, "Going Home");
    goHome();
    delay(MOTORDELAY);

    webPrint(MACHINENUM, "TaringCell");
    tareCells();

    webPrint(MACHINENUM, "Going Up  ");
    actUp();
    //moveUp(20);
  }
  if(first)first = 0;
  
  //nullWeights();
  delay(ACTDELAY);
  calibrateCells();
  
  for(int i = 0; i < CHECKCELLAMOUNT; i++){
    reWeigh = 0;
    flagCell = checkCells();
    if(flagCell != 0){
      warningAlert(3, flagCell);
      reWeigh = 1;
    }
    actDn();
    
    actUp();
    flagCell = checkCells();
    actDn();
    if(flagCell == 0 || reWeigh == 0)break;

    warningAlert(3, flagCell);

    tareCells();
    actUp();
    calibrateCells();

    delay(1000);
  }
  if(flagCell > 0)errorAlert(3, flagCell); 

  actDn();
  delay(ACTDELAY);
  
  awayHome();
  for(int i = 0; i < ROWS-1; i++){
  //for(int i = 0; i < 2-1; i++){
    
    webPrint(MACHINENUM, "Going Next");
    if(!goNext()){Serial.println("Go next failed."); webPrint(MACHINENUM, "Nxt Failed"); break;}
    delay(MOTORDELAY);
    actUp();
    //moveUp(20);
    delay(ACTDELAY);
    weighCells();
    
    for(int j = 0; j < POTS; j++){
      /*
      Serial2.print("Machine=1&Pot=");
      Serial2.print((i*5)+(j+1));
      Serial2.print("&Type=Weight&value=");
      Serial2.println(weight[j]);
      */
      
      webPrintPot(MACHINENUM, (i*5)+(j+1), weight[j]);
    }
    
                                            
    //calibrationWeightSet(i);

    //webPrint(xxxxxxxx, "1234567890");
    webPrint(MACHINENUM, "Watering  ");
    //waterPots();

    //UNCOMMENT TO START WATERING
    waterPotOAT();
    
    weighCells();
    for(int j = 0; j < POTS; j++){
      /*
      Serial2.print("Machine=1&Pot=");
      Serial2.print((i*5)+(j+1));
      Serial2.print("&Type=Weight&value=");
      Serial2.println(weight[j]);
      */

      webPrintPot(MACHINENUM, (i*5)+(j+1), weight[j]);
    }
    
    actDn();
    
    //webPrint(xxxxxxxx, "1234567890");
    webPrint(MACHINENUM, "Going Down");
    //moveDown(20);
    delay(ACTDELAY);
  }
}
