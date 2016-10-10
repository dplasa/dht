#include "dht.h"

// Various named constants.
#define DHTLIB_DHT11_WAKEUP         18
#define DHTLIB_DHT_WAKEUP           1
#define DHTLIB_DHT11_LEADING_ZEROS  1
#define DHTLIB_DHT_LEADING_ZEROS    6

#ifndef F_CPU
#define DHTLIB_TIMEOUT 1000  // should be approx. clock/40000
#else
#define DHTLIB_TIMEOUT (F_CPU/40000)
#endif

#include <Arduino.h>
#include <Wire.h>

dht::ReadStatus dht::read() 
{
    ReadStatus res = OK;

    // READ VALUES
    if (DHT11 == model)
        res = _readSensor(DHTLIB_DHT11_WAKEUP, DHTLIB_DHT11_LEADING_ZEROS);
    else
        res = _readSensor(DHTLIB_DHT_WAKEUP, DHTLIB_DHT_LEADING_ZEROS);
    // CHECK
    if (OK == res)
        res = _checksum();
    // STORE
    if (OK == res)
        res = _storeData();
    return res;
}

// dewPoint function NOAA
// reference (1) : http://wahiduddin.net/calc/density_algorithms.htm
// reference (2) : http://www.colorado.edu/geography/weather_station/Geog_site/about.htm
//

#include <math.h>
float dht::dewPoint() const
{
  // sloppy but good approximation for 0 ... +70 °C with max. deviation less than 0.25 °C
  const float a = 17.271;
  float b = 2377; //237.7;
  float tempF = temperature;
  float tmp = (a * tempF) / (b + tempF) + log(humidity*0.001);
  b *= 0.1f;
  float Td = (b * tmp) / (a - tmp);
  return Td;
  /* PRECISE calulcation

  float RATIO = 373.15 / (273.15 + temperature * 0.1f);
  float RHS = -7.90298 * (RATIO - 1);
  RHS += 5.02808 * log10(RATIO);
  RHS += -1.3816e-7 * (pow(10, (11.344 * (1 - 1 / RATIO ))) - 1) ;
  RHS += 8.1328e-3 * (pow(10, (-3.49149 * (RATIO - 1))) - 1) ;
  RHS += 3.0057149; //log10(1013.246);

  // factor -4 is to adjust units - Vapor Pressure SVP * humidity
  float VP = pow(10, RHS - 4) * humidity;

  // (2) DEWPOINT = F(Vapor Pressure)
  //float T = log(VP/0.61078);   // temp var
  float T = log(VP) + 0.49301845;   // temp var
  return (241.88 * T) / (17.558 - T);
  */
}

//////// PRIVATE
dht::ReadStatus dht::_checksum()
{
    uint8_t sum = bits[0] + bits[1] + bits[2] + bits[3];
    if (bits[4] != sum) 
        return ERROR_CHECKSUM;
    return OK;
}

dht::ReadStatus dht1wire::_storeData()
{
    if (DHT11 == model)
    {     
        // these bits are always zero, masking them reduces errors.
        bits[0] &= 0x7F;
        bits[2] &= 0x7F;

        // CONVERT AND STORE
        humidity = bits[0];     // bits[1] == 0;
        humidity *= 10;
        temperature = bits[2];  // bits[3] == 0;
        temperature *= 10;
    }
    else if (DHT12 == model)
    {
        return ERROR_UNKNOWN;
    }
    else
    {
        // these bits are always zero, masking them reduces errors.
        bits[0] &= 0x03;
        bits[2] &= 0x83;
        
        // CONVERT AND STORE
        humidity = (bits[0]*256 + bits[1]);
        temperature = ((bits[2] & 0x7F)*256 + bits[3]);
        if (bits[2] & 0x80)  // negative temperature
            temperature = -temperature;
    }
    return OK;
}

dht::ReadStatus dht12::_storeData()
{
    if (DHT12 == model)
    {     
        // CONVERT AND STORE
        temperature = bits[2]*10 +bits[3];
        humidity = bits[0]*10 + bits[1];
        return OK;
    }
    return ERROR_UNKNOWN;
}

dht::ReadStatus dht12::_readSensor(uint8_t, uint8_t)
{
    Wire.beginTransmission(pin);
    Wire.write(0);
    if (Wire.endTransmission() != 0) 
        return ERROR_CONNECT;  
    Wire.requestFrom(pin, (uint8_t) 5);
    for (uint8_t i = 0; i<5; ++i)
        bits[i] = Wire.read();

    delay(1);
    if (Wire.available()!=0) 
        return ERROR_TIMEOUT;
    return OK;
}

dht::ReadStatus dht1wire::_readSensor(uint8_t wakeupDelay, uint8_t leadingZeroBits)
{
    // INIT BUFFERVAR TO RECEIVE DATA
    uint8_t mask = 128;
    uint8_t idx = 0;

    uint8_t data = 0;
    uint8_t state = LOW;
    uint8_t pstate = LOW;
    uint16_t zeroLoop = DHTLIB_TIMEOUT;
    uint16_t delta = 0;

    leadingZeroBits = 40 - leadingZeroBits; // reverse counting...

    // replace digitalRead() with Direct Port Reads.
    // reduces footprint ~100 bytes => portability issue?
    // direct port read is about 3x faster
    uint8_t bit = digitalPinToBitMask(pin);
    uint8_t port = digitalPinToPort(pin);
    volatile uint8_t *PIR = portInputRegister(port);

    // START READOUT
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW); // T-be
    delay(wakeupDelay);
    digitalWrite(pin, HIGH); // T-go
    pinMode(pin, INPUT_PULLUP);

    // wait for pin to drop low
    uint16_t loopCount = DHTLIB_TIMEOUT * 2;  // 200uSec max
    //while(digitalRead(pin) == HIGH)
    while ((*PIR & bit) != LOW )
    {
        if (--loopCount == 0) return ERROR_CONNECT;
    }

    // GET ACKNOWLEDGE or TIMEOUT
    loopCount = DHTLIB_TIMEOUT;
    // while(digitalRead(pin) == LOW)
    while ((*PIR & bit) == LOW )  // T-rel
    {
        if (--loopCount == 0) return ERROR_ACK_L;
    }

    loopCount = DHTLIB_TIMEOUT;
    // while(digitalRead(pin) == HIGH)
    while ((*PIR & bit) != LOW )  // T-reh
    {
        if (--loopCount == 0) return ERROR_ACK_H;
    }

    loopCount = DHTLIB_TIMEOUT;

    // READ THE OUTPUT - 40 BITS => 5 BYTES
    for (uint8_t i = 40; i != 0; )
    {
       // WAIT FOR FALLING EDGE
        state = (*PIR & bit);
        if (state == LOW && pstate != LOW)
        {
            if (i > leadingZeroBits) // DHT22 first 6 bits are all zero !!   DHT11 only 1
            {
                zeroLoop = min(zeroLoop, loopCount);
                delta = (DHTLIB_TIMEOUT - zeroLoop)/4;
            }
            else if ( loopCount <= (zeroLoop - delta) ) // long -> one
            {
                data |= mask;
            }
            mask >>= 1;
            if (mask == 0)   // next byte
            {
                mask = 128;
                bits[idx] = data;
                idx++;
                data = 0;
            }
            // next bit
            --i;

            // reset timeout flag
            loopCount = DHTLIB_TIMEOUT;
        }
        pstate = state;
        // Check timeout
        if (--loopCount == 0)
        {
            return ERROR_TIMEOUT;
        }

    }
    // not sure if this is needed at all!
    // pinMode(pin, OUTPUT);
    // digitalWrite(pin, HIGH);
    return OK;
}


dht12::dht12(uint8_t id) : 
    dht(id, dht::DHT12)
{
    Wire.begin();
}

