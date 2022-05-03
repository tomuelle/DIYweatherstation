#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include "DS1337.h"  // RTC
#include "SeeeduinoStalker.h"
#include <Wire.h>
#include <SD.h>
#include <SparkFun_MS5803_I2C.h>
#define sensorPower 9

// ################################## VARIABLE CONFIGURATION #####################
// Name of SD card save file
String fileName = "DataP1.CSV";

// Sensor read frequency in seconds (300 = 5 minutes)
static uint16_t interruptInterval = 300;   
// ###############################################################################

//MS5803-14BA settings (addressing)
// HIGH for x76 (normal) // LOW for x77 (soldered jumper on MS5803 board)
MS5803 sensor(ADDRESS_HIGH);

//Initialize board
Stalker stalker;

// Save the Analog to Digital Conversion (ADC) configuration (to switch on/off to save battery)
byte adcsra_save = ADCSRA;

// The variables
float vol;
float temperature_water;
float pressure_water;
char datetime_buf[10]; // size big enough

// Initialize internal clock
DS1337 RTC;
static DateTime interruptTime;

File logFile;

byte checkSD = 1 ;

//############################## SETUP ############################
void setup() {
  /*Initialize INT0 pin for accepting interrupts */
  PORTD |= 0x04;
  DDRD &= ~ 0x04;
  Serial.begin(57600);
  Wire.begin();

  //pinMode(LED_BUILTIN, OUTPUT);
  pinMode(sensorPower, OUTPUT); //PIN 9 NEED TO BE HIGH TO POWER ANY SENSOR !!!
  digitalWrite(sensorPower, HIGH);
  sensor.reset();
  sensor.begin();

  pinMode(4, OUTPUT);   // SD communication is on pin 10, power on pin 4. 
  digitalWrite(4, LOW); //Power On SD Card. //SD Card power control pin. LOW = On, HIGH = Off
  initSD();
  digitalWrite(4, HIGH); //Power Off SD Card

  //Run clock
  RTC.begin();

  // UNCOMMENT the following line and upload the code ONCE to set the clock to the build time (PC time)
  // Once time is set, COMMENT the line and load again the code !
  RTC.adjust(DateTime(__DATE__, __TIME__)); 

  // setup interrupt (to wake up the board based on a time interval)
  attachInterrupt(0, INT0_ISR, LOW); //Only LOW level interrupt can wake up from PWR_DOWN
  DateTime  start = RTC.now();
  // We define the interrupt based on the time interval selected, the first wake up occurs at the next "round" time.
  interruptTime = DateTime(start.get()  + int(((((floor(start.minute() / (interruptInterval / 60)) + 1) * (interruptInterval / 60)) - start.minute()) * 60) - start.second()));
  //interruptTime = DateTime(start.get() + interruptInterval); //Add interruptInterval in seconds to start time
  delay(100);

  adcsra_save = ADCSRA;
  //ADCSRA = 0;
  //mySleepFct();
}

//############################## MAIN ############################
void loop() {

  // Read time and date
  DateTime now = RTC.now();
  //ADCSRA = adcsra_save;

  //===================================================================================
  //                   READ BATTERY STATE
  //====================================================================================

  delay(20);
  vol = stalker.readBattery();
  //Serial.println(vol);
  delay(100);

  //====================================================================================
  //                  CHECK BATTERY STATE (alarm if too low)
  //====================================================================================

  if (vol < 3.6) {
    Serial.print(F("too low"));
    Serial.print(vol);
    alarmLED();
  }

  //===================================================================================
  //                    READ SENSORS
  //====================================================================================

  temperature_water = sensor.getTemperature(CELSIUS, ADC_4096);
  pressure_water = sensor.getPressure(ADC_4096);

  // PRINT results in the serial
  sprintf(datetime_buf, "%02u-%02u-%02u %02u:%02u:%02u", now.year(), now.month(), now.date(), now.hour(), now.minute(), now.second());
  Serial.println(datetime_buf);
  Serial.println(vol);
  Serial.println(temperature_water);
  Serial.println(pressure_water);

  // ADCSRA = 0;

  //====================================================================================
  //                  PREPARE DATA TO BE SEND
  //====================================================================================

  //Check if any reads failed and alarm in case of problem !
  if (isnan(vol) ||  isnan(pressure_water) || temperature_water > 99 || temperature_water < -99) {
    Serial.println(F("Sensor fail"));
    pressure_water = -999;
    temperature_water = -999;
    alarmLED();
  }

  //===================================================================================
  //                    SAVE DATA ON SD CARD
  //====================================================================================

  // open SD channel
  digitalWrite(4, LOW); //Power On SD Card.
  delay(50);

  if (checkSD == 0) {
    initSD();
  }

  logFile = SD.open(fileName, FILE_WRITE);

  if (logFile) {
    logFile.print(datetime_buf);
    logFile.print(',');
    logFile.print(temperature_water);
    logFile.print(',');
    logFile.print(pressure_water);
    logFile.print(',');
    logFile.println(vol);;

    logFile.close();
    Serial.print(F("Data saved"));

  } else {
    Serial.println(F("Write fail"));
    checkSD = 0;
    alarmLED();
  }

  delay(50);
  // CLOSE SD channel
  digitalWrite(4, HIGH); //Power Off SD Card.

  //===================================================================================
  //                    DEEP SLEEP
  //===================================================================================

  //wdt_disable();
  delay(100);
  mySleepFct();
}

//====================================================================================
//                                   FONCTIONS
//====================================================================================

void initSD() {
  if (!SD.begin(10)) { //Chipselect is on pin 10
    Serial.println(F("SD fail"));
    checkSD = 0;
  } else {
    Serial.println(F("SD found"));
    checkSD = 1;
  }
}

void mySleepFct() {
  // The deep sleep function

  //digitalWrite(sensorPower, LOW);
  digitalWrite(4, HIGH);
  RTC.clearINTStatus(); //This function call is a must to bring /INT pin HIGH after an interrupt.
  RTC.enableInterrupts(interruptTime.hour(), interruptTime.minute(), interruptTime.second());  // set the interrupt at (h,m,s)
  sleep_enable();                     //before attaching the interrupt!!!IMPORTANT
  attachInterrupt(0, INT0_ISR, LOW);  //Enable INT0 interrupt (as ISR disables interrupt). This strategy is required to handle LEVEL triggered interrupt
  
  // Optimized sleeping instructions (very much inspired from sleep.h documentation and https://forum.arduino.cc/index.php?topic=409402.0)
  Serial.println(F("\nSleeping"));
  digitalWrite(sensorPower, LOW);
  delay(100);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // most power saving sleeping mode
  cli();
  power_all_disable(); //This shuts down ADC, TWI, SPI, Timers and USART
  sleep_bod_disable(); //Special power saving measure, not avilable for every Arduino board
  sei();
  sleep_cpu(); // sleeping
  sleep_disable(); // waking up
  power_all_enable(); // returning power to all elements mentioned above
  sei();
  digitalWrite(sensorPower, HIGH);
  delay(10); //This delay is required to allow CPU to stabilize
  Serial.println(F("Awake"));
}

void INT0_ISR() {
  //Keep this as short as possible. Possibly avoid using function calls
  
  sleep_disable(); // in case the interrupt was called before we sleep
  detachInterrupt(0);
  interruptTime = DateTime(interruptTime.get() + interruptInterval);  //decide the time for next interrupt, configure next interrupt
}

void alarmLED() {
  //CREATES A QUICK LED FLASHING IN CASE OF PROBLEM
  
  digitalWrite(10, LOW);
  for (int i = 0; i < 5; i++) {
    delay(50);
    logFile = SD.open("A", FILE_WRITE);
    delay(50);
    logFile.close();
    delay(300);
  }
  digitalWrite(10, HIGH);
}

void watchdogOn() {
  MCUSR = MCUSR & B11110111;
  WDTCSR = WDTCSR | B00011000;
  WDTCSR = B00100001;
  WDTCSR = WDTCSR | B01000000;
  MCUSR = MCUSR & B11110111;
}
