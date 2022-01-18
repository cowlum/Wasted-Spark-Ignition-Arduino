Wasted Spark Ignition System for Atomic4 Engine. 
Should work as the basis for most 4 cylinder engines.

Calcualates ignition timing and sends RPM to the onbaord boat computer in NMEA format.

The ign-esp32-tdc-wifi.ino code is dual core and presents a wifi page for reading data 05/01/2022
The coil pack is a 032 905 106b VW imitation.
Two magnets 180 degrees opposite and polar opposites are mounted to the fly wheel (crank). 
One of the magents is placed at TDC.

December 2020 Engine running very well. 
Engine running on ESP32
https://www.youtube.com/watch?v=GZBaOKuiqLw


ign-esp32-tdc-nmea.ino: Calcualates ignition timing and sends RPM to the onbaord boat computer in NMEA format.
ign-esp32-tdc-wifi.ino : Dual core setup with Wifi page presenting data as per video. (obosolete)
ign-esp8266-tdc.ino : Stopped developing this but it was running well, the basis should work for an single core arduino (obsolete)

TODO::
Use remotedebug librabry to send rpm,dwell,misfire etc over telent https://github.com/JoaoLopesF/RemoteDebug = Completed Jan.22
Remove the wifi webpage display and replace with nmea stream to onboard nmea server. e.g. IIRPM = Completed Jan.22


