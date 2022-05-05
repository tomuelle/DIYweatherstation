# Air temperature, humidity and pressure sensor

A large range of arduino-compatible temperature, humidity sensors are available. For example, a list of sensors is <a href="https://wiki.seeedstudio.com/Sensor_temperature/">available here</a>.

I tended to use a <a href="https://www.seeedstudio.com/Grove-BME280-Environmental-Sensor-Temperature-Humidity-Barometer.html/">BME280</a> in the field which also measures air pressure in order to estimate piezometric level (see my post <a href="/sensor_waterpressure">on water pressure sensor</a>). Don't forget to also use a radiation shield to protect the sensor from rain and solar radiation (for accurate temperature measurement). You can find relatively cheap shields, such as this one from <a href="https://www.seeedstudio.com/Solar-Radiation-Shield-for-Outdoor-Sensor-Protection-A10-p-4601.html/">Seeeduino</a>. Even better, you can 3D print it, here is a <a href="https://github.com/chrisys/mini-lora-weatherstation">very cool tutorial with 3D plans</a>, I tried it, it works nicely !

One note of caution, although the air pressure and temperature sensor measurements appeared to be very satisfying, with a similar precision as commercial sensors, the relative air humidity measurement seems less accurate. Indeed, when air moisture is high, the measurement seems saturated and remains stuck at 100 % and only decreases when a significantly drier weather occurs. There may be better outdoor sensors for air humidity.

