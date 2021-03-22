/* AN-126 Example 2 uses kseries.h
  Reports values from a K-series sensor back to the computer written by Jason Berger
  Co2Meter.com
*/
#include "kSeries.h" //include kSeries Library
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Encoder.h>
#include <EEPROM.h>

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(OLED_RESET);

Encoder encoder(2, 3);


#define EN_BUTTON 4
#define CO2_FAN   6
#define CO2_FAN1   3
#define CO2_FAN2   5
#define LED_FAN   A1
#define CO2_SOL   7
#define SENSOR    A3
#define CO2_PUMP  10
#define LED_SOL   A0

#define TARGETCO2  400 //PPM
#define CORFACTOR  0   //PPM
#define BUFFERCO2  100 //PPM
#define OFFTIMEFAN 0   //Seconds
#define ONTIMEFAN  60  //Seconds
#define OFFTIMESOL 5   //Seconds
#define ONTIMESOL  100 //Milliseconds

#define AVGTIME 900000 //15 Minutes

//noise contrl
const int noise = 100;//0~255

//celebration
int cele_offset = 0;





bool firstRun, firstLoop, firstRead, firstSend;
int currentCO2Int = 0;

//encoder
long encoderPos;
long encoderOld = -999;
bool encoderBut, encoderFalling, encoderLast;

//menu
byte enterMenu  = 0;
byte enterMenu2 = 0;
int menuSteps = 0, time1, time2, time3;
unsigned long int loopCount;

//button
byte buttonHeld;

int co2;

kSeries K_30(12, 13); //Initialize a kSeries Sensor with pin 12 as Rx and 13 as Tx

int held, debounce = 250, lowbounce = 100, count = 0;
bool fanOn = 0, fanOff = 0;

unsigned long lastOn, lastOff;


//Saved Variables
int bufferrange, onTimeFan = 2, offTimeFan = 1, targetCO2 = 400, correctionCO2, bufferCO2, offTimeSol, onTimeSol;


//Data Variables
int minPPM = 10001, maxPPM = -1, avgPPM, avg[4];
unsigned long lastAvg;

unsigned long minutetimer;

//---------------------------  DISPLAY ----------------------
void displaySetup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x64)
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.setTextSize(0);
  display.setTextColor(0);
  display.setTextColor(1);
  display.setCursor(0, 0);
  //display.flip();
  display.display();
}

void displayUpdate() {
  //display.flip();
  display.display();
  display.clearDisplay();

  display.setCursor(0, 0);

}

void displayCO2() {
  display.setTextSize(2);
  display.print(co2); //print value delay(1500); //wait 1.5 seconds
  display.println(F(" ppm"));
  display.setTextSize(0);
  display.print(F("Target: "));
  display.print(targetCO2);
  display.println(F(" ppm"));
}

void sensorOutofRange() {
  digitalWrite(CO2_FAN, 0);
  digitalWrite(CO2_SOL, 0);
  if (count < 10)count++;
  if (count == 10) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(F("Sensor out of Range"));
    display.println(F("Check power sources"));
    displayUpdate();
  }
  delay(100);
  co2 = K_30.getCO2('p');//returns co2 value in ppm ('p') or percent ('%')
}

void regulationCO2() {
  if (targetCO2 > co2 + correctionCO2 + bufferCO2) {
    digitalWrite(CO2_FAN, 0);
    digitalWrite(LED_FAN, 0);
    if (millis() - lastOff > offTimeSol * 1000) {
      digitalWrite(CO2_SOL, 1);
      digitalWrite(LED_SOL, 1);
      delay(onTimeSol);
      digitalWrite(CO2_SOL, 0);
      digitalWrite(LED_SOL, 0);
      lastOff = millis();
    }
    display.println(F("Adding CO2"));
    fanOn = fanOff = 0;
  }


  else if (targetCO2 < co2 + correctionCO2 - bufferCO2) {
    display.print(F("Removing CO2 "));
    if ((!fanOn && !fanOff) || offTimeFan <= 0) {
      digitalWrite(CO2_FAN, 1);
      digitalWrite(LED_FAN, 1);
      //digitalWrite(13, 1);
      fanOn = 1;
      fanOff = 0;
      lastOn = millis();
      //display.print("x");
    }

    if (millis() - lastOn > onTimeFan * 1000 && fanOn) {
      digitalWrite(CO2_FAN, 0);
      digitalWrite(LED_FAN, 0);
      //digitalWrite(13, 0);
      fanOn = 0;
      fanOff = 1;
      lastOff = millis();
      //display.print("o");
    }

    if (millis() - lastOff > offTimeFan * 1000 && fanOff) {
      digitalWrite(CO2_FAN, 1);
      digitalWrite(LED_FAN, 1);
      //digitalWrite(13, 1);
      fanOn = 1;
      fanOff = 0;
      lastOn = millis();
      //display.print("x");
    }
  }

  else {
    digitalWrite(CO2_FAN, 0);
    digitalWrite(LED_FAN, 0);
    digitalWrite(CO2_SOL, 0);
    digitalWrite(LED_SOL, 0);
    display.println(F("CO2 in Range"));
  }
}


void screenMenu() {
  int menuPos = mod(encoderPos, 3);
  if (menuPos == 0)display.print(F(">"));
  display.print(F("C02 Adjust         \n"));
  if (menuPos == 1)display.print(F(">"));
  display.print(F("On/Off Time Adjust \n"));
  if (menuPos == 2)display.print(F(">"));
  display.println(F("Back\n"));

  if (encoderFalling) {
    enterMenu = 2 + menuPos;
    if (menuPos == 2)enterMenu = 0;
    firstLoop = 1;
  }
}

void screenMenu1() {
  int menuPos = mod(encoderPos, 5);

  if (menuPos == 0)display.print(F(">"));
  if (enterMenu == 2) display.println(F("Target CO2 PPM"));
  else if (enterMenu == 3) display.println(F("Off Time Fan Adjust"));

  if (menuPos == 1)display.print(F(">"));
  if (enterMenu == 2) display.println(F("CO2 PPM Correction"));
  else if (enterMenu == 3) display.println(F("On  Time Fan Adjust"));

  if (menuPos == 2)display.print(F(">"));
  if (enterMenu == 2) display.println(F("CO2 PPM Buffer"));
  else if (enterMenu == 3) display.println(F("Off Time Sol Adjust"));

  if (menuPos == 3)display.print(F(">"));
  if (enterMenu == 2) display.println(F("Show stats"));
  else if (enterMenu == 3) display.println(F("On  Time Sol Adjust"));


  if (menuPos == 4) {
    display.clearDisplay();
    display.setCursor(0, 0);
    if (enterMenu == 2) display.println(F("CO2 PPM Correction"));
    else if (enterMenu == 3) display.println(F("On  Time Fan Adjust"));

    if (enterMenu == 2) display.println(F("CO2 PPM Buffer"));
    else if (enterMenu == 3) display.println(F("Off Time Sol Adjust"));

    if (enterMenu == 2) display.println(F("Show stats"));
    else if (enterMenu == 3) display.println(F("On  Time Sol Adjust"));
    display.println(F(">Back"));
  }

  if (encoderFalling) {
    enterMenu2 = menuPos + 1;
    if (menuPos == 4)enterMenu = 1;
    firstLoop = 1;
    firstRead = 1;
  }
}

void adjustValue(byte location) {
  if (firstRead) {
    EEPROM.get(location, currentCO2Int);
    firstRead = 0;
  }

  int newVal = currentCO2Int + encoderPos * 1;

  display.setTextSize(2);
  display.println(newVal);
  display.setTextSize(0);

  if (encoderFalling) {
    if (newVal != currentCO2Int)EEPROM.put(location, newVal);
    firstLoop = 1;
    enterMenu2 = 0;

    if (location == 10)     EEPROM.get(10, targetCO2);
    else if (location == 20)EEPROM.get(20, correctionCO2);
    else if (location == 30)EEPROM.get(30, bufferCO2);
    else if (location == 40)EEPROM.get(40, offTimeFan);
    else if (location == 50)EEPROM.get(50, onTimeFan);
    else if (location == 60)EEPROM.get(60, offTimeSol);
    else if (location == 70)EEPROM.get(70, onTimeSol);
    //else if(location == 80)EEPROM.get(80, coolerOff);
  }
}

void valImport() {
  encoderOld = encoder.read() / 4;
  encoderPos = encoder.read() / 4 - encoderOld;

  bool flag = 0;
  bool resetMem = 0;

  EEPROM.get(10, targetCO2);
  celebrate();
  EEPROM.get(30, bufferCO2);
  EEPROM.get(40, offTimeFan);
  EEPROM.get(50, onTimeFan);
  EEPROM.get(60, offTimeSol);
  EEPROM.get(70, onTimeSol);

  if (targetCO2    <= 0 || targetCO2 > 10000)flag = 1;
  if (correctionCO2  < -2000 || correctionCO2  > 2000)flag = 1;
  if (bufferCO2 < 0 || bufferCO2 > 2000)flag = 1;
  if (offTimeFan  < 0 || offTimeFan  > 200)flag = 1;
  if (onTimeFan  < 0 || onTimeFan  > 200)flag = 1;
  if (offTimeSol < 0 || offTimeSol > 200)flag = 1;
  if (onTimeSol  < 0 || onTimeSol  > 200)flag = 1;
  if (!digitalRead(4))flag = 1;


  while (flag == 1) {
    encoderLast = encoderBut;
    encoderBut = !digitalRead(4);
    encoderFalling = (!encoderLast && encoderBut);

    encoderPos = encoder.read() / 4 - encoderOld;
    display.println(F("Invalid val detected"));
    display.println(F("Reset Saved Values?"));
    if (mod(encoderPos, 2) == 1) {
      display.println(F(">Yes    No"));
      displayUpdate();
      if (encoderFalling == 1) {
        resetMem = 1;
        break;
      }
    }
    else {
      display.println(F(" Yes   >No"));
      displayUpdate();
      if (encoderFalling == 1) {
        resetMem = 0;
        break;
      }
    }
  }

  if (resetMem == 1) {
    display.println(F("Resetting EEPROM..."));
    displayUpdate();
    EEPROM.put(10, (int)TARGETCO2);
    EEPROM.put(20, (int)CORFACTOR);
    EEPROM.put(30, (int)BUFFERCO2);
    EEPROM.put(40, (int)OFFTIMEFAN);
    EEPROM.put(50, (int)ONTIMEFAN);
    EEPROM.put(60, (int)OFFTIMESOL);
    EEPROM.put(70, (int)ONTIMESOL);
    display.println(F("Sucessfully reset"));
    displayUpdate();
    delay(500);

    EEPROM.get(10, targetCO2);
    EEPROM.get(20, correctionCO2);
    EEPROM.get(30, bufferCO2);
    EEPROM.get(40, offTimeFan);
    EEPROM.get(50, onTimeFan);
    EEPROM.get(60, offTimeSol);
    EEPROM.get(70, onTimeSol);
  }

}

void displayStats() {
  display.print(F("Min PPM: "));
  display.println(minPPM);
  display.print(F("Max PPM: "));
  display.println(maxPPM);
  display.print(F("Avg PPM (1 hr): "));
  display.println(avgPPM);
  display.print(F("Current PPM: "));
  display.println(co2);

  if (encoderFalling == 1) {
    enterMenu2 = enterMenu = 0;
  }
}

void stats() {
  if (co2 < minPPM)minPPM = co2;
  if (co2 > maxPPM)maxPPM = co2;
  if (millis() - lastAvg > AVGTIME) {
    avg[0] = avg[1];
    avg[1] = avg[2];
    avg[2] = avg[3];
    avg[3] = co2;

    lastAvg = millis();
  }

  avgPPM = (avg[0] + avg[1] + avg[2] + avg[3]) / 4;
}

int mod(long x, int y) {
  return x < 0 ? ((x + 1) % y) + y - 1 : x % y;
}

void celebrate() {
  digitalWrite(CO2_FAN, 1);
  analogWrite(CO2_FAN1, noise);
  analogWrite(CO2_FAN2, 0);
  display.println(F("Celebrating Sensor"));
  displayUpdate();
  delay(1000);
  //process //EEPROM.get(20, correctionCO2);
  int count = 0;
  int tempcorrection1 = K_30.getCO2('p');
  int tempcorrection2;
  while (1) {
    tempcorrection2=0;
    if (count < 30) {
      for (int i = 0; i < 11; i++) {
        tempcorrection2 += K_30.getCO2('p');
      }
      tempcorrection2 = tempcorrection2 / 10;
      display.setTextSize(2);
      display.print(tempcorrection2); //print value delay(1500); //wait 1.5 seconds
      display.println(F(" ppm"));
      display.setTextSize(0);
      display.print(F("Target: "));
      display.print(targetCO2);
      display.println(F(" ppm"));
      display.print(F("count: "));
      display.print(count);
      displayUpdate();
      if (tempcorrection1 == tempcorrection2) {
        count++;
      } else {
        count = 0;
      }
      tempcorrection1 = tempcorrection2;
    } else {
      break;
    }
  }
  correctionCO2 = (tempcorrection1 + tempcorrection1) / 2;
  analogWrite(CO2_FAN1, 0);
  analogWrite(CO2_FAN2, noise);
  display.println(F("Celebration finished."));
  displayUpdate();
}



void setup() {
  pinMode(CO2_FAN1, OUTPUT);
  pinMode(CO2_FAN2, OUTPUT);
  analogWrite(CO2_FAN1, 0);
  analogWrite(CO2_FAN2, noise);


  Serial.begin(74880);
  Serial.println("Machine=1&Type=System&value=CO2 Start");

  //Serial.begin(9600); //start a serial port to communicate with the computer
  //Serial.println(F("Serial Up!"));
  displaySetup();
  display.println(F("CO2 Viewer"));
  displayUpdate();

  pinMode(8, INPUT_PULLUP);
  pinMode(9, INPUT_PULLUP);
  pinMode(10, INPUT_PULLUP);

  pinMode(EN_BUTTON, INPUT_PULLUP);

  pinMode(LED_SOL, OUTPUT);
  digitalWrite(LED_SOL, 0);

  pinMode(LED_FAN, OUTPUT);
  digitalWrite(LED_FAN, 0);

  //SENSOR ON
  pinMode(SENSOR, OUTPUT);
  digitalWrite(SENSOR, 1);

  //PUMP ON
  pinMode(CO2_PUMP, OUTPUT);
  digitalWrite(CO2_PUMP, 1);


  //ADD CO2
  pinMode(CO2_SOL, OUTPUT);
  digitalWrite(CO2_SOL, 0);

  //Remove CO2
  pinMode(CO2_FAN, OUTPUT); //FAN MOSFET
  digitalWrite(CO2_FAN, 0);

  pinMode(13, OUTPUT);

  delay(500);

  valImport();

  delay(500);

  avg[0] = avg[1] = avg[2] = avg[3] = co2 = K_30.getCO2('p');
  lastAvg = millis();

  minutetimer = millis();

  firstSend = 1;
}


void loop() {


  count = 0;
  encoderLast = encoderBut;
  encoderBut = !digitalRead(4);
  encoderFalling = (!encoderLast && encoderBut);

  if (firstLoop) {
    encoderOld = encoder.read() / 4;
    encoderPos = 0;
    firstLoop = 0;
  }
  else encoderPos = encoder.read() / 4 - encoderOld;

  if (encoderBut) {
    if (buttonHeld < 10)buttonHeld++;
  }
  else buttonHeld = 0;

  if (buttonHeld == 10 && !enterMenu) {
    enterMenu = 1;
    encoderOld = encoder.read() / 4;
  }

  co2 = K_30.getCO2('p');
  while (co2 <= 0) {
    sensorOutofRange();
    //digitalWrite(SENSOR, 0);
    //delay(2500);
    //digitalWrite(SENSOR, 1);
    //delay(2500);
  }

  stats();

  if (enterMenu == 1) {
    screenMenu ();
  }

  else if (enterMenu == 2) {
    if (enterMenu2 == 1) {
      display.println(F("Target CO2:        \n"));
      adjustValue(10);
    }
    else if (enterMenu2 == 2) {
      display.println(F("CO2 Offset:        \n"));
      adjustValue(20);
    }
    else if (enterMenu2 == 3) {
      display.println(F("CO2 Buffer:        \n"));
      adjustValue(30);
    }
    else if (enterMenu2 == 4) {
      //Show stats..
      displayStats();
    }
    else screenMenu1();
  }



  else if (enterMenu == 3) {
    if (enterMenu2 == 1) {
      display.println(F("Off Time Fan (s):  \n"));
      adjustValue(40);
    }
    else if (enterMenu2 == 2) {
      display.println(F("On Time Fan (s):   \n"));
      adjustValue(50);
    }
    else if (enterMenu2 == 3) {
      display.println(F("Off Time Sol (s)  :\n"));
      adjustValue(60);
    }
    else if (enterMenu2 == 4) {
      display.println(F("On Time Sol (ms)  :\n"));
      adjustValue(70);
    }
    else screenMenu1();
  }

  else displayCO2();

  regulationCO2();
  displayUpdate();

  if (millis() - minutetimer > 300000 || firstSend) {
    firstSend = 0;
    minutetimer = millis();
    Serial.print("Machine=1&Type=CO2&value=");
    Serial.println(co2);
  }
}
