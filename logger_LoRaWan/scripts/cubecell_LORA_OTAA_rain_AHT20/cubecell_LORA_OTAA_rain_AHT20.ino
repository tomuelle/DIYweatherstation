#include <LoRaWan_APP.h>
#include <Arduino.h>
#include "CubeCell_NeoPixel.h"
CubeCell_NeoPixel pixels(1, RGB, NEO_GRB + NEO_KHZ800);
#include <SparkFun_MS5803_I2C.h>
#include "SparkFun_Qwiic_Humidity_AHT20.h"

#include "ttnparams.h"

#define INT_PIN GPIO3

bool ENABLE_SERIAL = true; // enable serial debug output here if required
uint32_t appTxDutyCycle = 300000; // the frequency of readings, in milliseconds(set 300s)

uint16_t userChannelsMask[6] = { 0x00FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;
DeviceClass_t  loraWanClass = LORAWAN_CLASS;
bool overTheAirActivation = LORAWAN_NETMODE;
bool loraWanAdr = LORAWAN_ADR;
bool keepNet = LORAWAN_NET_RESERVE;
bool isTxConfirmed = LORAWAN_UPLINKMODE;
uint8_t appPort = 2;
uint8_t confirmedNbTrials = 4;

uint16_t rain, rain0, temperature, humidity, batteryVoltage, cumulRain;
uint32_t pressure;

int * downlinkValue;
bool downlinkOK;

bool accelWoke = false;
bool sendCheck = true;
int colors[3] = {0, 0, 0};
uint16_t countErr ;

AHT20 humiditySensor;

void(* resetFunc) (void) = 0;//declare reset function at address 0

void setup()
{
  boardInitMcu();
  if (ENABLE_SERIAL) {
    Serial.begin(115200);
  }

  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW); //SET POWER
  pinMode(GPIO2, OUTPUT);
  digitalWrite(GPIO2, LOW); //SET POWER
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.clear(); // Set all pixel colors to 'off'
  delay(100);
  colors[1] = 1;
  alarmLED(0);
  delay(100);
  
  deviceState = DEVICE_STATE_INIT;
  LoRaWAN.ifskipjoin();

  accelWoke = false;
  pinMode(INT_PIN, INPUT_PULLUP);
  attachInterrupt(INT_PIN, accelWakeup, CHANGE);
  Serial.println("Interrupts attached");

  rain = 0;
  rain0 = 0;
}

static void prepareTxFrame( uint8_t port )
{
  //if (downlinkOK == false) {
  // countErr += 1;
  //}

  // This enables the output to power the sensor
  digitalWrite(Vext, LOW);
  delay(500);

  initAHT20();
  delay(100);

  long mytime = millis();
  while ((!humiditySensor.available()) & (millis() - mytime) < 5100) {
    //Serial.println(millis() - mytime);
    delay(100);
    if ( (millis() - mytime) > 5100 ) {
      Serial.println("Temp sensor error");
    }
  }

  temperature = humiditySensor.getTemperature() * 100;
  humidity = humiditySensor.getHumidity() * 100;
  pressure = 1000;
  
  delay(100);
  sensor.reset();
  sensor.begin();
  delay(100);

  temperature_water = sensor.getTemperature(CELSIUS, ADC_4096) * 100;
  pressure_water = sensor.getPressure(ADC_4096) * 100;

  // Turn the power to the sensor off again
  digitalWrite(Vext, HIGH);

  cumulRain = cumulRain + rain0;
  rain0 = 0;

  batteryVoltage = getBatteryVoltage();

  if (humidity > 10000 || humidity < 100 || temperature > 9900)  {
    Serial.println(F("Failed to read from w. sensor!"));

    humidity = 33300;
    temperature = 33300;

    colors[0] = 1;
    //////alarmLED(6);
    sendCheck = true; //FALSE TO PROHIBIT SEND
    countErr += 1;
    if (countErr == 24) {
      Serial.println(F("Reset..."));
      countErr = 0;
      delay(100);
      CySoftwareReset();
    } else {
      sendCheck = true;
      colors[1] = 1;
      alarmLED(3);
    }
  }

  if ( pressure / 100 <= 100 || (pressure / 100) > 1800) {
    Serial.println(F("Failed to read from p. sensor!"));

    pressure = 33300;
    colors[0] = 1;
    alarmLED(6);
    sendCheck = true; //FALSE TO PROHIBIT SEND
    countErr += 1;
    if (countErr == 24) {
      Serial.println(F("Reset..."));
      countErr = 0;
      delay(100);
      CySoftwareReset();
    } else {
      sendCheck = true;
    }
  }

  //#####  BATTERY CHECK  #####
  if (batteryVoltage < 3700) {
    colors[0] = 5;
    colors[1] = 1;
    alarmLED(6);
  }

  appDataSize = 14;
  appData[0] = highByte(temperature);
  appData[1] = lowByte(temperature);

  appData[2] = highByte(humidity);
  appData[3] = lowByte(humidity);

  appData[4] = (byte) ((pressure & 0xFF000000) >> 24 );
  appData[5] = (byte) ((pressure & 0x00FF0000) >> 16 );
  appData[6] = (byte) ((pressure & 0x0000FF00) >> 8  );
  appData[7] = (byte) ((pressure & 0X000000FF)       );

  appData[8] = highByte(rain);
  appData[9] = lowByte(rain);

  appData[10] = highByte(cumulRain);
  appData[11] = lowByte(cumulRain);

  appData[12] = highByte(batteryVoltage);
  appData[13] = lowByte(batteryVoltage);

  if (ENABLE_SERIAL) {
    Serial.print("Rain: ");
    Serial.print(rain);
    Serial.print("mm, Cumul. rain: ");
    Serial.print(cumulRain);
    Serial.print("mm, Temperature: ");
    Serial.print(temperature / 100);
    Serial.print("C, Humidity: ");
    Serial.print(humidity / 100);
    Serial.print("%, Pressure: ");
    Serial.print(pressure / 100);
    Serial.print(" hPa, Battery Voltage: ");
    Serial.print(batteryVoltage);
    Serial.println(" V");
  }

  downlinkOK = false;
}

void initAHT20() {
  long mytime = millis();
  bool sensorOK = true;
  //Check if the AHT20 will acknowledge
  while ((humiditySensor.begin() == false) & ((millis() - mytime) < 2100))
  {
    delay(100);
    if ( (millis() - mytime) > 1900 ) {
      Serial.println(F("AHT20 not detected."));
      sensorOK = false;
    }
  }
  if (sensorOK == true) {
    Serial.println("AHT20 acknowledged.");
  }
  delay(100);
}

void accelWakeup()
{
  delay(10);
  accelWoke = true;
}

void loop()
{
  switch ( deviceState )
  {
    case DEVICE_STATE_INIT:
      {
        printDevParam();
        LoRaWAN.init(loraWanClass, loraWanRegion);
        deviceState = DEVICE_STATE_JOIN;
        break;
      }
    case DEVICE_STATE_JOIN:
      {
        LoRaWAN.join();
        break;
      }
    case DEVICE_STATE_SEND:
      {
        prepareTxFrame( appPort );
        if (sendCheck == true) {
          LoRaWAN.send();
          colors[1] = 1;
          //alarmLED(3);
        }
        deviceState = DEVICE_STATE_CYCLE;
        break;
      }
    case DEVICE_STATE_CYCLE:
      {
        // Schedule next packet transmission
        Serial.println(F("Cycle..."));
        txDutyCycleTime = appTxDutyCycle + randr( 0, APP_TX_DUTYCYCLE_RND );
        LoRaWAN.cycle(txDutyCycleTime);
        deviceState = DEVICE_STATE_SLEEP;
        break;
      }
    case DEVICE_STATE_SLEEP:
      {
        //Serial.println(F("Sleeping..."));
        if (accelWoke) {
          rain = rain + 0.1 * 100;
          rain0 = rain0 + 0.1 * 100;
          uint32_t now = TimerGetCurrentTime();
          Serial.print(now); Serial.print(F(", Rain: "));
          Serial.println(rain);
          accelWoke = false;
          colors[2] = 1;
          alarmLED(0);
        } else {
          downlinkValue = returnFct();
          if (downlinkValue[0] != 0) {
            Serial.print(F("RSSI: "));
            Serial.print(downlinkValue[0]);
            Serial.print(F("; SNR: "));
            Serial.print(downlinkValue[1]);
            Serial.print(F("; RX: "));
            Serial.println(downlinkValue[2]);
            downlinkOK = true;
            //countErr = 0;
            colors[2] = 1;
            alarmLED(3);

            downlinkValue[0] = 0;

            rain = 0;

            if (cumulRain > 10000) {
              cumulRain = 0;
            }

          }
        }
        LoRaWAN.sleep();
        break;
      }
    default:
      {
        deviceState = DEVICE_STATE_INIT;
        break;
      }
  }
}
