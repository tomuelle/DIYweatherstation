# Air temperature, humidity and pressure sensor

A large range of arduino-compatible temperature, humidity sensors are available. For example, a list of sensors is <a href="https://wiki.seeedstudio.com/Sensor_temperature/">available here</a>.

I tended to use a <a href="https://www.seeedstudio.com/Grove-Temperature-Humidity-Pressure-and-Gas-Sensor-for-Arduino-BME680.html/">BME680</a> in the field which also measures air pressure in order to estimate piezometric level (see my post <a href="/sensor_waterpressure">on water pressure sensor</a>). Don't forget to also use a radiation shield to protect the sensor from rain and solar radiation (for accurate temperature measurement). You can find relatively cheap shields, such as this one from <a href="https://www.seeedstudio.com/Solar-Radiation-Shield-for-Outdoor-Sensor-Protection-A10-p-4601.html/">Seeeduino</a>. Even better, you can 3D print it, here is a <a href="https://github.com/chrisys/mini-lora-weatherstation">very cool tutorial with 3D plans</a>, I tried it, it works nicely !

