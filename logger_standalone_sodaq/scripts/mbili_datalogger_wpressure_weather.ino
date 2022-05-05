#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <Sodaq_DS3231.h>
#include <Sodaq_PcInt.h>
#include <SparkFun_MS5803_I2C.h>
#include "BlueDot_BME280.h"

// ################################## VARIABLE CONFIGURATION #####################
// Name of SD card save file
String fileName = "DataP1.CSV";

// Sensor read frequency in seconds (300 = 5 minutes)
static uint16_t interruptInterval = 300;
// ###############################################################################3

// BME settings
BlueDot_BME280 bme1;

//MS5803-14BA settings (addressing)
//HIGH for x76 (normal) // LOW for x77 (soldered jumper on MS5803 board)
MS5803 sensor(ADDRESS_HIGH);

//RTC Interrupt pin
#define RTC_PIN A7

//Digital pin 11 is the MicroSD slave select pin on the Mbili
#define SD_PIN 11

//BATTERY READING
#define ADC_AREF 3.3
#define BATVOLTPIN A6
#define BATVOLT_R1 4.7
#define BATVOLT_R2 10

// The sensor variables
float temperature;
float pressure;
float humdity;
float vol;
float temperature_water;
float pressure_water;

char dprint[7];
char datetime_buf[10];
String dateTime;

DateTime previousTime;
DateTime interruptTime;

bool checkSD = true;
static uint32_t baud_rate = 57600;

//#################################################################################################
//                                            SETUP
//#################################################################################################

void setup()
{
  //Initialise the serial connection
  Serial.begin(baud_rate);

  //Initialise sensors
  setupSensors();

  //Initialise log file
  setupLogFile();

  //Setup sleep mode 
  //NOTE:  UNCOMMENT LINE "rtc.setDateTime(DateTime(__DATE__, __TIME__));" in the setupSleep() function to synchronize time (see below) !
  setupSleep();
  
  DateTime  TimeInit;
  TimeInit = rtc.now();
  previousTime = TimeInit.get() - interruptInterval - 10;
  
  // RUN THE MAIN PROGRAM
  takeAction();

  // GO TO SLEEP
  TimeInit = rtc.now();
  previousTime = TimeInit.get() - interruptInterval - 10;
  systemSleep();
}

//#################################################################################################
//                                          MAIN LOOP
//#################################################################################################

void loop()
{
  // RUN THE MAIN PROGRAM
  takeAction();
  
   // GO TO SLEEP
  systemSleep();
}

//#################################################################################################
//                                           FUNCTIONS
//#################################################################################################

void takeAction()
{
  // Turn LED on to indicate sensor reading
  digitalWrite(LED2, HIGH);
  
  // Read time and date
  DateTime myTime = rtc.now();

  Serial.println(getDateTime());
  sprintf(datetime_buf, "%02u-%02u-%02u %02u:%02u:%02u", interruptTime.year(), interruptTime.month(), interruptTime.date(), interruptTime.hour(), interruptTime.minute(), interruptTime.second());
  Serial.println(datetime_buf);
  Serial.println(myTime.get());
  Serial.println(previousTime.get());
  Serial.println(int(myTime.get() - previousTime.get()));

  // The board wakes up sometimes just after going to sleep, after sensor reading (for unknown reasons). This avoids a second reading in case this occurs
  if (int(myTime.get() - previousTime.get()) < (interruptInterval - 10)) {
    previousTime = rtc.now();
    Serial.println(F("Not the time..."));
    systemSleep();

  // In the normal case, read the sensors
  } else {

    // #####  READ DATA  #####
    readSensors();

    // Check if any reads failed and alert (blink red LED), writes generic error value to SD (-999)
    if ( temperature_water > 99 || temperature_water < -99) {
      Serial.println(F("Failed to read from pressure sensor!"));
      interruptTime = DateTime(myTime.get()  + int(((((floor(myTime.minute() / (interruptInterval / 60)) + 1) * (interruptInterval / 60)) - myTime.minute()) * 60) - myTime.second()));

      temperature_water = -999;
      pressure_water = -999;
      alarmLED();
    }

    if (isnan(humdity) ||  isnan(vol) ||  humdity > 100 || humdity <= 0 || temperature > 99 || temperature < -99) {
      Serial.println(F("Failed to read from weather sensor!"));

      humdity = -999;
      pressure = -999;
      temperature = -999;
      alarmLED();
    }


    // #####  WRITE TO SD  #####
    //Create the data record
    String dataRec = createDataRecord();

    //Save the data record to the log file
    logData(dataRec);

	// Check if any error occured (alert (blink red LED))
    if (checkSD == false) {
      alarmLED();
    }

    Serial.print(F("Data saved to SD card : "));
    Serial.println(dataRec);

    // #####  BATTERY CHECK  ##### 	Check if battety is ok (alert (blink red LED))

    if (vol < 3.7) {
      Serial.print(F("Battery too low : "));
      Serial.println(vol);
      alarmLED();
    }

    //DEFINE NEXT TRIGGER TIME (We define the interrupt based on the time interval selected, the next wake up occurs at the next "round" time)
    interruptTime = DateTime(myTime.get()  + int(((((floor(myTime.minute() / (interruptInterval / 60)) + 1) * (interruptInterval / 60)) - myTime.minute()) * 60) - myTime.second()));

    previousTime = rtc.now();
  }
}

void alarmLED()
{
  for (int i = 1; i <= 3; i++) {
    digitalWrite(LED1, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(200);                       // wait for a second
    digitalWrite(LED1, LOW);    // turn the LED off by making the voltage LOW
    delay(200);
  }
}

void setupSensors()
{
  //INITIALIZE SENSORS
  //pinMode(GROVEPWR, OUTPUT);
  
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  Wire.begin();
  delay(100);
  sensor.reset();
  sensor.begin();
  delay(100);
  bme1.parameter.communication = 0;                   
  bme1.parameter.I2CAddress = 0x77;                    //I2C Address for BME Sensor: CHECK IF 0x77 or 0x76
  bme1.parameter.sensorMode = 0b11;
  bme1.parameter.IIRfilter = 0b100;
  bme1.parameter.humidOversampling = 0b101;
  bme1.parameter.tempOversampling = 0b101;
  bme1.parameter.pressOversampling = 0b101;
  bme1.init();
  delay(100);
}

void readSensors()
{
  // READ SENSORS
  
  dateTime = getDateTime();

  // read battery
  vol = (ADC_AREF / 1023.0) * (BATVOLT_R1 + BATVOLT_R2) / BATVOLT_R2 * analogRead(BATVOLTPIN);
  
  // read bme280 sensor
  temperature = bme1.readTempC();
  pressure = bme1.readPressure();
  humdity = bme1.readHumidity();

  // read water pressure sensor
  temperature_water = sensor.getTemperature(CELSIUS, ADC_4096);
  pressure_water = sensor.getPressure(ADC_4096);

  delay(100);
}

String createDataRecord()
{
  // write data to string for SD card
  String data = dateTime + ",";
  dtostrf(humdity, 5, 2, dprint);
  data += String(dprint) + ",";
  dtostrf(temperature, 5, 2, dprint);
  data += String(dprint) + ",";;
  dtostrf(pressure, 7, 2, dprint);
  data += String(dprint) + ",";;
  dtostrf(temperature_water, 5, 2, dprint);
  data += String(dprint) + ",";;
  dtostrf(pressure_water, 7, 2, dprint);
  data += String(dprint) + ",";;
  dtostrf(vol, 4, 2, dprint);
  data += String(dprint);
  return data;
}

void setupLogFile()
{
  //Initialise the SD card
  Serial.print(F("Load SD card..."));

  if (!SD.begin(SD_PIN))
  {
    Serial.println("Error: SD card failed to initialise or is missing.");
    checkSD = false;
    alarmLED();
    delay(10);
  } else {
    Serial.println(F("OK"));
    checkSD = true;
  }
}

void logData(String rec)
{
  // Write to SD
  if (!checkSD)
  {
    setupLogFile();
  }

  //Re-open the file
  File logFile = SD.open(fileName, FILE_WRITE);

  if (!logFile)
  {
    setupLogFile();
    logFile = SD.open(fileName, FILE_WRITE);
  }

  if (logFile)
  {
    checkSD = true;
    logFile.println(rec);
  }
  else
  {
    checkSD = false;
    Serial.println(F("!!!   Fail to SD card   !!!"));
  }
  //Close the file to save it
  logFile.close();
}

String getDateTime()
{
  // Read time
  String dateTimeStr;

  //Create a DateTime object from the current time
  DateTime dt(rtc.makeDateTime(rtc.now().getEpoch()));

  //Convert it to a String
  dt.addToString(dateTimeStr);

  return dateTimeStr;
}

//SLEEP FUNCTIONS
void wakeISR()
{
  //Leave this blank
}

void setupSleep()
{
  DateTime  TimeNow;
  //Setup the RTC in interrupt mode
  rtc.begin();
  
  // UNCOMMENT the following line and upload the code ONCE to set the clock to the build time (PC time). Once time is set, COMMENT the line and load again the code !
  //rtc.setDateTime(DateTime(__DATE__, __TIME__));

  pinMode(RTC_PIN, INPUT_PULLUP);
  PcInt::attachInterrupt(RTC_PIN, wakeISR);

  TimeNow = rtc.now();
  interruptTime = DateTime(TimeNow.get() + int(((((floor(TimeNow.minute() / (interruptInterval / 60)) + 1) * (interruptInterval / 60)) - TimeNow.minute()) * 60) - TimeNow.second()));

  Serial.println(getDateTime());
  sprintf(datetime_buf, "%02u-%02u-%02u %02u:%02u:%02u", interruptTime.year(), interruptTime.month(), interruptTime.date(), interruptTime.hour(), interruptTime.minute(), interruptTime.second());
  Serial.println(datetime_buf);

  //Set the sleep mode
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
}

void systemSleep()
{
  Serial.println(F("Going to sleep..."));
  digitalWrite(LED2, LOW);
  //This method handles any sensor specific sleep setup
  sensorsSleep();

  //Wait until the serial ports have finished transmitting
  Serial.flush();
  Serial1.flush();

  //The next timed interrupt will not be sent until this is cleared
  rtc.clearINTStatus();
  rtc.enableInterrupts(interruptTime.hour(), interruptTime.minute(), interruptTime.second());  // set the interrupt at (h,m,s)

  //Disable ADC
  ADCSRA &= ~_BV(ADEN);

  //Sleep time
  noInterrupts();
  sleep_enable();
  power_all_disable(); //This shuts down ADC, TWI, SPI, Timers and USART
  sleep_bod_disable(); //Special power saving measure, not avilable for every Arduino board
  interrupts();
  sleep_cpu(); // sleeping

  sleep_disable(); // waking up
  power_all_enable(); // returning power to all elements mentioned above

  //Enbale ADC
  ADCSRA |= _BV(ADEN);

  Serial.println(F("Waking up..."));
  //This method handles any sensor specific wake setup
  sensorsWake();
}

void sensorsSleep()
{
  //Add any code which your sensors require before sleep
}

void sensorsWake()
{
  //Add any code which your sensors require after waking
}
