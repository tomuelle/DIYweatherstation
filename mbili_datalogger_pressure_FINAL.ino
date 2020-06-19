#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <Sodaq_DS3231.h>
#include <Sodaq_PcInt.h>
#include <SparkFun_MS5803_I2C.h>

// ################################## ENTER FILE NAME ############################3
String fileName = "DataP5.CSV";
static uint16_t interruptInterval = 300;
// ###############################################################################3

//MS5803-14BA settings
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

char dprint[7];
char datetime_buf[10];
String dateTime;

float vol;
float temperature_water;
float pressure_water;

bool initbool;
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
  setupSleep();
  //Sleep

  initbool = 1;
  DateTime  TimeInit;
  TimeInit = rtc.now();
  takeAction();

  TimeInit = rtc.now();
  systemSleep();
  initbool = 0;
}

//#################################################################################################
//                                          MAIN LOOP
//#################################################################################################

void loop()
{

  takeAction();
  systemSleep();

}

//#################################################################################################
//                                           FUNCTIONS
//#################################################################################################

void takeAction()
{
  digitalWrite(LED2, HIGH);
  DateTime myTime = rtc.now();

  Serial.println(getDateTime());
  sprintf(datetime_buf, "%02u-%02u-%02u %02u:%02u:%02u", interruptTime.year(), interruptTime.month(), interruptTime.date(), interruptTime.hour(), interruptTime.minute(), interruptTime.second());
  Serial.println(datetime_buf);
  Serial.println(int(interruptTime.get() - myTime.get()));
  
  if ((int(interruptTime.get() - myTime.get()) > 0) && (initbool == 0)) {
    Serial.println(F("Not the time..."));
    //systemSleep();

  } else {

    // #####  READ DATA  #####
    readSensors();

    // Check if any reads failed and exit early (to try again).
    if (isnan(vol) ||  isnan(pressure_water) || temperature_water > 99 || temperature_water < -99) {
      Serial.println(F("Failed to read from sensor!"));
      interruptTime = DateTime(myTime.get()  + int(((((floor(myTime.minute() / (interruptInterval / 60)) + 1) * (interruptInterval / 60)) - myTime.minute()) * 60) - myTime.second()));

      pressure_water = -999;
      temperature_water = -999;
      alarmLED();
    }

    // #####  WRITE TO SD  #####
    //Create the data record
    String dataRec = createDataRecord();

    //Save the data record to the log file
    logData(dataRec);

    if (checkSD == false) {
      interruptTime = DateTime(myTime.get()  + int(((((floor(myTime.minute() / (interruptInterval / 60)) + 1) * (interruptInterval / 60)) - myTime.minute()) * 60) - myTime.second()));
      alarmLED();
    }

    Serial.print(F("Data saved to SD card : "));
    Serial.println(dataRec);

    // #####  BATTERY CHECK  #####
    if (vol < 3.7) {
      Serial.print(F("Battery too low : "));
      Serial.println(vol);
      interruptTime = DateTime(myTime.get()  + int(((((floor(myTime.minute() / (interruptInterval / 60)) + 1) * (interruptInterval / 60)) - myTime.minute()) * 60) - myTime.second()));
      alarmLED();
    }

    //DEFINE NEXT TRIGGER TIME
    interruptTime = DateTime(myTime.get()  + int(((((floor(myTime.minute() / (interruptInterval / 60)) + 1) * (interruptInterval / 60)) - myTime.minute()) * 60) - myTime.second()));

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
  Serial.println("CHECHK");
  //pinMode(GROVEPWR, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  Wire.begin();
  sensor.reset();
  sensor.begin();

  delay(100);
}

void readSensors()
{
  //Create a String type data record in csv format
  //  digitalWrite(GROVEPWR, HIGH);
  dateTime = getDateTime();

  vol = (ADC_AREF / 1023.0) * (BATVOLT_R1 + BATVOLT_R2) / BATVOLT_R2 * analogRead(BATVOLTPIN);

  temperature_water = sensor.getTemperature(CELSIUS, ADC_4096);
  pressure_water = sensor.getPressure(ADC_4096);

  delay(100);
  //  digitalWrite(GROVEPWR, LOW);
}

String createDataRecord()
{
  String data = dateTime + ",";
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
  //rtc.setDateTime(DateTime(__DATE__, __TIME__)); //Adjust date-time as defined 'dt' above

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
