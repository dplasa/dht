 /*  
 *   Example for a DHT12 sensor
 *   Beware some of the diagrams about the pinouts are incorrect.
 *   Pin Definition 1.VDD 2.SDA 3.GND 4.SCL
 *   Look at the sensor one side has holes to allow air inside. 
 *   Let's call the surface with holes the FRONT.
 *   The BACK of the sensor has printed codes and no holes.
 *   The 4 PINS for connection come out of the BOTTOM of the sensor.
 *   OK so we have defined the orientation of the sensor.
 *   Looking at the FRONT of the Sensor with the legs pointing down 
 *   the LEFTMOST PIN is PIN ONE
 *   
 *   
 *   -----------------
 *   !   O O O O O   !
 *   !   O O O O O   !
 *   !   O O O O O   !
 *   !   O O O O O   !
 *   -----------------
 *       !  !  !  !
 *       !  !  !  !
 *       !  !  !  !
 *       !  !  !  ! 
 *       1  2  3  4
 *       
*/

#include <Wire.h>
#include <dht.h>

dht12 DHT(0x5c);

void setup()
{
  Serial.begin(9600);
  Serial.println(F("DHT12 TEST PROGRAM"));
}

void loop()
{
  unsigned long b = micros();
  dht::ReadStatus chk = DHT.read();
  unsigned long e = micros();

  Serial.print(F("Read sensor: "));
  switch (chk)
  {
    case dht::OK:
      Serial.print(F("OK, took "));
      Serial.print (e - b); Serial.print(F(" usec, "));
      break;
    case dht::ERROR_CHECKSUM:
      Serial.println(F("Checksum error"));
      break;
    case dht::ERROR_TIMEOUT:
      Serial.println(F("Timeout error"));
      break;
    case dht::ERROR_CONNECT:
      Serial.println(F("Connect error"));
      break;
    case dht::ERROR_ACK_L:
      Serial.println(F("AckL error"));
      break;
    case dht::ERROR_ACK_H:
      Serial.println(F("AckH error"));
      break;
    default:
      Serial.println(F("Unknown error"));
      break;
  }

  Serial.print(F("Humidity: "));
  Serial.print((float)DHT.getHumidity()/(float)10);
  Serial.print(F("%, "));

  Serial.print(F(". Temperature (degrees C): "));
  Serial.print((float)DHT.getTemperature()/(float)10);

  Serial.print(F(", Dew Point (degrees C): "));
  Serial.println(DHT.dewPoint());

  delay(4000);
}
//
// END OF FILE
//
