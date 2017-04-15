//
// Example for a DHT11 sensor
//

#include <Wire.h>
#include <dht.h>

#define DHT11PIN 8
dht1wire DHT(DHT11PIN, dht::DHT11);

void setup()
{
  Serial.begin(9600);
  Serial.println(F("DHTxx TEST PROGRAM"));
}

void loop()
{
  unsigned long b = micros();
  dht::ReadStatus chk = DHT.read();
  unsigned long e = micros();

  Serial.print(F("\nRead sensor: "));
  switch (chk)
  {
    case dht::OK:
      Serial.print(F("OK, took "));
      Serial.print (e - b); Serial.println(F(" usec"));
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

  Serial.print(F("Humidity (%): "));
  Serial.println(DHT.getHumidity());

  Serial.print(F("Temperature (°C): "));
  Serial.println(DHT.getTemperature());

  Serial.print(F("Dew Point (°C): "));
  Serial.println(DHT.dewPoint());

  delay(5000);
}
//
// END OF FILE
//
