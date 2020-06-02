# Arduino DHTxx (xx=11,12,22,44) and AM2320 library
This is a library for the DHT11, DHT12, DHT22, DHT33, DHT44 and clone temperature and humidity sensors.
Read more on http://playground.arduino.cc/Main/Dht

## Warnings
These sensors are very slow and have a huge (~800uA) standby current (if they even have a standby mode). So the only option for power-aware use (e.g. in battery powered devices) is to switch then on/off. However they are also slow and we need to wait at least some milliseconds before access.
* The DHT12 needs at least 500ms after power on. Also there needs to be at least 100ms beetween two reads. 
* The is even slower and needs 1000ms after power on but handles reads in any speed.

### Credits
This library includes codes and ideas from various places.
* See http://playground.arduino.cc/main/DHT11Lib 
* See http://playground.arduino.cc/main/DHTLib 
* See https://www.mischianti.org/2019/01/01/dht12-library-en/
* See https://github.com/EngDial/AM2320/

### Requirements
This library needs to be compiled with C++11 features enabled -- any Arduino IDE later than 2015 will do that.
