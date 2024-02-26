#include <avr/io.h>
#include <util/delay.h>
#include <DS3231.h>
#include <Wire.h>
#include <LiquidCrystal.h>

/* LCD pins
8 -> PORTD7
9 -> PORTD8
4 -> PORTD3
5 -> PORTD4
6 -> PORTD5
7 -> PORTD6
*/

DS3231 Clock;
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
bool lightPeriod = false;

/*****************************************************************************************************************/

const uint8_t ALRM1_MATCH_EVERY_SEC      = 0b1111;  // once a second
const uint8_t ALRM1_MATCH_SEC            = 0b1110;  // when seconds match
const uint8_t ALRM1_MATCH_MIN_SEC        = 0b1100;  // when minutes and seconds match
const uint8_t ALRM1_MATCH_HR_MIN_SEC     = 0b1000;  // when hours, minutes, and seconds match
const uint8_t ALRM1_MATCH_DY_HR_MIN_SEC  = 0b0000;  // when hours, minutes, and seconds match

const uint8_t ALRM2_ONCE_PER_MIN         = 0b0111;   // once per minute (00 seconds of every minute)
const uint8_t ALRM2_MATCH_MIN            = 0b0110;   // when minutes match
const uint8_t ALRM2_MATCH_HR_MIN         = 0b0100;   // when hours and minutes match

/*****************************************************************************************************************/

enum menuScreens {  

  modeSHOWDATETIME,
  modeSHOWALARM1,
  modeSHOWALARM2,
  modeSETDATE,
  modeSETTIME,
  modeSETALARM1ON,
  modeSETALARM1,
  modeSETALARM1METHOD,
  modeSETALARM2ON,
  modeSETALARM2,
  modeSETALARM2METHOD,
};

uint8_t currentMode = modeSHOWDATETIME;

// buffers for lcd display
char line0[21]; 
char line1[21];

enum buttons {

  btnRIGHT,
  btnUP,
  btnDOWN,
  btnLEFT,
  btnSELECT,
  btnNONE,
};

uint8_t lcd_key = 0;
const uint8_t bounceDelay = 50;
uint8_t oldKey = 0;

//Variables to control menu position
bool blinkNow = false;
uint8_t blinkInt = 1;
uint8_t maxBlinkInt = 3;
uint32_t blinkStart = 0;
uint32_t blinkDelay = 500;

/****************************************************************************************************************************************************************************************************************************
 * Set up and Loop
 ****************************************************************************************************************************************************************************************************************************/

//int main(void) {
void setup(){

DDRB |= (1 << PB5); // ledpin for testing. to be adjusted for real aquarium use
PORTB &= ~(1 << PB5);

DDRC &= ~(1 << PC0); // portA pins as input for buttons
DDRC &= ~(1 << PC1);
DDRC &= ~(1 << PC2);
DDRC &= ~(1 << PC3);
DDRC &= ~(1 << PC4);

PORTC |= (1 << PC0); // pullup resistors for buttons
PORTC |= (1 << PC1);
PORTC |= (1 << PC2);
PORTC |= (1 << PC3);
PORTC |= (1 << PC4);

lcd.begin(16, 2);
lcd.setCursor(0, 0);
Wire.begin();
}

//while(1) {
void loop(){
  
  alarm();
  setMenu();

  lcd_key = read_LCD_buttons();  

  if (blinkInt > maxBlinkInt) blinkInt = maxBlinkInt;

  if (currentMode > modeSHOWALARM2 && ((millis() - blinkStart) > blinkDelay)){
    blinkNow =! blinkNow;
    blinkStart = millis();
    } 
  else if (currentMode <= modeSHOWALARM2) blinkNow = false; 

  // check the key press against the previous key press to avoid issues from long key presses
  if (oldKey != lcd_key) {
    // depending on which button was pushed, we perform an action
    switch (lcd_key)               
    {     
      case btnRIGHT:
      {
        oldKey = btnRIGHT;
        if (blinkInt < maxBlinkInt) blinkInt += 1;

        if (currentMode == modeSHOWDATETIME) currentMode = modeSHOWALARM1;
        else if (currentMode == modeSHOWALARM1) currentMode = modeSHOWALARM2;
        else if (currentMode == modeSHOWALARM2) currentMode = modeSHOWDATETIME;
        _delay_ms(50);
        break;
      }
      case btnLEFT:
      {
        oldKey = btnLEFT;
        if (blinkInt > 1) blinkInt -= 1;
        _delay_ms(50);
        break;
      }
      case btnUP:
      {
        oldKey = btnUP;
        if (currentMode == modeSETDATE){
          if (blinkInt == 1) increaseDate();
          if (blinkInt == 2) increaseMonth();
          if (blinkInt == 3) increaseYear();
        }
        if (currentMode == modeSETTIME){
          if (blinkInt == 1) increaseHour();
          if (blinkInt == 2) increaseMinute();
        }
        if (currentMode == modeSETALARM1ON) {
          if (Clock.checkAlarmEnabled(1)) AlarmOn(1, false);
        }
        if (currentMode == modeSETALARM1) {
          if (blinkInt == 1) changeAlarmDayOption(1);
          if (blinkInt == 2) ChangeAlarm(1, 1, 0, 0, 0);
          if (blinkInt == 3) ChangeAlarm(1, 0, 1, 0, 0);
          if (blinkInt == 4) ChangeAlarm(1, 0, 0, 1, 0);
          if (blinkInt == 5) ChangeAlarm(1, 0, 0, 0, 1); 
        }
        if (currentMode == modeSETALARM1METHOD) changeAlarmMethod(1, 1);
        
        if (currentMode == modeSETALARM2ON) {
          if (Clock.checkAlarmEnabled(2)) AlarmOn(2, false);
        }
        if (currentMode == modeSETALARM2) {
          if (blinkInt == 1) changeAlarmDayOption(2);
          if (blinkInt == 2) ChangeAlarm(2, 1, 0, 0, 0);
          if (blinkInt == 3) ChangeAlarm(2, 0, 1, 0, 0);
          if (blinkInt == 4) ChangeAlarm(2, 0, 0, 1, 0);
          if (blinkInt == 5) ChangeAlarm(2, 0, 0, 0, 1);
        }
        if (currentMode == modeSETALARM2METHOD) {
          changeAlarmMethod(2, 1);
        }
        _delay_ms(50);
        break;
      }
      case btnDOWN:
      {
        oldKey = btnDOWN;
        if (currentMode == modeSETDATE){
          if (blinkInt == 1) decreaseDate();
          if (blinkInt == 2) decreaseMonth();
          if (blinkInt == 3) decreaseYear();
        }
        if (currentMode == modeSETTIME){
          if (blinkInt == 1) decreaseHour();
          if (blinkInt == 2) decreaseMinute();
        }
        if (currentMode == modeSETALARM1ON) {
          if (!Clock.checkAlarmEnabled(1)) AlarmOn(1, true);
        }
        if (currentMode == modeSETALARM1) {
          if (blinkInt == 1) changeAlarmDayOption(1);
          if (blinkInt == 2) ChangeAlarm(1, -1, 0, 0, 0);
          if (blinkInt == 3) ChangeAlarm(1, 0, -1, 0, 0);
          if (blinkInt == 4) ChangeAlarm(1, 0, 0, -1, 0);
          if (blinkInt == 5) ChangeAlarm(1, 0, 0, 0, -1);
        }
        if (currentMode == modeSETALARM1METHOD) changeAlarmMethod(1, 0);
        
        if (currentMode == modeSETALARM2ON) {
          if (!Clock.checkAlarmEnabled(2)) AlarmOn(2, true);
        }
        if (currentMode == modeSETALARM2) {
          if (blinkInt == 1) changeAlarmDayOption(2);
          if (blinkInt == 2) ChangeAlarm(2, -1, 0, 0, 0);
          if (blinkInt == 3) ChangeAlarm(2, 0, -1, 0, 0);
          if (blinkInt == 4) ChangeAlarm(2, 0, 0, -1, 0);
          if (blinkInt == 5) ChangeAlarm(2, 0, 0, 0, -1);
        }
        if (currentMode == modeSETALARM2METHOD) changeAlarmMethod(2, 0);
      
        _delay_ms(50);
        break;
      }
      case btnSELECT:
      {
        blinkInt = 1;
        oldKey = btnSELECT;
        if (currentMode == modeSHOWDATETIME) currentMode = modeSETDATE;
        else if (currentMode == modeSHOWALARM1) currentMode = modeSETALARM1ON;
        else if (currentMode == modeSHOWALARM2) currentMode = modeSETALARM2ON;
        else if (currentMode == modeSETDATE) currentMode = modeSETTIME;
        else if (currentMode == modeSETTIME) currentMode = modeSHOWDATETIME;
        else if (currentMode == modeSETALARM1ON && Clock.checkAlarmEnabled(1)) currentMode = modeSETALARM1;
        else if (currentMode == modeSETALARM1ON && !Clock.checkAlarmEnabled(1)) currentMode = modeSHOWALARM1;
        else if (currentMode == modeSETALARM1) currentMode = modeSETALARM1METHOD;
        else if (currentMode == modeSETALARM1METHOD) currentMode = modeSHOWALARM1;
        else if (currentMode == modeSETALARM2ON && Clock.checkAlarmEnabled(2)) currentMode = modeSETALARM2;
        else if (currentMode == modeSETALARM2ON && !Clock.checkAlarmEnabled(2)) currentMode = modeSHOWALARM2;
        else if (currentMode == modeSETALARM2) currentMode = modeSETALARM2METHOD;
        else if (currentMode == modeSETALARM2METHOD) currentMode = modeSHOWALARM2;
        break;
      }
      case btnNONE:
      {
        oldKey = btnNONE;
        break;
      }
    }
  }
}
//}

/***************************************************************************************************************************************************************************************************************************
 * Menu Control
 ***************************************************************************************************************************************************************************************************************************/

void setMenu(){

  switch (currentMode)
  {
    case modeSHOWDATETIME:

         showDateTime();
         break;
    
    case modeSHOWALARM1:
    
         showAlarm1();
         break;
    
    case modeSHOWALARM2:
    
         showAlarm2();
         break;
    
    case modeSETDATE:
    
         setDate();
         break;
    
    case modeSETTIME:
    
         setTime();
         break;
    
    case modeSETALARM1ON:
    
         setAlarmOnOff(1);
         break;
    
    case modeSETALARM1:
    
         setAlarm(1);
         break;
       
    case modeSETALARM1METHOD:
    
         setAlarmMethod(1);
         break;
    
    case modeSETALARM2ON:
    
         setAlarmOnOff(2);
         break;
    
    case modeSETALARM2:
    
         setAlarm(2);
         break;
    
    case modeSETALARM2METHOD:
    
         setAlarmMethod(2);
         break;  
  }
}

/***************************************************************************************************************************************************************************************************************************
 * Function to read the buttons
 **************************************************************************************************************************************************************************************************************************/

// read the buttons
byte read_LCD_buttons(){

 if (!(PINC & (1 << PC0)))  return btnLEFT;
 if (!(PINC & (1 << PC1)))  return btnUP;
 if (!(PINC & (1 << PC2)))  return btnRIGHT;
 if (!(PINC & (1 << PC3)))  return btnDOWN;
 if (!(PINC & (1 << PC4)))  return btnSELECT; 
 return btnNONE;  // when all else fails, return this...
}

/***************************************************************************************************************************************************************************************************************************
 * Functions to read and display the time
 **************************************************************************************************************************************************************************************************************************/

void displayText(String line0Text, String line1Text){
  lcd.setCursor(0, 0);
  sprintf(line0,"%-21s", line0Text.c_str());
  lcd.print(String(line0));
  lcd.setCursor(0, 1);
  sprintf(line1,"%-21s", line1Text.c_str());
  lcd.print(String(line1));  
}

String dateText() {  
  String result = "Date: ";
  bool Century = false;
  if (blinkInt != 1 || blinkNow == false) result += twoDigitNumber(Clock.getDate());
  else result += "  ";

  result += "/";

  if (blinkInt != 2 || blinkNow == false) result += twoDigitNumber(Clock.getMonth(Century));
  else result += "  ";

  result += "/";

  if (blinkInt != 3 || blinkNow == false) result += twoDigitNumber(Clock.getYear());
  else result += "  ";

  return result;
}

String timeText() {  
  String result = "Time: ";
  bool h12, PM;
  if (blinkInt!=1 || blinkNow == false) result += twoDigitNumber(Clock.getHour(h12, PM));
  else result += "  ";

  result += ":";

  if (blinkInt!=2 || blinkNow == false) result += twoDigitNumber(Clock.getMinute());
  else result += "  ";

  result += ":";

  result += twoDigitNumber(Clock.getSecond()); 

  return result;
}

void showDateTime(){  
  displayText(dateText(), timeText());
}

void showAlarm1(){

  uint8_t ADay, AHour, AMinute, ASecond, ABits;
  bool ADy, A12h, Apm;

  String result1 = "Alarm1: ";
  if (Clock.checkAlarmEnabled(1)) {  
    Clock.getA1Time(ADay, AHour, AMinute, ASecond, ABits, ADy, A12h, Apm);
    if (blinkInt != 1 || blinkNow == false){
      if (ADy) result1 += "DOW  ";
      else result1 += "DATE "; 
    }
    else result1 += "     ";

    if (blinkInt != 2 || blinkNow == false) result1 += ADay;     
    String result2 = "Alarm1: ";

    if (blinkInt != 3 || blinkNow == false) result2 += twoDigitNumber(AHour);
    else result2 += "  ";

    result2 += ":";

    if (blinkInt != 4 || blinkNow == false) result2 += twoDigitNumber(AMinute);
    else result2 += "  ";

    result2 += ":";

    if (blinkInt != 5 || blinkNow == false) result2 += twoDigitNumber(ASecond);
  
    displayText(result1, result2);
  }
  else displayText("Alarm1: Off", "");
}

void showAlarm2(){

  uint8_t ADay, AHour, AMinute, ASecond, ABits;
  bool ADy, A12h, Apm;

  if (Clock.checkAlarmEnabled(2)) {
    String result1 = "Alarm2: ";    
    Clock.getA2Time(ADay, AHour, AMinute, ABits, ADy, A12h, Apm);
    if (blinkInt != 1 || blinkNow == false){
      if (ADy) result1 += "DOW  ";
      else result1 += "DATE ";
    }
    else result1 += "     "; 

    if (blinkInt != 2 || blinkNow == false) result1 += ADay;   
    else result1 += "  ";

    String result2 = "Alarm2: ";  
    if (blinkInt != 3 || blinkNow == false) result2 += twoDigitNumber(AHour);
    else result2 += "  ";

    result2 += ":"; 

    if (blinkInt != 4 || blinkNow == false) result2 += twoDigitNumber(AMinute);
    else result2 += "  ";  
    displayText(result1, result2);
  }
  else displayText("Alarm2: Off", "");
}

void showAlarmMethod(int alarmNum) {

  String myString1 = "";
  String myString2 = "";

  uint8_t ADay, AHour, AMinute, ASecond, ABitsOP = 0b0;
  bool ADy, A12h, Apm;

  if (alarmNum == 1){

    myString1 = "Alarm 1 Method:";
    
    Clock.getA1Time(ADay, AHour, AMinute, ASecond, ABitsOP, ADy, A12h, Apm);  

    ABitsOP  = ABitsOP & 0b1111;
    
    if (ABitsOP == ALRM1_MATCH_EVERY_SEC) myString2 = "Once per Second";
    else if (ABitsOP == ALRM1_MATCH_SEC) myString2 = "Seconds Match";
    else if (ABitsOP == ALRM1_MATCH_MIN_SEC) myString2 = "Min & Secs Match";
    else if (ABitsOP == ALRM1_MATCH_HR_MIN_SEC) myString2 = "Hr, Min & Sec Match";
    else if (ABitsOP == ALRM1_MATCH_DY_HR_MIN_SEC) myString2 = "Dy, Hr, Mn & Sec";
    } else {
    
    Clock.getA2Time(ADay, AHour, AMinute, ABitsOP, ADy, A12h, Apm);  
    
    myString1 = "Alarm 2 Method:";
    
    if ((ABitsOP >> 4) == ALRM2_ONCE_PER_MIN) myString2 = "Once per Minute";
    else if ((ABitsOP >> 4) == ALRM2_MATCH_MIN) myString2 = "Match Minute";
    else myString2 = "Match Hour & Min";
  }  
  displayText(myString1 , myString2);
}

String twoDigitNumber(uint8_t number){
  
  char buffer[3];
  snprintf(buffer,sizeof(buffer), "%02d", number );
  return buffer;
}

/***************************************************************************************************************************************************************************************************************************
 * Functions to set the time
 **************************************************************************************************************************************************************************************************************************/

void setDate(){
  displayText("Set the date:", dateText());
  maxBlinkInt = 3;  
}

void setTime(){
  displayText("Set the time:", timeText());
  maxBlinkInt = 2;  
}

void setAlarmOnOff(int alarmNum){
  if (alarmNum > 0 && alarmNum < 3) {  
    maxBlinkInt = 1;    
    if(Clock.checkAlarmEnabled(alarmNum)) blinkInt = 2;
    else blinkInt = 1;    
    if (blinkInt == 1 && blinkNow == true) displayText("", "Alarm" + String(alarmNum) + ": ON");
    else if (blinkInt == 2 && blinkNow == true) displayText("Alarm" + String(alarmNum) + ": OFF", "");
    else displayText("Alarm" + String(alarmNum) + ": OFF", "Alarm" + String(alarmNum) + ": ON");
  }
}

void setAlarm(int alarmNum){
  if (alarmNum > 0 && alarmNum < 3) {  
    if (alarmNum == 1){
      maxBlinkInt = 5;    
      showAlarm1();
    }
    if (alarmNum == 2){
      maxBlinkInt = 4;    
      showAlarm2();
    }
  }
}

void setAlarmMethod(int alarmNum){

  if (alarmNum > 0 && alarmNum < 3){    

    if (alarmNum == 1){
      maxBlinkInt = 1;    
      showAlarmMethod(1);
    }
    if (alarmNum == 2){
      maxBlinkInt = 1;    
      showAlarmMethod(2);
    }
  }
}

/***************************************************************************************************************************************************************************************************************************
 * Functions to increase and decrease time elements
 **************************************************************************************************************************************************************************************************************************/

void increaseYear(){
  bool Century = false;
  uint8_t Year = Clock.getYear();

  if (Year < 99) Year = Year + 1;
  else Year = 00;

  Clock.setYear(Year);  
  if (Clock.getDate() > monthMaxDays(Clock.getMonth(Century))) Clock.setDate(monthMaxDays(Clock.getMonth(Century)));
}

void decreaseYear(){
  bool Century = false;
  uint8_t Year = Clock.getYear();  

  if (Year > 1) Year = Year - 1;
  else Year = 99;   

  Clock.setYear(Year); 
   if (Clock.getDate() > monthMaxDays(Clock.getMonth(Century))) Clock.setDate(monthMaxDays(Clock.getMonth(Century))); 
}

void increaseMonth(){
  bool Century = false;
  uint8_t Month = Clock.getMonth(Century);

  if (Month < 12) Month = Month + 1;
  else Month = 1;
  
  Clock.setMonth(Month);  
  if (Clock.getDate() > monthMaxDays(Clock.getMonth(Century))) Clock.setDate(monthMaxDays(Clock.getMonth(Century)));
}

void decreaseMonth(){
  bool Century = false;
  uint8_t Month = Clock.getMonth(Century);

  if (Month > 1) Month = Month - 1;
  else Month = 12;
   
  Clock.setMonth(Month); 
  if (Clock.getDate() > monthMaxDays(Clock.getMonth(Century))) Clock.setDate(monthMaxDays(Clock.getMonth(Century)));
  
}

void increaseDate(){ 
  bool Century = false; 
  uint8_t Date = Clock.getDate();

  if (Date < monthMaxDays(Clock.getMonth(Century))) Date = Date + 1;
  else Date = 1;
     
  Clock.setDate(Date);  
}

void decreaseDate(){ 
  bool Century = false; 
  uint8_t Date = Clock.getDate();  

  if(Date > 1) Date = Date- 1;    
  else Date = monthMaxDays(Clock.getMonth(Century));

  Clock.setDate(Date);    
}

void increaseHour(){
  bool h12, PM;
  uint8_t Hour = Clock.getHour(h12, PM);

  if (Hour < 23) Hour = Hour + 1;
  else Hour = 1;

  Clock.setHour(Hour);  
}

void decreaseHour(){
  bool h12, PM;
  uint8_t Hour = Clock.getHour(h12, PM);

  if (Hour > 1) Hour = Hour - 1;
  else Hour = 23;

  Clock.setHour(Hour);  
}

void increaseMinute(){
  uint8_t Minute = Clock.getMinute();

  if (Minute < 59) Minute = Minute + 1;
  else Minute = 1;

  Clock.setMinute(Minute);  
  Clock.setSecond(0);  
}

void decreaseMinute(){
  uint8_t Minute = Clock.getMinute();

  if (Minute > 0) Minute = Minute - 1;
  else Minute = 59;

  Clock.setMinute(Minute);  
  Clock.setSecond(0);  
}

int monthMaxDays(int monthNumber){
  switch (monthNumber){
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
      return 31;
    case 4:
    case 6:
    case 9:
    case 11:
      return 30;
    case 2:
      int remainingYears;
      remainingYears = ((Clock.getYear() - 2020) % 4);
      if (remainingYears == 0) return 29;
      else return 28;

   default:
    return 0;
  }
}

/**********************************************************************************************************************************************************************************************************************
 * Functions to set Alarms
 *********************************************************************************************************************************************************************************************************************/

 void AlarmOn(int alarmNum, bool setOn){
  if (alarmNum > 0 && alarmNum < 3) {
    if (setOn) Clock.turnOnAlarm(alarmNum);
    else Clock.turnOffAlarm(alarmNum);
  }  
 }

 void changeAlarmDayOption(int alarmNum){

  uint8_t ADay, AHour, AMinute, ASecond, ABits;
  bool ADy, A12h, Apm;

  //Collect the current alarm settings
  if (alarmNum == 1) Clock.getA1Time(ADay, AHour, AMinute, ASecond, ABits, ADy, A12h, Apm);
  if (alarmNum == 2) Clock.getA2Time(ADay, AHour, AMinute, ABits, ADy, A12h, Apm); 

  ADy =! ADy;

  if (ADy && ADay > 7) ADay = 7;

  //Reset the alarm settings
  if (alarmNum == 1) Clock.setA1Time(ADay, AHour, AMinute, ASecond, ABits, ADy, A12h, Apm);
  if (alarmNum == 2) Clock.setA2Time(ADay, AHour, AMinute, ABits, ADy, A12h, Apm);
}

 void ChangeAlarm(int alarmNum, int dayAdjust, int hourAdjust, int minAdjust, int secAdjust){

  uint8_t ADay, AHour, AMinute, ASecond, ABits;
  bool ADy, A12h, Apm;

  //Collect the current alarm settings
  if (alarmNum == 1) Clock.getA1Time(ADay, AHour, AMinute, ASecond, ABits, ADy, A12h, Apm);
  if (alarmNum == 2) Clock.getA2Time(ADay, AHour, AMinute, ABits, ADy, A12h, Apm);

  //Adjust the date
  ADay += dayAdjust;
  if (ADy){
    if (ADay < 1) ADay = 7;
    if (ADay > 7) ADay = 1;
  }
  else {
    if (ADay < 1) ADay = 31;
    if (ADay > 31) ADay = 1;
  }

  //Adjust the hour
  AHour += hourAdjust;
  if (AHour < 0) AHour = 3;
  if (AHour > 23) AHour = 0;

  //Adjust the minute
  AMinute += minAdjust;
  if (AMinute < 0) AMinute = 59;
  if (AMinute > 59) AMinute = 0;

  //Adjust the second
  if (alarmNum == 1){
    ASecond += secAdjust;
    if (ASecond < 0) ASecond = 59;
    if (ASecond > 59) ASecond = 0;
  }
  
  //Reset the alarm settings
  if (alarmNum == 1) Clock.setA1Time(ADay, AHour, AMinute, ASecond, ABits, ADy, A12h, Apm);
  if (alarmNum == 2) Clock.setA2Time(ADay, AHour, AMinute, ABits, ADy, A12h, Apm);

 }

void changeAlarmMethod(int alarmNum, int dir) {

  uint8_t ADay1, AHour1, AMinute1, ASecond1, ADay2, AHour2, AMinute2, ABits = 0b0;
  bool ADy1, A12h1, Apm1, ADy2, A12h2, Apm2;

  int AlarmBits;
  
  if (alarmNum == 1){    
    Clock.getA1Time(ADay1, AHour1, AMinute1, ASecond1, ABits, ADy1, A12h1, Apm1); 
    ABits  = ABits & 0b1111;
    
    if (dir == 1) {
        if (ABits == ALRM1_MATCH_EVERY_SEC) AlarmBits |= ALRM1_MATCH_SEC;        
        else if (ABits == ALRM1_MATCH_SEC) AlarmBits |= ALRM1_MATCH_MIN_SEC;
        else if (ABits == ALRM1_MATCH_MIN_SEC) AlarmBits |= ALRM1_MATCH_HR_MIN_SEC;
        else if (ABits == ALRM1_MATCH_HR_MIN_SEC) AlarmBits |= ALRM1_MATCH_DY_HR_MIN_SEC;
        else if (ABits == ALRM1_MATCH_DY_HR_MIN_SEC) AlarmBits |= ALRM1_MATCH_EVERY_SEC;
        }
    else if (dir == 0) {
        if (ABits == ALRM1_MATCH_EVERY_SEC) AlarmBits |= ALRM1_MATCH_DY_HR_MIN_SEC;
        else if (ABits == ALRM1_MATCH_SEC) AlarmBits |= ALRM1_MATCH_EVERY_SEC;
        else if (ABits == ALRM1_MATCH_MIN_SEC) AlarmBits |= ALRM1_MATCH_SEC;
        else if (ABits == ALRM1_MATCH_HR_MIN_SEC) AlarmBits |= ALRM1_MATCH_MIN_SEC;
        else AlarmBits |= ALRM1_MATCH_HR_MIN_SEC;
    }
    else AlarmBits |= ABits;
    Clock.setA1Time(ADay1, AHour1, AMinute1, ASecond1, AlarmBits, ADy1, A12h1, Apm1);       
    
  } 
    else {

    Clock.getA2Time(ADay2, AHour2, AMinute2, ABits, ADy2, A12h2, Apm2); 
    ABits = ABits >> 4;

    if (dir == 1) {
      if (ABits == ALRM2_ONCE_PER_MIN) AlarmBits = ALRM2_MATCH_MIN;
      else if (ABits == ALRM2_MATCH_MIN) AlarmBits = ALRM2_MATCH_HR_MIN;
      else AlarmBits = ALRM2_ONCE_PER_MIN;
    }
    if (dir == 0) {
      if (ABits == ALRM2_ONCE_PER_MIN) AlarmBits = ALRM2_MATCH_HR_MIN;
      else if (ABits == ALRM2_MATCH_HR_MIN) AlarmBits = ALRM2_MATCH_MIN;
      else AlarmBits = ALRM2_ONCE_PER_MIN;
    }

    AlarmBits = AlarmBits << 4;
    Clock.setA2Time(ADay2, AHour2, AMinute2, AlarmBits, ADy2, A12h2, Apm2);
  }
}
void alarm(){

  uint8_t ADay, AHour, AMinute, ASecond, ABits;
  uint8_t A2Day, A2Hour, A2Minute, A2Bits;
  bool ADy, A12h, Apm;
  bool h12, PM;

  uint8_t alarmHour  = 0;
  uint8_t alarmMinute = 0;
  uint8_t alarm2Hour  = 0;
  uint8_t alarm2Minute = 0;
  uint8_t hour = 0;
  uint8_t minute = 0;
 

  String result1 = "Alarm1: ";
  if (Clock.checkAlarmEnabled(1)) Clock.getA1Time(ADay, AHour, AMinute, ASecond, ABits, ADy, A12h, Apm);
  
    String result2 = "Alarm2: ";

  if (Clock.checkAlarmEnabled(2)) Clock.getA2Time(A2Day, A2Hour, A2Minute, A2Bits, ADy, A12h, Apm);
  
  alarmHour = AHour;
  alarmMinute = AMinute;
  alarm2Hour = A2Hour;
  alarm2Minute = A2Minute;

  hour = Clock.getHour(h12, PM);
  minute = Clock.getMinute();

  if (lightPeriod == false){
  if(hour >= alarmHour && minute >= alarmMinute){
    //if (hour >= alarmHour || (hour == alarmHour && minute >= alarmMinute)) {
    PORTB |= (1 << PB5);
    lightPeriod = true;
    }
  }

  if (lightPeriod == true){
  if(hour >= alarm2Hour && minute >= alarm2Minute){
    //if (hour >= alarmHour || (hour == alarmHour && minute >= alarmMinute)) {
    PORTB &= ~(1 << PB5);
    lightPeriod = false;
    }
  }
}
