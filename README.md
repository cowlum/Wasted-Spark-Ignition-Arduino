Wasted Spark Ignition system for atomic4 engine. Should work as the basis for most 4 cylinder engines.

TODO###

Use remotedebug librabry to send rpm,dwell,misfire eetc over telent.
https://github.com/JoaoLopesF/RemoteDebug
Remove the wifi webpage display and replace with nmea stream to onboard nmea server. e.g. IIRPM

December 2020 Engine running very well. 
Requires code and repo cleanup. 

ign-esp32-tdc-wifi.ino : Dual core setup for wifi. 

ign-esp8266-tdc.ino : Stopped developing this but it was running well, the basis should work for an single core arduino

The esp32 code is dual core and presents a wifi page for reading data as of 05/01/2022
The coil pack is a 032 905 106b 
Two magnets 180 degrees opposite, polar opposites are mounted to the fly wheel. One of the magents is at TDC.

Engine running on ESP32
https://www.youtube.com/watch?v=GZBaOKuiqLw


