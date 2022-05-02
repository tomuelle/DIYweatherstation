# LoRaWan automatic datalogger 

An automatic connected datalogger to record and send data using the low-power LoRaWan network.

The architecture of the system is composed of different blocks :
 <ol>
  <li>The actual arduino-based datalogger with LoRa antenna</li>
  <li>A LoRaWan gateway to receive any LoRa RF signal and transmit it</li>
  <li>A server to communicate with the LoRaWan gateway to store data and provide queries for vizualisation</li>
</ol> 

<img src="images/loraWan.jpg" width="960"/>



