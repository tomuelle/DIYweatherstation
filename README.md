# DIY automatic monitoring station
## Contact and credit
This tutorial was designed by Tom Müller, in the framework of a PhD thesis at the University of Lausanne (Switzerland) and was co-supervised by Bettina Schaefli (University of Bern, Switzerland) and Stuart Lane (University of Lausanne, Switzerland).

The codes are Open-Source. For scientific reference, please use the following link : https://doi.org/10.5194/egusphere-egu22-940

For any questions, please write to tom.muller1@gmail.com

## Description

This page provides different low-cost arduino based automatic datalogger solutions developed in the framework of my PhD work in a remote high alpine environment.
It contains all necessary information for the replication of such solutions and should be accessible for non-expert users. The solutions proposed here are based on my own experience and I stress that I had no prior training in sensor development nor electronics.

While the most simple <a href="https://www.arduino.cc/en/main/arduinoBoardUno">Arduino UNO</a> board only provides basic function to power low-power sensors, a large range of development boards were specifically developed to provide simple hardware solutions to develop low-cost dataloggers.
The main features of these boards are :
<ul>
  <li>The basic functions of an adruino boards (Microcontroller, digital, analog and I2C pins); </li>
  <li>A low-power consumption; </li>
  <li>A real-time clock (RTC) clock to record time and to allow an RTC external interrupts to wake up the board from deep sleep;</li>
  <li>A built-in connector for a 3.7V LiPo battery;</li>
  <li>A built-in connector for a 5V solar pannel with a charge controller;</li>
  <li>A built-in LoRa module or a socket to add a LoRa module with antenna;</li>
  <li>A SD Card holder to save data locally.</li>
</ul>  

## Arduino-based data acquisition board

We have tested and used three different development boards which are summarized hereafter.

### Seeeduino Stalker v3.1
<img align="right" src="images/seeeduino.jpg" width="200"/>
<p align="justify">
<a href="https://wiki.seeedstudio.com/Seeeduino_Stalker_V3.1/">The Seeeduino Stalker v3.1</a> board contains all necessary features to build a simple local autonomious datalogger. It has a relatively low internal memory (Atmega328P) so that it should be used for small applications with only one or two sensors. It is the cheapest option but shows sometimes small bugs, although it is overall very satisfying. It is imported from the US and may be subject to supply limitation.
  
A description of a simple water table monitoring project is available in the <a href="/logger_standalone_seeed">logger standalone Seeed folder</a>.
</p>
<br>

### SODAQ Mbili
<img align="right" src="images/sodaq.jpg" width="200"/>
<p align="justify">
<a href="https://support.sodaq.com/Boards/Mbili/">The SODAQ Mbili</a> board is quite similar but has a larger memory (ATmega 1284P), has some more flexible hardware settings and comes with plug-in grove sockets. It is a bit more expensive and is imported from the Netherland (Europe). SODAQ also proposes a bee-socket where a <a href="https://shop.sodaq.com/lorabee-rn2483-order-now.html">LoRa bee-module</a> can be added in order to send data remotely. 
  
A description of a simple water table monitoring project is available in the <a href="/logger_standalone_sodaq">logger standalone Sodaq folder</a>.</p>
<br>

### CubeCell Lora Dev-Board
<img align="right" src="images/cubecell.jpg" width="200"/>
<p align="justify">
<a href="https://heltec.org/product/htcc-ab01/">The CubeCell Lora</a> Dev-Board is a low-cost, low-power board specifically designed for LoRaWAN applications. This board does not have any SD card holder nor RTC clock and data are only sent using the built-in LoRa module. The data can only be retrieved using a LoRa gateway and are not saved on the device.

For this LoRaWAN application, we developped a completely autonomous, solar-based, local wireless network for regions without internet (4G) connection which allows to retrieve data from mutliple LoRa stations. A similar solution using 4G is also shown. Details are accessible in the <a href="/logger_LoRaWan">LoRaWan_logger folder</a>.
</p>
