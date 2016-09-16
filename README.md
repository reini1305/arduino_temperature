# Desktop temperature sensor with web connection

This sketch uses 3 Dallas One-Wire temperature sensors to create a desktop
thermometer with a LC-Display. The humidity is measured using a DHT22. On the
desktop, a python script reads the current temperature via USB and stores it in
a CSV file. A web interface gives some statistic about daily and weekly trends.

A Pebble app is provided that displays the current temperature as an *AppGlance*.

This is what it looks like when finished:

<img src="https://github.com/reini1305/arduino_temperature/raw/master/arduinopebble.jpg"></img>
<img src="https://github.com/reini1305/arduino_temperature/raw/master/lcd2.jpg"></img>

and this is the corresponding web interface:

<img src="https://github.com/reini1305/arduino_temperature/raw/master/web.png"></img>
