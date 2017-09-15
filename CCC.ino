#include <DS3231.h>
#include <TimerOne.h>
#include <dht.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>

// select the pins used on the LCD panel
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// define local variables
dht DHT;
#define DHT22_PIN A2
unsigned long millisCounter = 0;
volatile float LowestTemp = 100;
volatile float HighestTemp = -100;

volatile float LowestHumi = 100;
volatile float HighestHumi = 0;

volatile int lcd_key     = 0;
volatile int adc_key_in  = 0;

volatile int LightMagnitude = 0;
volatile int ClosingValue = EEPROM.read(4);//25;
volatile int LCDmenu = 0;
volatile int LCDmainPage = 0;
volatile int LCDsubPage = 0;
volatile int LCDDateEditMode = 0;
volatile int NumberOfLCDmainPages = 7;
volatile int NumberOfLCDsubPages = 14;

volatile unsigned int Every1secondsCounter = 0;
volatile unsigned int Every1secondsCounterOverflow = 400; //running a little faster than 1hz
volatile unsigned int Every60secondsCounter = 25000;
volatile unsigned int Every60secondsCounterOverflow = 30005; 
volatile unsigned int Every05secondsCounter = 0;
volatile unsigned int Every05secondsCounterOverflow = 250; //250 er ca 2Hz, 1000 er ca ½Hz
volatile unsigned int Every01secondsCounter = 0;
volatile unsigned int Every01secondsCounterOverflow = 50; 

volatile int GateOpeningDelayCounter = 60 * EEPROM.read(11);
volatile int GateOpeningDelay = EEPROM.read(11);
volatile int GateClosingDelayCounter = 60 * EEPROM.read(12);
volatile int GateClosingDelay = EEPROM.read(12);
volatile boolean TimeOperateGate = false;

volatile long LastOpeningMin = EEPROM.read(8);
volatile long LastOpeningHour = EEPROM.read(7);

volatile long LastClosingMin = EEPROM.read(10);
volatile long LastClosingHour = EEPROM.read(9);

volatile long SpecOpeningMin = EEPROM.read(17);
volatile long SpecOpeningHour = EEPROM.read(18);

volatile long SpecClosingMin = EEPROM.read(19);
volatile long SpecClosingHour = EEPROM.read(20);

volatile int LCDLightCounter = 60;

volatile long MinDayTime = EEPROM.read(5);
volatile long RemainingDayTime = 0;

volatile int FeedingTime = EEPROM.read(0);//15;
volatile int FeedingTimer = 0;

volatile double Latitude =57;
volatile long Longitude =-20;
volatile long SunriseMinutes;
volatile long SunsetMinutes;
volatile double debug;

volatile boolean GateOpen = false;
volatile boolean GateClosing = false;
volatile boolean GateOpening = false;
volatile int GateClosingCounter = 0;
volatile int GateOpeningCounter = 0;
volatile int GateClosingTime = EEPROM.read(1);//23;
volatile int GateOpeningTime = EEPROM.read(2);//35;

volatile int btnReleaseCountDown = 0;
volatile int btnReleaseValue = 100;
volatile boolean btnRelease = false;

String ErrorString1;
String ErrorString2;
String ErrorString3;
String ErrorString4;
String ErrorString5;

volatile int MaxOpeningDeviation = 60; // minutes
volatile int MaxClosingDeviation = 60; // minutes

volatile int Current;

volatile int WaterTime = EEPROM.read(13);
volatile int WaterCounter = 0;
volatile int WaterConsumption = 0;
volatile boolean AllowWater = true;

volatile int NormalPumpCurrent = EEPROM.read(14);
volatile int NormalGateCurrent = EEPROM.read(15);
volatile int NormalFeedCurrent = EEPROM.read(16);

volatile int NumberOfErrorStrings = 0;
volatile int ErrorStringCounter = 0;


// Init the DS3231 using the hardware interface
DS3231  rtc(SDA, SCL);
Time t;

#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

#define WarningLight 10
#define FeedMotor 2
#define WaterPump 13
#define WaterSensor 12
#define LightSensor A1
#define TempSensor A2
#define CurrentSensor A3

//Defining the LCD main pages
#define LCDfrontpage 0
#define LCDgateStatus 1
#define LCDopenCloseTime 2
#define LCDrunFeed 3
#define LCDSunUpDown 4
#define LCDwaterInfo 5
//following pages should be last
#define LCDchangeValues 6
#define LCDerror 7


//Defining the LCD sub pages
#define LCDclosingLightValue 0
#define LCDopenDelay 1
#define LCDcloseDelay 2
#define LCDopeningTime 3
#define LCDclosingTime 4
#define LCDfeedingTime 5
#define LCDminDayTime 6
#define LCDwater 7
#define LCDpumpCurrent 8
#define LCDgateCurrent 9
#define LCDfeedCurrent 10
#define LCDtimeOperatedGate 11
#define LCDspecOpenCloseTime 12
#define LCDDate 13

#define DateEditNone 0
#define DateEditDay 1
#define DateEditMonth 2
#define DateEditYear 3

#define LCDmainMenu 0
#define LCDsubMenu 1

void UpdateDisplay();
void ReadAndSaveTemperature();
void DecideIfGateShouldBeOperated();
void DecideStateOfDayTimeExtensionLight();
void OperateLCDlight();
void ControlFeeding();
void OperateGate();
void RunFeedMotor();
void CheckForButtonPres();
void DecideIfWatering();
void RunWaterPump();
void SunUpDown();

// read the buttons, this function is copyed from the internet
int read_LCD_buttons()
{
 adc_key_in = analogRead(0);      // read the value from the sensor 
 // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
 // we add approx 50 to those values and check to see if we are close
 if (adc_key_in > 1000) return btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result
 
 // For V1.0 comment the other threshold and use the one below:

 if (adc_key_in < 50)   return btnRIGHT;  
 if (adc_key_in < 195)  return btnUP; 
 if (adc_key_in < 380)  return btnDOWN; 
 if (adc_key_in < 555)  return btnLEFT; 
 if (adc_key_in < 790)  return btnSELECT;   

 return btnNONE;  // when all others fail, return this...
}

void setup()
{
 lcd.begin(16, 2);              // start the library
 lcd.setCursor(0,0);
 pinMode(0, OUTPUT); //Gate relay
 pinMode(1, OUTPUT); //Gate relay
 pinMode(2, OUTPUT); //Feed relay
 pinMode(3, OUTPUT); //Warning FET
 pinMode(WaterSensor, INPUT_PULLUP); //Water sensor
 pinMode(WaterPump, OUTPUT); //Water relay
 //digitalWrite(WaterSensor, HIGH);
 
 pinMode(10, OUTPUT); //LCD backlight
 pinMode(11, OUTPUT); //Light FET
 digitalWrite(0, LOW);
 digitalWrite(1, LOW);
 
 if (LastOpeningHour >= 25){
   LastOpeningHour = 6;
   EEPROM.write(7,6);
   }
 if (LastOpeningMin >= 61){
   LastOpeningMin = 0;
   EEPROM.write(8,0);
   }
 if (LastClosingHour >= 25){
   LastClosingHour = 18;
   EEPROM.write(9,18);
   }
 if (LastClosingMin >= 61){
   LastClosingMin = 0;
   EEPROM.write(10,0);
   }  
 if (GateOpeningDelay <= 0){
   GateOpeningDelay = 10;
   GateOpeningDelayCounter = 10;
   EEPROM.write(11,GateOpeningDelay);
   }
 if (GateClosingDelay <= 0){
   GateClosingDelay = 10;
   GateClosingDelayCounter = 10;
   EEPROM.write(12,GateClosingDelay);
   }

if ((analogRead(1)/10) > ClosingValue){
  GateOpen = true;
}
else{
  GateOpen = false;
}  
  // Initialize the rtc object
  rtc.begin();
  
   //Timer1.initialize(1000000); 
   Timer1.attachInterrupt( timerIsr ); // attach the service routine here
}

/// --------------------------
/// Custom ISR Timer Routine
/// --------------------------
void timerIsr()
{
  
  if(btnReleaseCountDown > 0){
    btnReleaseCountDown = btnReleaseCountDown - 1;
  }

  if(Every01secondsCounter >= Every01secondsCounterOverflow){
    AdjustTime();
    Every01secondsCounter = 0;
    CheckForButtonPres();
    
  }
  Every01secondsCounter++;
  
  if(Every05secondsCounter >= Every05secondsCounterOverflow){
    SunUpDown();
    Every05secondsCounter = 0;
    UpdateDisplay();
    ControlFeeding();
  }
  Every05secondsCounter++;

  if(Every60secondsCounter >= Every60secondsCounterOverflow){
    Every60secondsCounter = 0;
    ReadAndSaveTemperature();
    DecideIfWatering();
    
  }
  Every60secondsCounter++;

  if(Every1secondsCounter >= Every1secondsCounterOverflow){ 
    Every1secondsCounter = 0;
    
    t = rtc.getTime();
    Current = max((analogRead(3)-510),0);

    ErrorStringCounter++;
    if (ErrorStringCounter >= NumberOfErrorStrings + 1){
      ErrorStringCounter = 1;
    }
    DecideIfGateShouldBeOperated();
    OperateLCDlight();
    OperateGate();
    RunFeedMotor();
    RunWaterPump();
    DecideStateOfDayTimeExtensionLight();
    btnReleaseCountDown = true; 
  }
  Every1secondsCounter++;
  
}
void WriteErrorString(String Error){
  if (Error == ""){
    analogWrite(WarningLight,0);
    NumberOfLCDmainPages = 7;
    AllowWater = true;
    //TimeOperateGate = false;
    
    ErrorString1 = "";
    ErrorString2 = "";
    ErrorString3 = "";
    ErrorString4 = "";
    ErrorString5 = "";
    NumberOfErrorStrings = 0;
  }
  else{  
    analogWrite(WarningLight,250);
    NumberOfLCDmainPages = 8;
    LCDmainPage = LCDerror;
    if (ErrorString1 == ""){
      ErrorString1 = Error;
      NumberOfErrorStrings = 1;
    }
    else if(ErrorString2 == ""){
      ErrorString2 = Error;  
      NumberOfErrorStrings = 2; 
    }
    else if(ErrorString3 == ""){
      ErrorString3 = Error; 
      NumberOfErrorStrings = 3;  
    }
    else if(ErrorString4 == ""){
      ErrorString4 = Error; 
      NumberOfErrorStrings = 4;  
    }
    else if(ErrorString5 == ""){
      ErrorString5 = Error;  
      NumberOfErrorStrings = 5; 
    }
  }
  
  
}
void AdjustTime(){
  if (lcd_key == btnUP){
      if ((LCDmainPage == LCDfrontpage) && (LCDmenu == LCDmainMenu)){
            t = rtc.getTime(); 

             if (t.min >=59){
              if (t.hour >= 23){
                rtc.setTime(0,0,0);
              }
              else{
                rtc.setTime(t.hour+1,0,0);
              }
             }
             else{
              rtc.setTime(t.hour,t.min + 1,0);
             }          
             
        }
      }
    if ((lcd_key == btnDOWN) && (LCDmenu == LCDmainMenu)){
      if (LCDmainPage == LCDfrontpage){
         t = rtc.getTime(); 
           
             if (t.min <= 0){
              if (t.hour <= 0){
                rtc.setTime(23,59,0);
              }
              else{
                rtc.setTime(t.hour - 1,0,0);
              }
             }
             else{
              rtc.setTime(t.hour,t.min - 1,0);
             } 
        }
    } 
}
void ReadAndSaveTemperature(){
  DHT.read22(DHT22_PIN);
   if (DHT.temperature > HighestTemp){
     HighestTemp = DHT.temperature;
     }
   if (DHT.temperature < LowestTemp){
     LowestTemp = DHT.temperature;
     }    
   if (DHT.humidity > HighestHumi){
     HighestHumi = DHT.humidity;
     }
   if (DHT.humidity < LowestHumi){
     LowestHumi = DHT.humidity;
     } 
}

void UpdateDisplay(){
  lcd.clear();
if (LCDmenu == LCDmainMenu){
switch (LCDmainPage){
   case LCDfrontpage:
   {
    
     lcd.setCursor(0,0);
     lcd.print("CHICKOMATIC v2.0");
     lcd.setCursor(0,1);
     
        lcd.print(rtc.getTimeStr());
     
    
        
     
     lcd.print(" lux ");
     lcd.print(LightMagnitude);
     lcd.print("% ");




     
     break;
     }
     
     case LCDgateStatus:
     {
     lcd.setCursor(0,0);
     lcd.print("Gate is ");   
     if (GateOpeningCounter >= 1){
       lcd.print("opening");
       lcd.setCursor(0,1);
       lcd.print(GateOpeningCounter);
       lcd.print("s        ");
       lcd.print("I=");
       lcd.print(Current); 
       }
     else if (GateClosingCounter >= 1){
       lcd.print("closing");
       lcd.setCursor(0,1);
       lcd.print(GateClosingCounter);
       lcd.print("s        ");
       lcd.print("I=");
       lcd.print(Current); 
       }
     else{
         if (GateOpen == true){
         lcd.print("open   ");
         }
         if (GateOpen == false){
         lcd.print("closed ");
         }
         lcd.setCursor(0,1);
         lcd.print("SELECT to operat");
       }
     
     break;
     }
     
     case LCDopenCloseTime:
     {
     lcd.setCursor(0,0);
     lcd.print("Last open  ");
     lcd.print(LastOpeningHour);
     lcd.print(":");
     lcd.print(LastOpeningMin);
     lcd.print("  ");

     lcd.setCursor(0,1);
     lcd.print("Last close ");
     lcd.print(LastClosingHour);
     lcd.print(":");
     lcd.print(LastClosingMin);
     lcd.print("  ");
     
     break;
     }
     
     case LCDrunFeed:
     {   
     lcd.setCursor(0,0);
     lcd.print("Run feed once");
          
     lcd.setCursor(0,1);
     lcd.print("                ");
     lcd.setCursor(0,1);
     lcd.print("");
     lcd.print(FeedingTimer);
     lcd.print(" s       ");
     lcd.print("I=");
     lcd.print(Current);     
     break;
     }
     
     case LCDSunUpDown:
     {       
     lcd.setCursor(0,0);
     lcd.print("Sunrise: ");
     lcd.print(int(SunriseMinutes));
            
     lcd.setCursor(0,1);
     lcd.print("Sundown: ");
     lcd.print(int(SunsetMinutes));
     
     break;
     }

     case LCDwaterInfo:
     {    
     lcd.setCursor(0,0);
     lcd.print("Water used ");
     lcd.print(WaterConsumption); 
     lcd.setCursor(0,1);
     if (WaterCounter >= 1){
      lcd.print(WaterCounter);   
      lcd.print("s       ");  
     }
     else{
      lcd.print(digitalRead(WaterSensor));
      lcd.print(" ");
      lcd.print("SEL run  ");
     }
     lcd.print("I=");
     lcd.print(Current);  
     break;
     }
     
     case LCDchangeValues:
     {    
     lcd.setCursor(0,0);
     lcd.print("Change values");   
     lcd.setCursor(0,1);
     lcd.print("Press SELECT");     
     break;
     }

     case LCDerror:
     {    
     lcd.setCursor(0,0);
     lcd.print("    ERROR!!!");   
     lcd.setCursor(0,1);
     switch (ErrorStringCounter){
      case 1:{
        lcd.print(ErrorString1);
      }
      case 2:{
        lcd.print(ErrorString2);
      }
      case 3:{
        lcd.print(ErrorString3);
      }
      case 4:{
        lcd.print(ErrorString4);
      }
      case 5:{
        lcd.print(ErrorString5);
      }
      
     }
          
     break;
     }
   }
}
if (LCDmenu == LCDsubMenu){
switch (LCDsubPage){
   case LCDclosingLightValue:
   {     
     lcd.setCursor(0,0);
     lcd.print("Close light mag");
          
     lcd.setCursor(0,1);
     lcd.print("Value: ");
     lcd.print(ClosingValue);
     lcd.print(" ");
     break;
     }
     
   case LCDopenDelay:
   {     
     lcd.setCursor(0,0);
     lcd.print("Opening delay ");
     lcd.setCursor(0,1);
     lcd.print("Value: ");
     lcd.print(GateOpeningDelay);
     lcd.print("min ");
     break;
     }
     
   case LCDcloseDelay:
   {     
     lcd.setCursor(0,0);
     lcd.print("Closing delay ");
     lcd.setCursor(0,1);
     lcd.print("Value: ");
     lcd.print(GateClosingDelay);
     lcd.print("min ");
     break;
     }
     
   case LCDopeningTime:
   {     
     lcd.setCursor(0,0);
     lcd.print("Opening time ");
     lcd.setCursor(0,1);
     lcd.print("Value: ");
     lcd.print(GateOpeningTime);
     lcd.print("s ");
     
     break;
     }
   
   case LCDclosingTime:
   {     
    lcd.setCursor(0,0);
     lcd.print("Closing time ");
     lcd.setCursor(0,1);
     lcd.print("Value: ");
     lcd.print(GateClosingTime);
     lcd.print("s ");
     break;
     }
     
   case LCDfeedingTime:
   {     
    lcd.setCursor(0,0);
     lcd.print("Feeding time");
          
     lcd.setCursor(0,1);
     lcd.print("Value: ");
     lcd.print(FeedingTime);
     lcd.print("s ");
     break;
     }
     
   case LCDminDayTime:
   {     
    lcd.setCursor(0,0);
     lcd.print("Min day time ");
     lcd.print(MinDayTime);
     lcd.print("h");
     lcd.setCursor(0,1);
     lcd.print("Remain ");
     if ((RemainingDayTime/3600) <= 9){
      lcd.print("0");
     }
     lcd.print(RemainingDayTime/3600);
     lcd.print(":");
     if ((RemainingDayTime/60 - int(RemainingDayTime/3600)*60) <= 9){
      lcd.print("0");
     }
     lcd.print(RemainingDayTime/60 - int(RemainingDayTime/3600)*60);
     lcd.print(":");
     if ((RemainingDayTime - long(RemainingDayTime/60)*60) <= 9){
      lcd.print("0");
     }
     lcd.print(RemainingDayTime - long(RemainingDayTime/60)*60);
     lcd.print("  ");
     break;
     } 
   case LCDwater:
   {     
    lcd.setCursor(0,0);
     lcd.print("Water time");
          
     lcd.setCursor(0,1);
     lcd.print("Value: ");
     lcd.print(WaterTime);
     lcd.print("s");
     break;
     } 
   case LCDpumpCurrent:
   {     
    lcd.setCursor(0,0);
     lcd.print("Normal pump I ");
     
     lcd.setCursor(0,1);
     lcd.print("Value: ");
     lcd.print(NormalPumpCurrent);
     lcd.print("");
     break;
     }
   case LCDgateCurrent:
   {     
    lcd.setCursor(0,0);
     lcd.print("Normal gate I ");
     
     lcd.setCursor(0,1);
     lcd.print("Value: ");
     lcd.print(NormalGateCurrent);
     lcd.print("");
     break;
     }
   case LCDfeedCurrent:
   {     
    lcd.setCursor(0,0);
     lcd.print("Normal feed I ");
     
     lcd.setCursor(0,1);
     lcd.print("Value: ");
     lcd.print(NormalFeedCurrent);
     lcd.print("");
     break;
     } 
   case LCDtimeOperatedGate:
   {     
    lcd.setCursor(0,0);
     lcd.print("Gate operated by ");
     
     lcd.setCursor(0,1);
     if (TimeOperateGate == true){
       lcd.print("time");
     }
     else{
       lcd.print("light sensor");
     } 
     break;
     } 
   case LCDspecOpenCloseTime:
   {     
    lcd.setCursor(0,0);
     lcd.print("Open ");
     lcd.print(SpecOpeningHour);
     lcd.print(":");
     lcd.print(SpecOpeningMin);
     
     lcd.setCursor(0,1);
     lcd.print("Close ");
     lcd.print(SpecClosingHour);
     lcd.print(":");
     lcd.print(SpecClosingMin);
     break;
     } 
     case LCDDate:
     {
      if (LCDDateEditMode == DateEditNone){
           lcd.setCursor(0,0);
           lcd.print("Date ");
           lcd.setCursor(0,1);
           lcd.print(rtc.getDateStr());
      }
      if (LCDDateEditMode == DateEditDay){
           lcd.setCursor(1,0);
           lcd.print("*");
           lcd.setCursor(0,1);
           lcd.print(rtc.getDateStr());
      }
      if (LCDDateEditMode == DateEditMonth){
           lcd.setCursor(3,0);
           lcd.print("*");
           lcd.setCursor(0,1);
           lcd.print(rtc.getDateStr());
      }
      if (LCDDateEditMode == DateEditYear){
           lcd.setCursor(6,0);
           lcd.print("*");
           lcd.setCursor(0,1);
           lcd.print(rtc.getDateStr());
      }
     break;
     } 
  }
}
}
void DecideIfWatering(){
  if ((digitalRead(WaterSensor) == LOW) && (AllowWater == true) && (WaterTime >= 1)){
     WaterCounter = WaterTime;
     WaterConsumption++;
  }
  
}

void DecideIfGateShouldBeOperated(){
  LightMagnitude = 100-(analogRead(1))/10;   
  if (TimeOperateGate == false){
    //Check possible sensor failure if not last values are initial values
    if (LastOpeningHour == 6 && LastOpeningMin == 0){ 
    }
    else{
      if ( (GateOpen == false) && ((long(t.hour*60) + long(t.min)) == (long(LastOpeningHour*60) + long(LastOpeningMin)) + MaxOpeningDeviation)){
        WriteErrorString("Gate did'nt open");
        TimeOperateGate = true;
        LCDmainPage = LCDerror;
      }
      if ( (GateOpen == true) && ((long(t.hour*60) + long(t.min)) == (long(LastOpeningHour*60) + long(LastOpeningMin)) - MaxOpeningDeviation)){
        WriteErrorString("Early gate open");
        TimeOperateGate = true;
        LCDmainPage = LCDerror;
      }
    }
    if (LastClosingHour == 18 && LastClosingMin == 0){ 
    }
    else{
      if ( (GateOpen == false) && ((long(t.hour*60) + long(t.min)) == (long(LastClosingHour*60) + long(LastClosingMin)) - MaxClosingDeviation)){
        WriteErrorString("Early gate close");
        TimeOperateGate = true;
        LCDmainPage = LCDerror;
      }
    
    if ( (GateOpen == true) && ((long(t.hour*60) + long(t.min)) == (long(LastClosingHour*60) + long(LastClosingMin)) + MaxClosingDeviation)){
        WriteErrorString("Gate didnt close");
        TimeOperateGate = true;
        LCDmainPage = LCDerror;
      }
    }
    
    
    if ((LightMagnitude > ClosingValue) && 
       (GateOpen == false)){
           GateOpeningDelayCounter = GateOpeningDelayCounter - 1;
           if (GateOpeningDelayCounter == 0){
             
             GateOpeningCounter = GateOpeningTime;

             //Only saving opening hour as specified if the deviation is not too large
             if (abs(long(LastOpeningHour*60) + long(LastOpeningMin) - long(t.hour*60) - long(t.min)) < MaxOpeningDeviation){
                SpecOpeningMin = t.min;
                EEPROM.write(17,SpecOpeningMin);
                SpecOpeningHour = t.hour;
                EEPROM.write(18,SpecOpeningHour);
             }
             LastOpeningMin = t.min;
             EEPROM.write(8,LastOpeningMin);
             LastOpeningHour = t.hour;   
             EEPROM.write(7,LastOpeningHour);
             
             }  
        }      
        else{
          GateOpeningDelayCounter = GateOpeningDelay * 60;
        }
   if ((LightMagnitude < ClosingValue) &&
       (GateOpen == true)){
         GateClosingDelayCounter = GateClosingDelayCounter - 1;
         if (GateClosingDelayCounter == 0){
           GateClosingCounter = GateClosingTime;
           if (abs(LastClosingHour*60 + LastClosingMin - t.hour*60 - t.min) < MaxClosingDeviation){
               SpecClosingMin = t.min;
                EEPROM.write(19,SpecClosingMin);
                SpecClosingHour = t.hour;
                EEPROM.write(20,SpecClosingHour);
             }
           LastClosingMin = t.min;
           EEPROM.write(10,LastClosingMin);
           LastClosingHour = t.hour;
           EEPROM.write(9,LastClosingHour);
           }
         }
     else{
       GateClosingDelayCounter = GateClosingDelay * 60;
       }  
  }
  else{
    if ((t.hour * 60 + t.min >= SpecOpeningHour * 60 + SpecOpeningMin) && (t.hour * 60 + t.min <= SpecClosingHour *60 + SpecClosingMin)){
      //Here the gate should be open
      if (GateOpen == false){
        GateOpeningCounter = GateOpeningTime;
        GateOpen = true;
        LastOpeningMin = t.min;
        EEPROM.write(8,LastOpeningMin);
        LastOpeningHour = t.hour;   
        EEPROM.write(7,LastOpeningHour);
      }
    }
    else{
      if (GateOpen == true){
        GateClosingCounter = GateClosingTime;
        GateOpen = false;
        LastClosingMin = t.min;
        EEPROM.write(10,LastClosingMin);
        LastClosingHour = t.hour;
        EEPROM.write(9,LastClosingHour);
      }
    }
  }
   
}

void DecideStateOfDayTimeExtensionLight(){
  //Deciding the state of the daytime extension light.
  t = rtc.getTime();
 if (t.hour > LastOpeningHour){
   RemainingDayTime = max(0,(MinDayTime + LastOpeningHour - t.hour) * 3600 + (LastOpeningMin - t.min) * 60 - t.sec); 
   }   
 else{
   RemainingDayTime = max(0,(MinDayTime + LastOpeningHour - t.hour - 24) * 3600 + (LastOpeningMin - t.min) * 60 - t.sec);
   }


 if (t.hour * 60 + t.min >= SpecClosingHour * 60 + SpecClosingMin - 2 * 60){  
  if (RemainingDayTime >= 1){
     //if (LightMagnitude <= 10){
       if (RemainingDayTime <= 2000){
         analogWrite(11, RemainingDayTime / 8);
       }
       else{
         analogWrite(11,250);
       }
     //}
     //else {
     //    analogWrite(11,200-(LightMagnitude * 4));
     //}
   }
   else{
     analogWrite(11,0);
   }
 }
 else{
   analogWrite(11,0);  
 }
  
}


void OperateLCDlight(){
  if (LCDLightCounter >= 10){
       LCDLightCounter = LCDLightCounter - 1;
       analogWrite(10,100);
     }
   else if(LCDLightCounter >= 1){
       LCDLightCounter = LCDLightCounter - 1;
       analogWrite(10,LCDLightCounter*10);
     }
   else{
     analogWrite(10,0);
     }
}

void ControlFeeding(){
  t = rtc.getTime();
  // Starting feeding sequence one hour after opening
   if (t.hour == LastOpeningHour + 1){
     if (t.min == LastOpeningMin){
       if (t.sec == 1){
         FeedingTimer = FeedingTime;
         }
       }
     }
     
   // Starting feeding sequence one hours before closing
   if (t.hour == LastClosingHour - 1){
     if (t.min == LastClosingMin){
       if (t.sec == 1){
         FeedingTimer = FeedingTime;
         }
       }
     }
}

void OperateGate(){
  //Controlling the voltage to the gatemotor if the opening/closing counter is positive
   if (GateOpeningCounter >= 1){
     digitalWrite(0, LOW);
     digitalWrite(1, HIGH);
     GateOpeningCounter = GateOpeningCounter - 1;
     if (GateOpeningCounter == GateOpeningTime-5){
        if (Current <= NormalGateCurrent / 2){
       WriteErrorString("Opening I to low");
       }
     }
     if (GateOpeningCounter == 0){
       GateOpen = true;
       digitalWrite(0, LOW);
       digitalWrite(1, LOW);
       GateOpeningDelayCounter = GateOpeningDelay;
       }
       SunUpDown();
     }
     
   if (GateClosingCounter >= 1){
     digitalWrite(0, HIGH);
     digitalWrite(1, LOW);
     GateClosingCounter = GateClosingCounter - 1;
     if (GateClosingCounter == GateClosingTime-5){
       if (Current <= NormalGateCurrent / 2){
       WriteErrorString("Closing I to low");
       }
     }
     
     if (GateClosingCounter == 0){
       GateOpen = false;
       digitalWrite(0, LOW);
       digitalWrite(1, LOW);
       GateClosingDelayCounter = GateClosingDelay;
       }
     }
}

void RunWaterPump(){
  if (WaterCounter >= 1){
     WaterCounter = WaterCounter -1;
     digitalWrite(WaterPump, HIGH);

     //Checking if the pump is consuming current
     if (WaterCounter == 2){
       if (Current <= NormalPumpCurrent / 2){
          WriteErrorString ("I pump too low");
       }
     }
     
     //Checking if the sensor says that water is there now.
     if (WaterCounter == 1){   
        if (digitalRead(WaterSensor) == LOW){
            WriteErrorString ("Pump/sens error");
            AllowWater = false;
        }
     }
     }
   else{
     digitalWrite(WaterPump, LOW);  
     }

   //Checking if water has been added the last 24h
   if (t.hour == 20 && t.min == 0 && t.sec == 0){
     if (WaterConsumption == 0){
      WriteErrorString("No water used");
     }
   }
   if (t.hour == 0 && t.min == 0 && t.sec == 0){
     WaterConsumption = 0;
   }
}

void RunFeedMotor(){
  if (FeedingTimer >= 1){
     FeedingTimer = FeedingTimer -1;
     digitalWrite(FeedMotor, HIGH);
     if (FeedingTimer == 3){
      if (Current <= NormalFeedCurrent / 2){
        WriteErrorString("I feed to low");
      }
     }
     }
   else{
     digitalWrite(FeedMotor, LOW);  
     }
}

void CheckForButtonPres(){
lcd_key = read_LCD_buttons();  // read the buttons

if ((btnReleaseCountDown == 0) && (btnRelease = true)){
  if((lcd_key == btnRIGHT) && (LCDmenu == LCDmainMenu)){
    LCDmainPage++;
     if (LCDmainPage >= NumberOfLCDmainPages){
       LCDmainPage = 0;
       }
  }
  else if((lcd_key == btnLEFT) && (LCDmenu == LCDmainMenu)){
    LCDmainPage = LCDmainPage - 1;
     if (LCDmainPage < 0){
       LCDmainPage = NumberOfLCDmainPages - 1;
       }
  }
  else if((lcd_key == btnRIGHT) && (LCDmenu == LCDsubMenu)){
    LCDsubPage++;
     if (LCDsubPage >= NumberOfLCDsubPages){
       LCDsubPage = 0;
       }
  }
  else if((lcd_key == btnLEFT) && (LCDmenu == LCDsubMenu)){
    LCDsubPage = LCDsubPage - 1;
     if (LCDsubPage < 0){
       LCDsubPage = NumberOfLCDsubPages - 1;
       }
  }
else if((lcd_key == btnSELECT) && (LCDmenu == LCDsubMenu) && (LCDsubPage != LCDDate)){
    LCDmenu = LCDmainMenu;
  }
  else if((lcd_key == btnSELECT) && (LCDmenu == LCDmainMenu) && (LCDmainPage == LCDchangeValues)){
    LCDmenu = LCDsubMenu;
  }
  else if((lcd_key == btnSELECT) && (LCDmenu == LCDmainMenu) && (LCDmainPage == LCDgateStatus)){
   if (GateOpeningCounter <= 0 && GateClosingCounter <= 0){ 
    if (GateOpen == false){
      GateOpeningCounter = GateOpeningTime;
    }
    else{
      GateClosingCounter = GateClosingTime;
    }
   }
  }
  else if((lcd_key == btnSELECT) && (LCDmenu == LCDmainMenu) && (LCDmainPage == LCDrunFeed)){
    if(FeedingTimer <= 0){
      FeedingTimer = FeedingTime;
    } 
  }
  else if((lcd_key == btnSELECT) && (LCDmenu == LCDmainMenu) && (LCDmainPage == LCDwaterInfo)){
      WaterCounter = WaterTime;
  }
  else if((lcd_key == btnSELECT) && (LCDmenu == LCDmainMenu) && (LCDmainPage == LCDerror)){
    WriteErrorString("");
  }
  else if((lcd_key == btnSELECT) && (LCDmenu == LCDmainMenu) && (LCDmainPage == LCDSunUpDown)){
    LowestTemp = 100;
    HighestTemp = -100;

    LowestHumi = 100;
    HighestHumi = 0;
  }
  else if((lcd_key == btnSELECT) && (LCDmenu == LCDmainMenu) && (LCDmainPage == LCDopenCloseTime)){
    LastOpeningMin = 0;
    EEPROM.write(8,LastOpeningMin);
    LastOpeningHour = 6;   
    EEPROM.write(7,LastOpeningHour);

    LastClosingMin = 0;
    EEPROM.write(10,LastClosingMin);
    LastClosingHour = 18;
    EEPROM.write(9,LastClosingHour);
  }
  else if((lcd_key == btnSELECT) && (LCDmenu == LCDsubMenu) && (LCDsubPage == LCDDate)){
    LCDDateEditMode++;
    if (LCDDateEditMode > 3){   
       LCDDateEditMode=0;
    }
  }
  else if((lcd_key == btnUP) && (LCDmenu == LCDsubMenu)){
    switch (LCDsubPage){
      case LCDclosingLightValue:{
            if (ClosingValue <= 88){   
            ClosingValue++;
            EEPROM.write(4,ClosingValue);
            }
            else{
              ClosingValue = 1;
              EEPROM.write(4,ClosingValue);
              }
            break;
      }
      case LCDopenDelay:{
           if (GateOpeningDelay <=255){
           GateOpeningDelay++;
           EEPROM.write(11,GateOpeningDelay);
           GateOpeningDelayCounter = GateOpeningDelay * 60;
           }
           else{
               GateOpeningDelay = 1;
               EEPROM.write(11,GateOpeningDelay);
             } 
           break;   
      }
      case LCDcloseDelay:{
           if (GateClosingDelay <=255){
           GateClosingDelay++;
           EEPROM.write(12,GateClosingDelay);
           GateClosingDelayCounter = GateClosingDelay * 60;
           }
           else{
               GateClosingDelay = 1;
               EEPROM.write(11,GateClosingDelay);
             }
           break;       
      }
      case LCDopeningTime:{
           if (GateOpeningTime <= 254){
           GateOpeningTime++;
           EEPROM.write(2,GateOpeningTime);
           }
           else{
             GateOpeningTime = 1;
             EEPROM.write(2,GateOpeningTime);
             }
           break;
      }
      case LCDclosingTime:{
           if (GateClosingTime <= 254){
             GateClosingTime++;
             EEPROM.write(1,GateClosingTime);
           }
           else{
             GateClosingTime = 1;
             EEPROM.write(1,GateClosingTime);
             }
           break;
      }
      case LCDfeedingTime:{
           if (FeedingTime<= 254){
             FeedingTime++;
             EEPROM.write(0,FeedingTime);//
           }
           else{
             FeedingTime = 1;
             EEPROM.write(0,FeedingTime);//
             }
           break;
      }
      case LCDminDayTime:{
            if (MinDayTime <= 23){
             MinDayTime++;
             EEPROM.write(5,MinDayTime);
             }
           else{
             MinDayTime = 0;
             EEPROM.write(5,MinDayTime);
             }
           break;
      }
      case LCDwater:{
          if (WaterTime <= 239){
             WaterTime++;
             EEPROM.write(13,WaterTime);
             }
           else{
             WaterTime = 0;
             EEPROM.write(13,WaterTime);
             }
         break;
      }
      case LCDpumpCurrent:{
          if (NormalPumpCurrent <= 239){
             NormalPumpCurrent++;
             EEPROM.write(14,NormalPumpCurrent);
             }
           else{
             NormalPumpCurrent = 0;
             EEPROM.write(14,NormalPumpCurrent);
             }
         break;
      }
      case LCDgateCurrent:{
          if (NormalGateCurrent <= 239){
             NormalGateCurrent++;
             EEPROM.write(15,NormalGateCurrent);
             }
           else{
             NormalGateCurrent = 0;
             EEPROM.write(15,NormalGateCurrent);
             }
         break;
      }
      case LCDfeedCurrent:{
          if (NormalFeedCurrent <= 239){
             NormalFeedCurrent++;
             EEPROM.write(16,NormalFeedCurrent);
             }
           else{
             NormalFeedCurrent = 0;
             EEPROM.write(16,NormalFeedCurrent);
             }
         break;
      }
      case LCDtimeOperatedGate:{
          TimeOperateGate = true;
         break;
      }
      case LCDDate:{
        t = rtc.getTime();
        if (LCDDateEditMode == DateEditDay){
          rtc.setDate(t.date+1, t.mon, t.year);
        }
        if (LCDDateEditMode == DateEditMonth){
          rtc.setDate(t.date, t.mon+1, t.year);
        }
        if (LCDDateEditMode == DateEditYear){
          rtc.setDate(t.date, t.mon, t.year+1);
        }
         break;
      }
    }
  }  
  else if((lcd_key == btnDOWN) && (LCDmenu == LCDsubMenu)){
    switch (LCDsubPage){
      case LCDclosingLightValue:{
            if (ClosingValue >= 2){
           ClosingValue = ClosingValue - 1;
           EEPROM.write(4,ClosingValue);
         }
         break;
      }
      case LCDopenDelay:{
           if (GateOpeningDelay >= 2){
         GateOpeningDelay = GateOpeningDelay - 1;
         EEPROM.write(11,GateOpeningDelay);
         GateOpeningDelayCounter = GateOpeningDelay * 60;
         }
         break;
      }
      case LCDcloseDelay:{
           if (GateClosingDelay >= 2){
         GateClosingDelay = GateClosingDelay - 1;
         EEPROM.write(12,GateClosingDelay);
         GateClosingDelayCounter = GateClosingDelay * 60;
         }
         break;
      }
      case LCDopeningTime:{
           if (GateOpeningTime >= 2){
           GateOpeningTime = GateOpeningTime - 1;
           EEPROM.write(2,GateOpeningTime);
         }
         break;
      }
      case LCDclosingTime:{
            if (GateClosingTime >= 2){
           GateClosingTime = GateClosingTime - 1;
           EEPROM.write(1,GateClosingTime);
         }
         break;
      }
      case LCDfeedingTime:{
            if (FeedingTime >= 2){
           FeedingTime = FeedingTime - 1;
           EEPROM.write(0,FeedingTime);//
         }
         break;
      }
      case LCDminDayTime:{
            if (MinDayTime >= 1){
         MinDayTime = MinDayTime - 1;
         EEPROM.write(5,MinDayTime);
         }
         else{
           MinDayTime = 24;
           EEPROM.write(5,MinDayTime);
           }
         break;
      }
      case LCDwater:{
            if (WaterTime >= 1){
         WaterTime = WaterTime - 1;
         EEPROM.write(13,WaterTime);
         }
         else{
           WaterTime = 240;
           EEPROM.write(13,WaterTime);
           }
         break;
      }
      case LCDpumpCurrent:{
            if (NormalPumpCurrent >= 1){
         NormalPumpCurrent = NormalPumpCurrent - 1;
         EEPROM.write(14,NormalPumpCurrent);
         }
         else{
           NormalPumpCurrent = 240;
           EEPROM.write(14,NormalPumpCurrent);
           }
         break;
      }
      case LCDgateCurrent:{
            if (NormalGateCurrent >= 1){
         NormalGateCurrent = NormalGateCurrent - 1;
         EEPROM.write(15,NormalGateCurrent);
         }
         else{
           NormalGateCurrent = 240;
           EEPROM.write(15,NormalGateCurrent);
           }
         break;
      }
      case LCDfeedCurrent:{
            if (NormalFeedCurrent >= 1){
         NormalFeedCurrent = NormalFeedCurrent - 1;
         EEPROM.write(16,NormalFeedCurrent);
         }
         else{
           NormalFeedCurrent = 240;
           EEPROM.write(16,NormalFeedCurrent);
           }
         break;
      }
      case LCDtimeOperatedGate:{
          TimeOperateGate = false;
         break;
      }
      case LCDDate:{
        t = rtc.getTime(); 
        if (LCDDateEditMode == DateEditDay){
          rtc.setDate(t.date-1, t.mon, t.year);
        }
        if (LCDDateEditMode == DateEditMonth){
          rtc.setDate(t.date, t.mon-1, t.year);
        }
        if (LCDDateEditMode == DateEditYear){
          rtc.setDate(t.date, t.mon, t.year-1);
        }
         break;
      }
    }
    btnRelease = false;
  }

  if ((lcd_key == btnUP) || (lcd_key == btnDOWN) || (lcd_key == btnLEFT) || (lcd_key == btnRIGHT) || (lcd_key == btnSELECT)){
    LCDLightCounter = 600; 
    btnReleaseCountDown = btnReleaseValue;
    btnRelease = true;
    lcd.clear();
    UpdateDisplay();
  }
}
}

void SunUpDown(){
 String Date;
 long DayOfYear;
 long int Month;
 long int DayOfMonth;
 
Date =rtc.getDateStr();

 Month = Date.substring(3,5).toInt();
DayOfMonth= Date.substring(0,2).toInt();

 DayOfYear=0;
 if (Month > 1){  
   DayOfYear = DayOfYear+31;
 }       
if (Month > 2){  
   DayOfYear = DayOfYear+28;
 }     
if (Month > 3){  
   DayOfYear = DayOfYear+31;
 }     
if (Month > 4){  
   DayOfYear = DayOfYear+30;
 }     
 if (Month > 5){  
   DayOfYear = DayOfYear+31;
 }     
 if (Month > 6){  
   DayOfYear = DayOfYear+30;
 }     
 if (Month > 7){  
   DayOfYear = DayOfYear+31;
 }     
 if (Month > 8){  
   DayOfYear = DayOfYear+31;
 }     
 if (Month > 9){  
   DayOfYear = DayOfYear+30;
 }     
 if (Month > 10){  
   DayOfYear = DayOfYear+31;
 }     
 if (Month > 11){  
   DayOfYear = DayOfYear+30;
 }   
  
 DayOfYear = DayOfYear+DayOfMonth;
  debug=DayOfYear;
  //Radius of the earth (km)
  double R=6378;

  //Radians between the xy-plane and the ecliptic plane
  double epsilon=23.45/180*3.1428;

  //Convert observer's latitude to radians
  double L=Latitude/180*3.1428;

  // Calculate offset of sunrise based on longitude (min)
  // If Long is negative, then the mod represents degrees West of
  // a standard time meridian, so timing of sunrise and sunset should
  // be made later.
  long timezone = -4*(Longitude);

  // The earth's mean distance from the sun (km)
 double rdist = 149598000;

 double theta = 2*3.1428/365.25*(DayOfYear-80);

double zs = rdist*sin(theta)*sin(epsilon);
 double rp = sqrt(rdist*rdist-zs*zs);

  double t0 = 1440/(2*3.1428)*acos((R-zs*sin(L))/(rp*cos(L)));

  //a kludge adjustment for the radius of the sun
 double that = t0+5; 

  // Adjust "noon" for the fact that the earth's orbit is not circular:
 double n = 720-10*sin(4*3.1428*(DayOfYear-80)/365.25)+8*sin(2*3.1428*DayOfYear/365.25);

 // now sunrise and sunset are:
SunriseMinutes = (n-that+timezone);
SunsetMinutes = (n+that+timezone);
debug=SunsetMinutes;
  
}

void loop(){ 
   if (millis()/1000 > millisCounter){  
     
   
   
   millisCounter = millis()/1000;
   }       
}
