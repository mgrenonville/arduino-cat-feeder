//****************************************************************
// Cat Feeder !
//****************************************************************
#include <Wire.h>
#include "RTClib.h"
#include <EEPROM.h>
#include <avr/EEPROM.h>
#include <LiquidCrystal.h>

#define lastFoodAddress 0 // 4 bytes
#define numberOfDistributionAddress 4 // 1 byte
#define totalDistributionDurationAddress 5 // 4 bytes


byte timer_left[8] = {
  B00011,
  B00111,
  B01111,
  B01111,
  B01110,
  B01100,
  B00011,
};
byte timer_right[8] = {
  B10000,
  B01000,
  B00100,
  B00100,
  B00100,
  B01000,
  B10000,
};


// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(5, 4, 7, 3, 6, 2);

RTC_DS1307 RTC;

int RELAY_OUTPUT = 9;
int numberOfDistribution = 0;
int totalDuration = 2000;

void distributeFood(){
  Serial.print("distributeFood() ! on ");
  Serial.print(RELAY_OUTPUT,DEC);
  digitalWrite(RELAY_OUTPUT, HIGH);
  // 120 rmp / 6 division => 1 division = 33ms
  // delay(150);
  delay(totalDuration / numberOfDistribution);
  digitalWrite(RELAY_OUTPUT, LOW);

}

void setup(){

  Serial.begin(57600);
  Wire.begin();
  RTC.begin();


  lcd.createChar(0, timer_left);
  lcd.createChar(1, timer_right);
  // set up the LCD's number of columns and rows: 
  lcd.begin(16,2);
  lcd.print("Initialisation...");
  pinMode(RELAY_OUTPUT, OUTPUT);
  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    Serial.print("Setting to ");
    Serial.print(__DATE__);
    Serial.print(":");
    Serial.println(__TIME__);
    RTC.adjust(DateTime(__DATE__, __TIME__));
    DateTime now = RTC.now();
    writeDateInEprom(now);
  }
  numberOfDistribution = EEPROM.read(numberOfDistributionAddress);
  if(numberOfDistribution == 0 || numberOfDistribution > 99) {
    EEPROM.write(numberOfDistributionAddress, 24);
    numberOfDistribution = EEPROM.read(numberOfDistributionAddress);
    lcd.setCursor(0, 1);
    lcd.print("Number of d forced to 8");
  }

  totalDuration = readTotalDurationInEEPROM();
  if(totalDuration < 0 || totalDuration > 10000) {
    writeTotalDurationInEEPROM(2000);
    totalDuration = readTotalDurationInEEPROM();
  }



  Serial.print("init : ");
  Serial.print(totalDuration);


}

//****************************************************************
//****************************************************************
//****************************************************************
void loop(){
  delay(1000);
  lcd.setCursor(0, 0);
  lcd.clear();
  verifyTimeAndFeedIfNessecary();

  waitForInputAction();



}

void waitForInputAction(){
  if(Serial.available() > 0){
    int inByte = Serial.read();
    if(inByte == 'f'){
      distributeFood();
    }    
    else if(inByte == 'h') {
      Serial.println("Help : ");
      Serial.println("   f : distribute Food ");
      Serial.println("   r : Reset lastfood date");
      Serial.println("   a : Adjust time of RTC");
      Serial.println("   s : Save parameters");
      Serial.println("   - : Decrease distribution/day");
      Serial.println("   + : Increase distribution/day");
    }
    else if(inByte == 'r') {
      DateTime now = RTC.now();
      writeDateInEprom(now);
    } 
    else if(inByte == 'a'){
      Serial.print("Setting to RTC to ");
      Serial.print(__DATE__);
      Serial.print(":");
      Serial.println(__TIME__);
      RTC.adjust(DateTime(__DATE__, __TIME__));

    } 
    else if(inByte == '+'){
      numberOfDistribution++;
    } 
    else if(inByte == '-'){
      numberOfDistribution--;
    }
    else if(inByte == 's'){
      saveConfiguration();
    }

  } 
}

void verifyTimeAndFeedIfNessecary() {

  DateTime now = RTC.now();


  uint32_t lastFoodTime = retrieveLastFoodTime();
  long nextFoodTime = computeNextFoodTime(lastFoodTime);

  logRS232(now, lastFoodTime , nextFoodTime);
  drawLCDPanel(now, nextFoodTime);
  if(now.unixtime()  > nextFoodTime){
    Serial.print("C'mon ! It's feeding time !");
    writeDateInEprom(now);
    distributeFood();
  } 
  else {
    Serial.print("Not now ! Sorry cats !");
  }
  Serial.println();

}



uint32_t retrieveLastFoodTime(){
  uint32_t lastFoodTime = 0;
  eeprom_read_block((void *) &lastFoodTime, (unsigned char *) lastFoodAddress, 4);
  return lastFoodTime;
}

long computeNextFoodTime(uint32_t lastFoodTime){

  long durationBetweenDistribution =  (86400) / numberOfDistribution;
  long nextFoodTime = lastFoodTime + durationBetweenDistribution;
  return nextFoodTime;
}
void logRS232(DateTime now, uint32_t lastFoodTime ,long nextFoodTime){
  Serial.println();
  Serial.print("now : ");
  printDate(now);

  Serial.print("last food date : ");
  printDate( DateTime (lastFoodTime));
  Serial.print("next food date : ");
  printDate( DateTime (nextFoodTime));

  Serial.print("duration : ");
  Serial.print(totalDuration / numberOfDistribution);

  Serial.println();

  Serial.print("number of distribution : ");
  Serial.print(numberOfDistribution);


  Serial.println();


}
void drawLCDPanel(DateTime now, long nextFoodTime){
  // FIRST LINE
  lcd.setCursor(0, 0);
  printDate_LCD(now);
  printNumberOfDistribution();
  // SECOND LINE
  lcd.setCursor(0, 1);
  lcd.write(byte(0));
  lcd.write(byte(1));
  printDuration(abs(nextFoodTime - now.unixtime()));

}

void printDuration(long duration){
  int ss = duration % 60;
  duration /= 60;
  int mm = duration % 60;
  duration/= 60;
  int hh = duration;

  printIntOnTwoChar_LCD(hh);
  lcd.print(":");
  printIntOnTwoChar_LCD(mm);
  lcd.print(":");
  printIntOnTwoChar_LCD(ss);



}



void printNumberOfDistribution(){
  lcd.print("   ");
  lcd.print(numberOfDistribution);
  lcd.print("d/j");
}
void printDate(DateTime now) {
  Serial.print(now.year(), DEC);
  Serial.print('/');
  printIntOnTwoChar(now.month());
  Serial.print('/');
  printIntOnTwoChar(now.day());
  Serial.print(' ');
  printIntOnTwoChar(now.hour());
  Serial.print(':');
  printIntOnTwoChar(now.minute());
  Serial.print(':');  
  printIntOnTwoChar(now.second());
  Serial.println();
}
void printDate_LCD(DateTime now) {
  //  lcd.print(now.year(), DEC);
  //  lcd.print('/');
  //  printIntOnTwoChar_LCD(now.month());
  //  lcd.print('/');
  //  printIntOnTwoChar_LCD(now.day());
  //  lcd.print(' ');
  printIntOnTwoChar_LCD(now.hour());
  lcd.print(':');
  printIntOnTwoChar_LCD(now.minute());
  lcd.print(':');  
  printIntOnTwoChar_LCD(now.second());
}

inline void printIntOnTwoChar(int val){
  if(val <10){
    Serial.print('0');
  }
  Serial.print(val, DEC);
}

inline void printIntOnTwoChar_LCD(int val){
  if(val <10){
    lcd.print('0');
  }
  lcd.print(val, DEC);
}

void writeDateInEprom(DateTime now){
  EEPROM.write(lastFoodAddress, (now.unixtime() & 0xFF));
  EEPROM.write(lastFoodAddress + 1, (now.unixtime()>>8) & 0xFF);
  EEPROM.write(lastFoodAddress + 2, (now.unixtime()>>16) & 0xFF);
  EEPROM.write(lastFoodAddress + 3, (now.unixtime()>>24) & 0xFF);
}


void writeTotalDurationInEEPROM(long totalDuration){
  eeprom_write_block((const void*)&totalDuration, (void*)totalDistributionDurationAddress, sizeof(totalDuration));
}


long readTotalDurationInEEPROM(){
  long totalDuration = 0;
  eeprom_read_block((void *)&totalDuration, (unsigned char *)totalDistributionDurationAddress, sizeof(totalDuration));
  return totalDuration;
}


long saveConfiguration(){
  EEPROM.write(numberOfDistributionAddress, numberOfDistribution);
}











