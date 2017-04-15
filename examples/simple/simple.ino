//
// Simple sensor readout example
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
  DHT.read();

  Serial.print(F("Humidity (%): "));
  Serial.println(DHT.getHumidity()/10);

  Serial.print(F("Temperature (°C): "));
  Serial.println(DHT.getTemperature()/10);

  delay(5000);
}
//
// END OF FILE
//
