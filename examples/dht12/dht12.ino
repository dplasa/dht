
Skip to content
This repository

    Pull requests
    Issues
    Gist

    @dplasa

1
0

    0

dplasa/dht
Code
Issues 0
Pull requests 0
Projects 0
Wiki
Pulse
Graphs
Settings
dht/examples/dhtxx/dhtxx.ino
4056f86 an hour ago
@dplasa dplasa Create dhtxx.ino
60 lines (51 sloc) 1.19 KB
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

  delay(1000);
}
//
// END OF FILE
//

    Contact GitHub API Training Shop Blog About 

    © 2016 GitHub, Inc. Terms Privacy Security Status Help 

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
  uint32_t b = micros();
  dht::ReadStatus chk = DHT.read();
  uint32_t e = micros();

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
