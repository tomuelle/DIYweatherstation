#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <Sodaq_DS3231.h>
#include <Sodaq_PcInt.h>
#include "BlueDot_BME280.h"
//#include <Sodaq_RN2483.h>

#define debugSerial Serial
#define loraSerial Serial1
#define BEE_VCC 20

// ################################## ENTER FILE NAME ############################3
String fileName = "Meteo_v2.CSV";
static uint16_t interruptInterval = 300;
// ###############################################################################3

//https://support.sodaq.com/Boards/Mbili/Examples/interrupts/
//#define reedPin D10      // reed switch

// BME settings
BlueDot_BME280 bme1;

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

uint16_t temperature;
uint32_t pressure;
uint16_t humidity;
uint16_t vol;
uint16_t rain;
bool rainEvent = 0;
uint16_t countErr = 0;

//DateTime previousTime;
DateTime interruptTime;
bool initbool = 1;

bool checkSD = true;
static uint32_t baud_rate = 57600;

bool check;

const char *DevEUI = "";
const char *AppEUI = "";
const char *AppKey = "";
char rgbTxt[24];

//#################################################################################################
//                                            SETUP
//#################################################################################################

void setup()
{
  debugSerial.begin(57600);
  loraSerial.begin(57600);

  delay(100);

  //Initialise sensors
  setupSensors();

  setupLoRa();

  //Initialise log file
  setupLogFile();

  //Setup sleep mode
  setupSleep();
  //Sleep

  //DateTime  TimeInit;
  //TimeInit = rtc.now();
  //previousTime = TimeInit.get() - interruptInterval - 10;
  takeAction();
  initbool = 0;
  //TimeInit = rtc.now();
  //previousTime = TimeInit.get() - interruptInterval - 10;
  systemSleep();
}

//#################################################################################################
//                                          MAIN LOOP
//#################################################################################################

void(* resetFunc) (void) = 0;//declare reset function at address 0

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

  debugSerial.println(getDateTime());
  sprintf(datetime_buf, "%02u-%02u-%02u %02u:%02u:%02u", interruptTime.year(), interruptTime.month(), interruptTime.date(), interruptTime.hour(), interruptTime.minute(), interruptTime.second());
  debugSerial.println(datetime_buf);

  //debugSerial.println(previousTime.get());
  //debugSerial.println(interruptTime.get());
  //debugSerial.println(myTime.get());
  //debugSerial.println(int(myTime.get() - previousTime.get()));
  //debugSerial.println(int(interruptTime.get() - myTime.get()));
  //debugSerial.println(rain);
  //debugSerial.println(initbool);

  if (rainEvent == 1  && (int(interruptTime.get() - myTime.get()) > 0)) {
    rain = rain + 0.2 * 100;
    //DEFINE NEXT TRIGGER TIME
    interruptTime = DateTime(myTime.get()  + int(((((floor(myTime.minute() / (interruptInterval / 60)) + 1) * (interruptInterval / 60)) - myTime.minute()) * 60) - myTime.second()));
    rainEvent = 0;
    debugSerial.println(F("rain check"));
    debugSerial.println(rain);
    //systemSleep();
    return;

  } else if ((int(interruptTime.get() - myTime.get()) > 0) && (initbool == 0)) {
    //previousTime = rtc.now();
    debugSerial.println(F("Not the time..."));
    //systemSleep();
    return;

  } else {
    // #####  READ DATA  #####
    readSensors();

    int checkRead = 0;
    //Check if any reads failed and exit early (to try again).
    while (isnan(humidity) || humidity > 10000 || humidity < 0 || temperature > 9900 || (pressure / 100 + 800) > 1000) {
      //humidity = 99900;
      //temperature = 99900;
      //pressure = 99900;

      debugSerial.println(F("Failed to read from sensor!"));
      //interruptTime = DateTime(myTime.get()  + int(((((floor(myTime.minute() / (interruptInterval / 60)) + 1) * (interruptInterval / 60)) - myTime.minute()) * 60) - myTime.second()));
      delay(100);
      readSensors();
      checkRead++;

      if (checkRead > 2) {
        String dataRec = createDataRecord();
        debugSerial.println(dataRec);

        alarmLED();
        systemSleep();
        return;
      }
    }
    //systemSleep();
    //return;

    // #####  WRITE TO SD  #####
    //Create the data record
    String dataRec = createDataRecord();

    //Save the data record to the log file
    logData(dataRec);

    if (checkSD == false) {
      interruptTime = DateTime(myTime.get()  + int(((((floor(myTime.minute() / (interruptInterval / 60)) + 1) * (interruptInterval / 60)) - myTime.minute()) * 60) - myTime.second()));
      alarmLED();
      debugSerial.println(F("Fail to save to SD card"));
      //systemSleep();
      //return;
    }
    else {
      debugSerial.println(F("Data saved to SD card : "));
    }
    debugSerial.println(dataRec);
    //#####################################################################3

    createDataLora();
    delay(100);
    debugSerial.println(F("Sending LORA..."));

    digitalWrite(LED2, LOW);
    sendLoRa();

    //#####  BATTERY CHECK  #####
    if (vol < 3.7 * 100) {
      debugSerial.print(F("Battery too low : "));
      debugSerial.println(vol);
      //interruptTime = DateTime(myTime.get()  + int(((((floor(myTime.minute() / (interruptInterval / 60)) + 1) * (interruptInterval / 60)) - myTime.minute()) * 60) - myTime.second()));
      alarmLED();
      //systemSleep();
      //return;
    }

    //DEFINE NEXT TRIGGER TIME
    interruptTime = DateTime(myTime.get()  + int(((((floor(myTime.minute() / (interruptInterval / 60)) + 1) * (interruptInterval / 60)) - myTime.minute()) * 60) - myTime.second()));
    rain = 0;
    //previousTime = rtc.now();
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
  debugSerial.begin(57600);

  //pinMode(GROVEPWR, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  // THIS IS TO ALLOW INTERUPT ON REED SWITCH INT2 (D10)
  pinMode(22, OUTPUT);
  digitalWrite(22, HIGH);

  Wire.begin();
  rain=0;
  delay(100);
}

void readSensors()
{

  //0b00:     In sleep mode no measurements are performed, but power consumption is at a minimum
  //0b01:     In forced mode a single measured is performed and the device returns automatically to sleep mode
  //0b11:     In normal mode the sensor measures continually (default value)

  bme1.parameter.communication = 0;
  bme1.parameter.I2CAddress = 0x76; //0x76
  bme1.parameter.sensorMode = 0b11;
  bme1.parameter.IIRfilter = 0b100;
  bme1.parameter.humidOversampling = 0b101;
  bme1.parameter.tempOversampling = 0b101;
  bme1.parameter.pressOversampling = 0b101;

  bme1.init();
  //Create a String type data record in csv format
  //digitalWrite(GROVEPWR, HIGH);
  dateTime = getDateTime();

  vol = 100 * (ADC_AREF / 1023.0) * (BATVOLT_R1 + BATVOLT_R2) / BATVOLT_R2 * analogRead(BATVOLTPIN);
  temperature = 100 * bme1.readTempC();
  pressure = 100 * (bme1.readPressure() - 800);
  //pressure = pressure / 100;
  humidity = 100 * bme1.readHumidity();

  //Serial.println(temperature);
  delay(100);
  //  digitalWrite(GROVEPWR, LOW);
}

String createDataRecord()
{
  String data = dateTime + ",";
  dtostrf((float)rain / 100, 4, 1, dprint);
  data += String(dprint) + ",";
  dtostrf((float)humidity / 100, 5, 2, dprint);
  data += String(dprint) + ",";
  dtostrf(temperature / 100, 5, 2, dprint);
  data += String(dprint) + ",";
  dtostrf((float)(pressure / 100 + 800), 7, 2, dprint);
  data += String(dprint) + ",";
  dtostrf((float)vol / 100, 4, 2, dprint);
  data += String(dprint);
  return data;
}

void createDataLora()
{
  byte_to_str(&rgbTxt[0], highByte(rain));
  byte_to_str(&rgbTxt[2], lowByte(rain));

  byte_to_str(&rgbTxt[4], highByte(humidity));
  byte_to_str(&rgbTxt[6], lowByte(humidity));

  byte_to_str(&rgbTxt[8], highByte(temperature));
  byte_to_str(&rgbTxt[10], lowByte(temperature));

  byte_to_str(&rgbTxt[12], highByte(pressure));
  byte_to_str(&rgbTxt[14], lowByte(pressure));

  byte_to_str(&rgbTxt[16], highByte(vol));
  byte_to_str(&rgbTxt[18], lowByte(vol));
}

void setupLogFile()
{
  //Initialise the SD card
  debugSerial.print(F("Load SD card..."));

  if (!SD.begin(SD_PIN))
  {
    debugSerial.println("Error: SD card failed to initialise or is missing.");
    checkSD = false;
    alarmLED();
    delay(10);
  } else {
    debugSerial.println(F("OK"));
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
    debugSerial.println(F("!!!   Fail to SD card   !!!"));
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

void wakeRain()
{
  rainEvent = 1;
}

void setupSleep()
{
  DateTime  TimeNow;
  //Setup the RTC in interrupt mode
  rtc.begin();
  //rtc.setDateTime(DateTime(__DATE__, __TIME__)); //Adjust date-time as defined 'dt' above

  pinMode(RTC_PIN, INPUT_PULLUP);
  PcInt::attachInterrupt(RTC_PIN, wakeISR);

  //pinMode(22, OUTPUT);
  //digitalWrite(22, HIGH);

  attachInterrupt(2, wakeRain, CHANGE);  // reed switch ON; 0 = RX1; 2 = PIN D10
  //PcInt::attachInterrupt(A4, wakeRain);
  TimeNow = rtc.now();
  interruptTime = DateTime(TimeNow.get() + int(((((floor(TimeNow.minute() / (interruptInterval / 60)) + 1) * (interruptInterval / 60)) - TimeNow.minute()) * 60) - TimeNow.second()));

  debugSerial.println(getDateTime());
  sprintf(datetime_buf, "%02u-%02u-%02u %02u:%02u:%02u", interruptTime.year(), interruptTime.month(), interruptTime.date(), interruptTime.hour(), interruptTime.minute(), interruptTime.second());
  debugSerial.println(datetime_buf);

  //Set the sleep mode
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
}

void systemSleep()
{
  debugSerial.println(F("Going to sleep..."));
  loraSerial.println(F("sys sleep 600000"));

  digitalWrite(LED2, LOW);
  //This method handles any sensor specific sleep setup
  sensorsSleep();

  //Wait until the serial ports have finished transmitting
  debugSerial.flush();
  loraSerial.flush();

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
  debugSerial.println(F("Waking up..."));
  //This method handles any sensor specific wake setup
  //LoRaBee.wakeUp();
  sensorsWake();
  changeBaud();
}

void sensorsSleep()
{
  //Add any code which your sensors require before sleep
}

void sensorsWake()
{
  //Add any code which your sensors require after waking
}

void readBee() {
  delay(200);
  unsigned long currentMillis = millis();
  while (loraSerial.available()) {
    if ((millis() - currentMillis) > 5000) {
      check = false;
      debugSerial.println(F("Time out"));
      break;
    }
    if ( loraSerial.available() )   {
      debugSerial.write((char) loraSerial.read() );
    }
  }
  debugSerial.println(F(""));
}

void setupLoRa() {
  changeBaud() ;
  loraSerial.println(F("sys reset"));
  checkBeeCommand("RN", 5000);
  loraSerial.print("mac set deveui ");
  loraSerial.println(DevEUI);
  checkBeeCommand("ok", 2000);
  loraSerial.print("mac set appeui ");
  loraSerial.println(AppEUI);
  checkBeeCommand("ok", 2000);
  loraSerial.print("mac set appkey ");
  loraSerial.println(AppKey);
  checkBeeCommand("ok", 2000);

  loraSerial.println("mac set rx2 3 869525000");
  checkBeeCommand("ok", 2000);
  loraSerial.println("mac set rxdelay1 5000");
  checkBeeCommand("ok", 2000);
  loraSerial.println("mac set adr on");
  checkBeeCommand("ok", 2000);
  loraSerial.println("mac save");
  checkBeeCommand("ok", 5000);

  Serial.println(F("Send join request..."));
  //loraSerial.println("mac set dr 0");
  //checkBeeCommand("ok", 2000);
  loraSerial.println("mac join otaa");
  checkBeeCommand("accepted", 10000);
}

void sendLoRa() {

  if (countErr == 4) {
    resetFunc();
  }

  Serial.println(rgbTxt);
  loraSerial.print(F("mac tx uncnf 1 "));
  loraSerial.println(rgbTxt);

  int i = 3;
  while (!checkBeeCommand("mac_tx_ok", 10000) && (i > 0) ) {
    //setupLoRa();
    loraSerial.print(F("mac tx uncnf 1 "));
    loraSerial.println(rgbTxt);
    if (checkBeeCommand("not_joined", 10000)) {
      loraSerial.println("mac join otaa");
      checkBeeCommand("accepted", 10000);
    }
    alarmLED();
    delay(1000);

    i--;
  };
  if (i > 0) {
    countErr = 0;
    digitalWrite(LED2, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(200);                       // wait for a second
    digitalWrite(LED2, LOW);    // turn the LED off by making the voltage LOW
    delay(200);
  } else {
    countErr = countErr + 1;
    alarmLED();
  }
}

void byte_to_str(char* buff, uint8_t val) {  // convert an 8-bit byte to a string of 2 hexadecimal characters
  buff[0] = nibble_to_hex(val >> 4);
  buff[1] = nibble_to_hex(val);
}

char nibble_to_hex(uint8_t nibble) {  // convert a 4-bit nibble to a hexadecimal character
  nibble &= 0xF;
  return nibble > 9 ? nibble - 10 + 'A' : nibble + '0';
}

boolean checkBeeCommand(String StrCheck, uint16_t timeint) {
  String message;
  bool check = false;
  unsigned long currentMillis = millis();

  while (1) {
    if ( Serial1.available() )   {
      message =  Serial1.readString();
      Serial.println(message);
    }
    if (message.indexOf("denied") >= 0) {
      check = false;
      Serial.println(F("Error"));
      break;
    }
    if (message.indexOf(StrCheck) >= 0) {
      check = true;
      Serial.println(F("Success!"));
      break;
    }
    if (((millis() - currentMillis) > timeint) ) {
      check = false;
      Serial.println(F("Timeout!"));
      delay(100);
      //  resetFunc();
      break;
    }
  }
  return check;
}

void changeBaud() {
  delay(500);
  loraSerial.begin(300);
  loraSerial.write((uint8_t)0x00);

  while (loraSerial.available())
  {
    Serial.write((char)loraSerial.read());
  }
  delay(50);
  loraSerial.begin(57600);
  loraSerial.write((uint8_t)0x55);

  while (loraSerial.available())
  {
    Serial.write((char)loraSerial.read());
  }
}
