//#include <Time.h> // library to keep track of the hour of the day
#include <Adafruit_GFX.h>
//#include <Adafruit_PCD8544.h>
#include <Wire.h>
#include "RTClib.h" //for clock
#include <EEPROM.h>
#include <Servo.h>
#include <Adafruit_MAX31865.h>
#include <Encoder.h>

#include <Adafruit_SSD1306.h>
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(OLED_RESET);

#define LIGHTSON  digitalWrite(lightsPin, 1)
#define LIGHTSOFF digitalWrite(lightsPin, 0)

#define COOLER_ON  digitalWrite(coolerPin, 1)//myservo.write(maxAngle)
#define COOLER_OFF digitalWrite(coolerPin, 0)//myservo.write(minAngle)

#define HEATER_ON  digitalWrite(heaterPin, 1)
#define HEATER_OFF digitalWrite(heaterPin, 0)

#define LIGHTSLED(x) digitalWrite(lightsLED, x)
#define COOLERLED(x) digitalWrite(coolerLED, x)
#define HEATERLED(x) digitalWrite(heaterLED, x)

#define COOLING  1
#define HEATING  2
#define INACTIVE 0

#define ACOFFTIME 60000

#define MENUTIMEOUT 15000

#define MACHINENUM 10


//DEFAULT VALUES
#define DAYTEMP     25.0
#define NIGHTTEMP   25.0
#define CORFACTOR   0.0
#define BUFFTEMP    1.0
#define HEATERON    5.0
#define HEATEROFF   10.0
#define OVERRIDE    300.0
#define COOLEROFF   60.0
#define MORNINGTIME 8
#define NIGHTTIME   18
#define MINANGLE    15
#define MAXANGLE    50

Servo myservo;  // create servo object to control a servo
// twelve servo objects can be created on most boards

Encoder encoder(2, 3);

int pos = 0;    // variable to store the servo position

RTC_DS3231 rtc;


//pt100
Adafruit_MAX31865 max = Adafruit_MAX31865(9,11, 12, 13); // Use software SPI: CS, DI, DO, CLK
#define RREF      430.0 // The value of the Rref resistor. Use 430.0 for PT100 and 4300.0 for PT1000
#define RNOMINAL  100.0   // The 'nominal' 0-degrees-C resistance of the sensor 100.0 for PT100, 1000.0 for PT1000
DateTime now;

///PINS

#define lightsPin 8 //digital pin to turn on light (NOTE: This function is not yet IMPlemented
#define heaterPin A3 //digital pin to turn on heater
#define coolerPin A2 //digital pin to turn on cooler

#define lightsLED A0
#define heaterLED 7
#define coolerLED 10


//VARIABLES
//for servo
int minAngle = MINANGLE;
int maxAngle = MAXANGLE;

//For time
byte morningTime;
byte nightTime;

//For temperature
float desiredTemp;
float desireDayTemp;//this is temperature expected during the day light
float desireNightTemp;//this is the temeprature expected during the nigh light
float correctionFactor;//the value by which the given termocouple is offset from actual temperature
float buffTemp; //marging accepted around the desire temeprature. Note that the PT100 has a precision of 0.1oC. becuase of this, it is goo to leave temperature to be within 0.2 oC of the desiere target.
float heaterOn; //time in milisecodns to turn ON Heather
float heaterOff; //time in milisecodns to turn OFF heather and wait for heat to stabailize
float coolerOverride; //time in milisecodns to turn ON Heather
float coolerOff; //time in milisecodns to turn OFF heather and wait for heat to stabailize
float buffMultiplier = 0.9;


//status
int coolingStatus, coolingStatusLast, tempControlOn = 1;
bool dayStatus, firstRun, firstLoop, firstRead;
double currentTemp; float currentTempFloat = 0;

//encoder
long encoderPos;
long encoderOld = -999; 
long encoderLastPos = 0;
bool encoderBut, encoderFalling, encoderLast;

//button
byte buttonHeld;

int machineNum;

//menu
byte enterMenu  = 0;
byte enterMenu2 = 0;
int menuSteps = 0, time1, time2, time3, lastmenu = 0;
unsigned long int loopCount;


//Timings
unsigned long int startTime;


unsigned long int minutetimer, menuTimeout;
bool firstSend;

float overshootDay = 0.5, overshootNight = 0.5;

unsigned long int acTimer;
int acOffTime = 60000;
bool acWasOn, firstCool = 1;


void displaySetup(){
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

void displayUpdate(){
  //display.flip();
  display.display();
  display.clearDisplay();
  
  display.setCursor(0,0);
  
}

double getTemperature(){
  return max.temperature(RNOMINAL, RREF) + correctionFactor;
}

void screenUpdate(char currentTime[], double temp1, double desireTemp){
  display.print(F("M"));
  display.print(machineNum);
  if(machineNum > 9)display.print(" ");
  else display.print("  ");
  if(now.month() < 10)display.print("0");
  display.print(now.month());
  display.print("/");
  if(now.day() < 10)display.print("0");
  display.print(now.day());
  display.print("/");
  display.print(now.year());
  display.print("  ");
  display.println(now.toString(currentTime));

  //display.print(F("Current Temp: "));

  display.setTextSize(0);
  display.print(" ");
  display.setTextSize(2);
  if(temp1 < 0)display.print(temp1, 0);
  else display.print(temp1, 1);
  display.print(" ");
  display.setTextSize(0);
  display.print(" ");
  display.setTextSize(2);
  
  display.println(desireTemp, 1);
  display.setTextSize(0);
  display.print(F("Temp | "));    
  if(!dayStatus)display.print(F("NIGHT"));
  else display.print(F(" DAY"));
  if(coolingStatus == HEATING)display.print("(H)");
  else if(coolingStatus == COOLING)display.print("(C)");
  else display.print("( )");
  display.print(F(" | Set"));
  display.print(F(" - "));
  
  displayUpdate();
}

void getCoolingStatus(){
  coolingStatusLast = coolingStatus;
  

  //If within buffer then turn off.
  if(currentTemp < desiredTemp + buffTemp && currentTemp > desiredTemp - buffTemp){
    //if(coolingStatus != INACTIVE)firstRun = 1;
    coolingStatus = INACTIVE;
  }

  //If temp is out of buffer start cooling
  else if (currentTemp > desiredTemp + buffTemp){
    if(coolingStatus != COOLING)firstRun = 1;
    coolingStatus = COOLING;
  }

  //If temp is within of buffer start heating
  else {
    if(coolingStatus != HEATING)firstRun = 1;
    coolingStatus = HEATING;
  }
}

void refreshEverything(){
  //Get time and temp
  now = rtc.now();
  
  currentTemp = getTemperature();

  if(now.hour() >= morningTime && now.hour() < nightTime)dayStatus = 1;
  else dayStatus = 0;

  desiredTemp = dayStatus ? desireDayTemp : desireNightTemp;
  
  getCoolingStatus();
}

void tempControlHeat(){
  if(coolingStatusLast != coolingStatus){
      COOLERLED(0);
      COOLER_OFF;
      HEATERLED(0); 
      HEATER_OFF;
  }
  
  if(coolingStatus == INACTIVE)return;
  float onTime, offTime;
  
  if(firstRun){
    startTime = millis();
    firstRun = 0;
  }
    
  /*
  if(coolingStatus == COOLING){
    onTime = coolerOverride;
    offTime = coolerOff; 
  }
  */

  if(coolingStatus == HEATING){
    onTime = heaterOn;
    offTime = heaterOff; 
  }

  onTime *=1000;
  offTime*=1000;

  if(tempControlOn){
    //if(coolingStatus == COOLING){COOLERLED(1); COOLER_ON;}
    if(coolingStatus == HEATING){HEATERLED(1); HEATER_ON;}

    if(offTime == 0);
    else if(millis() - startTime > onTime){
      tempControlOn = 0;
      COOLERLED(0);
      COOLER_OFF;
      HEATERLED(0); 
      HEATER_OFF;
      startTime = millis();
    }   
  }

  else {
    if(coolingStatus == COOLING){COOLERLED(0); COOLER_OFF;}
    else if(coolingStatus == HEATING){HEATERLED(0); HEATER_OFF;}

    if(millis() - startTime > offTime){
      tempControlOn = 1;
      COOLERLED(0);
      COOLER_OFF;
      HEATERLED(0); 
      HEATER_OFF;
      startTime = millis();
    }   
  }
}

void tempControlAC(){
  //acOffTime = 60000;
  overshootDay = coolerOverride;
  overshootNight = coolerOverride*0.5;
  //Conditionals to set AC off
  bool dayOff = currentTemp < desiredTemp - buffTemp*buffMultiplier + overshootDay && dayStatus;
  bool nightOff = currentTemp < desiredTemp - buffTemp*buffMultiplier + overshootNight && !dayStatus;

  //If temp is out of buffer start cooling
  if (currentTemp > desiredTemp + buffTemp*buffMultiplier && (millis() - acTimer > ACOFFTIME || firstCool)){
    if(firstCool)firstCool = 0;
    coolingStatus = COOLING;
    acWasOn = 1;
  }

  
  if (dayOff || nightOff){
    COOLER_OFF;
    COOLERLED(0);
    if(acWasOn){
      acWasOn = 0;
      acTimer = millis();
    }
  }

  if(!acWasOn){
    COOLER_OFF;
    COOLERLED(0);
    //display.print((millis() - acTimer)/1000);
    //display.println(" OFF");
  }

  if(acWasOn){
    COOLER_ON;
    COOLERLED(1);
    //display.print((millis() - acTimer)/1000);
    //display.println(" ON");
  }

  /*
  if(millis() - acTimer > ACOFFTIME){
    display.print("TIMER TRUE");
  }
  else {
    display.print("F");
    display.print(millis() - acTimer);
    display.print(" ");
    display.print(ACOFFTIME);
  }
  */
}

void lightControl(){
  dayStatus ? LIGHTSON : LIGHTSOFF;
  LIGHTSLED(dayStatus);
}

/*
__Temp Sensor__
Day Temp - 10
Night Temp - 20
Correction Factor - 30
Buffer Temp - 40
__Heating__
Heating On - 50
Heating Off - 60
__Cooling__
Cooling On - 70
Cooling Off - 80
__Time__
Morning Time - 90
Day Time - 100
RTC Month, Day, Year
RTC Hours, Min, Second
 */

void screenMenu(char opt1[], char opt2[], char opt3[]){
  int menuPos = mod(encoderPos, 4);
  if(menuPos == 0)display.print(">");
  for(int i = 0; i < 21 - 1; i++){
    display.print(opt1[i]);
  }
  if(menuPos == 1)display.print(">");
  for(int i = 0; i < 21 - 1; i++){
    display.print(opt2[i]);
  }
  if(menuPos == 2)display.print(">");
  for(int i = 0; i < 21 - 1; i++){
    display.print(opt3[i]);
  }
  if(menuPos == 3)display.print(">");
    //display.println(encoder.read()/4);
  display.println(F("Back"));

  /*
  if(menuPos == 4){
    display.clearDisplay();  
    display.setCursor(0,0);
    for(int i = 0; i < 21 - 1; i++){
      display.print(opt2[i]);
    }
    for(int i = 0; i < 21 - 1; i++){
      display.print(opt3[i]);
    }
    for(int i = 0; i < 21 - 1; i++){
      display.print(opt4[i]);
    }
    display.println(F(">Back"));
  }
  */

  displayUpdate();

  if(encoderFalling){
    enterMenu = 2 + menuPos;
    if(menuPos == 3)enterMenu = 0;
    firstLoop = 1;
  }
}

void screenMenu1(char opt1[], char opt2[], char opt3[], char opt4[]){
  int menuPos = mod(encoderPos, 5);
  if(menuPos == 0)display.print(F(">"));
  for(int i = 0; i < 21 - 1; i++){
    display.print(opt1[i]);
  }
  if(menuPos == 1)display.print(F(">"));
  for(int i = 0; i < 21 - 1; i++){
    display.print(opt2[i]);
  }
  if(menuPos == 2)display.print(F(">"));
  for(int i = 0; i < 21 - 1; i++){
    display.print(opt3[i]);
  }
  if(menuPos == 3)display.print(F(">"));
  //display.println(encoder.read()/4);
  for(int i = 0; i < 21 - 1; i++){
    display.print(opt4[i]);
  }

  if(menuPos == 4){
    display.clearDisplay();  
    display.setCursor(0,0);
    for(int i = 0; i < 21 - 1; i++){
      display.print(opt2[i]);
    }
    for(int i = 0; i < 21 - 1; i++){
      display.print(opt3[i]);
    }
    for(int i = 0; i < 21 - 1; i++){
      display.print(opt4[i]);
    }
    display.println(F(">Back"));
  }

  displayUpdate();

  if(encoderFalling){
    enterMenu2 = menuPos + 1;
    if(menuPos == 4)enterMenu = 1;
    firstLoop = 1;
    firstRead = 1;
  }
}

/*
void adjustValue(byte location, char name1[]){
  float newVal; 
  
  //if(firstRead){EEPROM.get(location, currentTempFloat); firstRead = 0;}
  currentTempFloat = 0;

  newVal = currentTempFloat + encoderPos*0.1;
  
  
  for(int i = 0; i < 21 - 1; i++){
    display.print(name1[i]);
  }
  
  display.setTextSize(2);
  display.print(newVal);
  display.setTextSize(0);

  displayUpdate();

  if(encoderFalling){
    //if(newVal != currentTempFloat)EEPROM.put(location, newVal);
    firstLoop = 1;
    enterMenu2 = 0;
  }
}x 
*/

void adjustValue(byte location){
  if(firstRead){EEPROM.get(location, currentTempFloat); firstRead = 0;}

  float newVal = currentTempFloat + encoderPos*0.1;
  
  display.setTextSize(2);
  display.println(newVal);
  display.setTextSize(0);

  displayUpdate();

  if(encoderFalling){
    if(newVal != currentTempFloat)EEPROM.put(location, newVal);
    firstLoop = 1;
    enterMenu2 = 0;
    
    if(location == 10)     EEPROM.get(10, desireDayTemp);
    else if(location == 20)EEPROM.get(20, desireNightTemp);
    else if(location == 30)EEPROM.get(30, correctionFactor);
    else if(location == 40)EEPROM.get(40, buffTemp);
    else if(location == 50)EEPROM.get(50, heaterOn);
    else if(location == 60)EEPROM.get(60, heaterOff);
    else if(location == 70)EEPROM.get(70, coolerOverride);
    else if(location == 80)EEPROM.get(80, coolerOff);
  }
}


void adjustValueByte(byte location){
  byte currentval = EEPROM.read(location);

  byte newVal = currentval + encoderPos;
  
  display.setTextSize(2);
  display.print(newVal);
  display.setTextSize(0);

  displayUpdate();

  if(encoderFalling){
    if(newVal != currentval)EEPROM.write(location, newVal);
    firstLoop = 1;
    enterMenu2 = 0;
    
    if(location == 90)     morningTime = EEPROM.read(90);
    else if(location == 100)nightTime =  EEPROM.read(100);
    else if(location == 110)minAngle =   EEPROM.read(110);
    else if(location == 120)maxAngle =   EEPROM.read(120);
  }
}

/*
void adjustServoLive(byte location){
  byte currentval = EEPROM.read(location);

  byte newVal = currentval + encoderPos;
  
  display.setTextSize(2);
  display.print(newVal);
  display.setTextSize(0);

  displayUpdate();

  myservo.write(newVal);

  if(encoderFalling){
    if(newVal != currentval)EEPROM.write(location, newVal);
    firstLoop = 1;
    enterMenu2 = 0;
    
    if(location == 110)minAngle = EEPROM.read(110);
    else if(location == 120)maxAngle = EEPROM.read(120);
  }
}
*/

void adjustRTCTime(){
  int newTime;
  int setTime1, setTime2, setTime3;
  
  if(firstRead){
    setTime1 = time1 = now.hour();
    setTime2 = time2 = now.minute();
    setTime3 = time3 = now.second();
    menuSteps = 1;
    loopCount = 0;
    firstRead = 0;
  }
  
  //loopCount++;

  display.setTextSize(2);
  
  if(menuSteps == 1){
    newTime = time1 + encoderPos;
    setTime1 = newTime;
    display.print(">");
  }
  if(setTime1 < 10)display.print("0");
  display.print(setTime1);
  display.print(":");

  if(menuSteps == 2){
    newTime = time2 + encoderPos;
    setTime2 = newTime;
    display.print(">");
  }
  if(setTime2 < 10)display.print("0");
  display.print(setTime2);
  display.print(":");

  if(menuSteps == 3){
    newTime = time3 + encoderPos;
    setTime3 = newTime;
    display.print(">");
  }
  if(setTime3 < 10)display.print("0");
  
  display.print(setTime3);
  display.setTextSize(0);

  if(menuSteps == 4){
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Reset device to");
    display.println("display changes.");
  }

  displayUpdate();

  if(encoderFalling){
    firstLoop = 1;
    menuSteps++;
    rtc.adjust(DateTime(now.year(), now.month(), now.day(), setTime1, setTime2, setTime3));  
    
    if(menuSteps == 5){
      enterMenu2 = 0;
      menuSteps = 0;
      
    }
    
  }
}

void adjustRTCDate(){
  int newTime;
  int setTime1, setTime2, setTime3;
  
  if(firstRead){
    setTime1 = time1 = now.year();
    setTime2 = time2 = now.month();
    setTime3 = time3 = now.day();
    menuSteps = 1;
    loopCount = 0;
    firstRead = 0;
  }
  
  //loopCount++;

  display.setTextSize(1);
  
  

  if(menuSteps == 1){
    newTime = time2 + encoderPos;
    setTime2 = newTime;
    display.print(">");
  }
  if(setTime2 < 10)display.print("0");
  display.print(setTime2);
  display.print("/");

  if(menuSteps == 2){
    newTime = time3 + encoderPos;
    setTime3 = newTime;
    display.print(">");
  }
  if(setTime3 < 10)display.print("0");
  display.print(setTime3);
  display.print("/");

  if(menuSteps == 3){
    newTime = time1 + encoderPos;
    setTime1 = newTime;
    display.print(">");
  }
  display.print(setTime1);

  
  display.setTextSize(0);

  if(menuSteps == 4){
    display.clearDisplay();
    display.setCursor(0,0);
    display.println("Reset device to");
    display.println("display changes.");
  }

  displayUpdate();

  if(encoderFalling){
    firstLoop = 1;
    menuSteps++;
    rtc.adjust(DateTime(setTime1, setTime2, setTime3, now.hour(), now.minute(), now.second()));  
    
    if(menuSteps == 5){
      enterMenu2 = 0;
      menuSteps = 0;
      
    }
    
  }
}

/*
void statsMenu(){
  display.print("Maximum Temp: ");
  display.println(maxTemp);
  display.print("Minimum Temp: ");
  display.println(minTemp);
  display.println(:
}
*/

void valImport(){
  encoderOld = encoder.read()/4;
  encoderPos = encoder.read()/4 - encoderOld;
  
  bool flag = 0;
  bool resetMem = 0;
  EEPROM.get(10, desireDayTemp);
  EEPROM.get(20, desireNightTemp);
  EEPROM.get(30, correctionFactor);
  EEPROM.get(40, buffTemp);
  EEPROM.get(50, heaterOn);
  EEPROM.get(60, heaterOff);
  EEPROM.get(70, coolerOverride);
  EEPROM.get(80, coolerOff);
  
  morningTime = EEPROM.read(90);
  nightTime = EEPROM.read(100);
  minAngle = EEPROM.read(110);
  maxAngle = EEPROM.read(120);

  if(desireDayTemp    < 10 || desireDayTemp    > 100)flag = 1;
  if(desireNightTemp  < 10 || desireNightTemp  > 100)flag = 1;
  if(correctionFactor < -20 || correctionFactor > 20)flag = 1;
  if(buffTemp  < -15 || buffTemp  > 15)flag = 1;
  if(heaterOn  < 0 || heaterOn  > 200)flag = 1;
  if(heaterOff < 0 || heaterOff > 200)flag = 1;
  if(coolerOverride  < 0 || coolerOverride  > 5)flag = 1;
  if(coolerOff < 0 || coolerOff > 10000)flag = 1;
  if(morningTime < 0 || morningTime > 24)flag = 1;
  if(nightTime   < 0 || nightTime   > 24)flag = 1;
  if(minAngle    < 0 || minAngle    > 180)flag = 1;
  if(maxAngle    < 0 || maxAngle    > 180)flag = 1;

  while(flag == 1){
    encoderLast = encoderBut; 
    encoderBut = !digitalRead(4);
    encoderFalling = (!encoderLast && encoderBut);
    
    encoderPos = encoder.read()/4 - encoderOld;
    display.println("Invalid val detected");
    display.println("Reset Saved Values?");
    if(mod(encoderPos, 2) == 1){
      display.println(F(">Yes    No"));
      displayUpdate();
      if(encoderFalling == 1){
        resetMem = 1;
        break;
      }
    }
    else{
      display.println(F(" Yes   >No"));
      displayUpdate();
      if(encoderFalling == 1){
        resetMem = 0;
        break;
      }
    }
  }

  if(resetMem == 1){
    display.println(F("Resetting EEPROM..."));
    displayUpdate();
    EEPROM.put(10, (float)DAYTEMP);
    EEPROM.put(20, (float)NIGHTTEMP);
    EEPROM.put(30, (float)CORFACTOR);
    EEPROM.put(40, (float)BUFFTEMP);
    EEPROM.put(50, (float)HEATERON);
    EEPROM.put(60, (float)HEATEROFF);
    EEPROM.put(70, (float)OVERRIDE);
    EEPROM.put(80, (float)COOLEROFF);
    EEPROM.put(90, (float)MORNINGTIME);
    EEPROM.put(100, (float)NIGHTTIME);
    EEPROM.write(90,  MORNINGTIME);
    EEPROM.write(100, NIGHTTIME);
    EEPROM.write(110, MINANGLE);
    EEPROM.write(120, MAXANGLE);
    display.println(F("Sucessfully reset"));
    displayUpdate();
    delay(500);

    EEPROM.get(10, desireDayTemp);
    EEPROM.get(20, desireNightTemp);
    EEPROM.get(30, correctionFactor);
    EEPROM.get(40, buffTemp);
    EEPROM.get(50, heaterOn);
    EEPROM.get(60, heaterOff);
    EEPROM.get(70, coolerOverride);
    EEPROM.get(80, coolerOff);
    morningTime = EEPROM.read(90);
    nightTime = EEPROM.read(100);
    minAngle = EEPROM.read(110);
    maxAngle = EEPROM.read(120);
  }
  
}

int mod(long x, int y){
  return x<0 ? ((x+1)%y)+y-1 : x%y;
}

void menus(){
  if(enterMenu == 1){
    screenMenu ("Temp Adjust        \n", "Heat/Cool Adjust   \n", "Time Adjust        \n");
  }

  else if(enterMenu == 2){
    if(enterMenu2 == 1){
      display.println(F("Day Temperature:   \n"));
      adjustValue(10);
    }
    else if(enterMenu2 == 2){
      display.println(F("Night Temperature: \n"));
      adjustValue(20);
    }
    else if(enterMenu2 == 3){
      display.println(F("Correction Factor: \n"));
      adjustValue(30);
    }
    else if(enterMenu2 == 4){
      display.println(F("Buffer Temperature:\n"));
      adjustValue(40);
    }
    else screenMenu1("Day Temperature    \n", "Night Temperature  \n", "Correction Factor  \n", "Buffer Temperature \n");
  }

  else if(enterMenu == 3){
    if(enterMenu2 == 1){
      display.println(F("Heater On sec(s):  \n"));
      adjustValue(50);
    }
    else if(enterMenu2 == 2){
      display.println(F("Heater Off sec(s): \n"));
      adjustValue(60);
    }
    else if(enterMenu2 == 3){
      display.println(F("Overshoot Adj (c): \n"));
      adjustValue(70);
    }
    else if(enterMenu2 == 4){
      display.println(F("Cooler Off sec(s): \n"));
      adjustValue(80);
    }
    else screenMenu1("Heater On Adjust   \n", "Heater Off Adjust  \n", "Overshoot Adjust   \n", "Cooler Off Adjust  \n");
  }

  else if(enterMenu == 4){
    if(enterMenu2 == 1){
      display.println(F("Morning Hour       \n"));
      adjustValueByte(90);
    }
    else if(enterMenu2 == 2){
      display.println(F("Night Hour         \n"));
      adjustValueByte(100);
    }
    else if(enterMenu2 == 3){
      display.println(F("RTC Time Adjust:"));
      adjustRTCTime();
    }
    else if(enterMenu2 == 4){
      display.println(F("RTC Date Adjust: "));
      adjustRTCDate();
    }
    else screenMenu1("Morning Hour       \n", "Night Hour         \n", "RTC Time Adjust    \n", "RTC Date Adjust    \n");
  }

/*
  else if(enterMenu == 5){
    if(enterMenu2 == 1){
      display.println(F("Servo Adjust Min:  \n"));
      adjustValueByte(110);
    }
    else if(enterMenu2 == 2){
      display.println(F("Servo Adj Live Min:\n"));
      adjustServoLive(110);
    }
    else if(enterMenu2 == 3){
      display.println(F("Servo Adjust Max:  \n"));
      adjustValueByte(120);
    }
    else if(enterMenu2 == 4){
      display.println(F("Servo Adj Live Max:\n"));
      adjustServoLive(120);
    }
    else screenMenu1("Servo Adjust Min   \n",  "Servo Adj Live Min \n",  "Servo Adjust Max   \n",  "Servo Adj Live Max \n");
  }

  */

  else {screenUpdate("hh:mm", currentTemp, desiredTemp); firstLoop = 1;}
}

void setup() {
  
  delay(300);
  Serial.begin(74880);
  delay(300);

  #ifdef MACHINENUM
    EEPROM.write(201, MACHINENUM);
  #endif

  machineNum = EEPROM.read(201);

  if(machineNum > 10 || machineNum < 1)machineNum = 0;

  Serial.print(F("Machine="));
  Serial.print(machineNum);
  Serial.println(F("&Type=System&value=Temp Start"));
  
  //pinMode(1, INPUT_PULLUP);
  //pinMode(0, INPUT_PULLUP);
  delay(300);
  // put your setup code here, to run once:
  displaySetup();

  //Encoder
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);

  //Heating and Lights pin
  pinMode(lightsPin, OUTPUT); //set pin for thelights
  pinMode(heaterPin, OUTPUT); //set pin for the fan as output

  pinMode(lightsLED, OUTPUT);
  pinMode(coolerLED, OUTPUT);
  pinMode(heaterLED, OUTPUT);

  pinMode(coolerPin, OUTPUT);
  

  //Servo
  //myservo.attach(coolerPin);  // attaches the servo on pin 9 to the servo object

  while(!rtc.begin()) {
    display.println(F("Couldn't find RTC"));
    displayUpdate();
  }

  while(!max.begin(MAX31865_3WIRE)){
    display.println(F("Couldn't find Temp"));
    displayUpdate();
  }

  //Import and Optionally Reset EEPROM Values
  valImport();


  //Servo Default Position
  myservo.write((maxAngle-minAngle/2) + minAngle);
  delay(1000); // wait for console opening
  myservo.write(minAngle);

  //pinMode(A3, OUTPUT);

  //digitalWrite(A3, 1);

  minutetimer = millis();
  firstSend = 1;

  acWasOn = 0;
  acTimer = 0;
}

void loop() {
  encoderLast = encoderBut; 
  encoderBut = !digitalRead(4);
  encoderFalling = (!encoderLast && encoderBut);


  if(firstLoop){encoderOld = encoder.read()/4; encoderPos = 0; firstLoop = 0;}
  else encoderPos = encoder.read()/4 - encoderOld;

  
  
  refreshEverything();

  if(encoderBut){
    if(buttonHeld < 10)buttonHeld++;
  }
  else buttonHeld = 0;
  
  if(buttonHeld == 10 && !enterMenu){
    enterMenu = 1;
    encoderOld = encoder.read()/4; 
  }

  if(millis() - minutetimer > 300000 || firstSend){
    firstSend = 0;
    minutetimer = millis();
    Serial.print(F("Machine="));
    Serial.print(machineNum);
    Serial.print(F("&Type=Temperature&value="));
    Serial.println(currentTemp);

    Serial.print(F("Machine="));
    Serial.print(machineNum);
    Serial.print(F("&Type=Light&value="));
    Serial.println(dayStatus);
  }
  
  menus();
  lightControl();
  tempControlHeat();
  tempControlAC();

  if(lastmenu != enterMenu || encoderLastPos != encoderPos)menuTimeout = millis();
  if(millis() - menuTimeout > MENUTIMEOUT)enterMenu = 0;

  encoderLastPos = encoderPos;
  lastmenu = enterMenu;
}
/*
"Temp Adjust        \n", 
"Heat/Cool Adjust   \n", 
"Time Adjust        \n",
"Servo Adjust       \n");
"Day Temperature    \n", 
"Night Temperature  \n", 
"Correction Factor  \n", 
"Buffer Temperature \n"
"Heater On Adjust   \n", 
"Heater Off Adjust  \n", 
"Cooler On Adjust   \n", 
"Cooler Off Adjust  \n"
"Morning Hour       \n", 
"Night Hour         \n", 
"RTC Time Adjust    \n", 
"RTC Date Adjust    \n"


"Servo Adjust Min   \n", 
"Servo Adjust Max   \n", 
"Servo Adjust Min   \n", 
"Servo Adjust Max   \n"

"Servo Adjust Min   \n",  "Servo Adjust Max   \n",  "Servo Adjust Min   \n",  "Servo Adjust Max   \n"

"Morning Hour       \n", "Night Hour         \n", "RTC Time Adjust    \n", "RTC Date Adjust    \n"

"Heater On Adjust   \n", "Heater Off Adjust  \n", "Cooler On Adjust   \n", "Cooler Off Adjust  \n"

"Heater On sec(s):  \n", 
"Heater Off sec(s): \n", 
"Cooler On sec(s):  \n", 
"Cooler Off sec(s): \n"
/*
__Temp Sensor__
Day Temp - 10
Night Temp - 20
Correction Factor - 30
Buffer Temp - 40
__Heating__
Heating On - 50
Heating Off - 60
__Cooling__
Cooling On - 70
Cooling Off - 80
__Time__
Morning Time - 90
Day Time - 100
RTC Month, Day, Year
RTC Hours, Min, Second
 */
