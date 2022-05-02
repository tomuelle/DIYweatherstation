# Waterproof pressure sensor

This sensor was inspired from this post of enviroDIY : https://www.envirodiy.org/construction-of-water-level-monitoring-sensor-station/
Links are examples of hardware providers for Switzerland in 2022.

The idea is to use a pressure valve (MS5803-14BA), solder a cable to the pins and envelop it with epoxy glue. All info can be found on the link above.
My current design is composed of the following components :
<ul>
  <li>1x MS5803-14BA : https://www.digikey.ch/short/25zndvnp
  <li>2.5 meters Mutlicore cable (4x0.5mm2) : https://www.distrelec.ch/en/multicore-cable-yy-pvc-4x-5mm-grey-50m-lapp-00100024-50/p/30049250?no-cache=true&track=true
  <li>Epoxy glue : https://www.reichelt.com/de/en/wiko-epoxy-adhesive-5-min-wiko-epo5-s25-p98449.html?r=1
  <li>Acrylic lacquer : https://www.reichelt.com/de/en/acrylic-protective-lacquer-400-ml-rnd-605-00135-p211619.html?r=1
</ul>

You basically solder the cable, spray lacquer (but care to protect the white pressure valve), apply epoxy arond the circuit board. This design is very rough and can clearly be improved, but it showed satisfying results to monitor water pressure during one year in a 2 m deep well.

<p align="center">
  <img src="images/sensor_waterpressure_1.jpg" height="300" />
  <img src="images/sensor_waterpressure_2.jpg" height="300" /> 
</p>
