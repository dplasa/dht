#include "dht.h"

// Various named constants.
#define DHTLIB_DHT11_WAKEUP 18
#define DHTLIB_DHT_WAKEUP 1
#define DHTLIB_DHT11_LEADING_ZEROS 1
#define DHTLIB_DHT_LEADING_ZEROS 6

#ifndef F_CPU
#define DHTLIB_TIMEOUT 1000 // should be approx. clock/40000
#else
#define DHTLIB_TIMEOUT (F_CPU / 40000)
#endif

#include <Arduino.h>
#include <Wire.h>


// implement CRC-16/MODBUS, see https://crccalc.com/
// polynom:     0x8005 => reverse 0xa001
// crc init:    0xffff
// reflect-in:  true
// reflect-out: true
// xor-out:     0x0000
#if defined(ARDUINO_ARCH_AVR)
  #include <util/crc16.h>
#else

// we implement it as weak symbol, if already in code by another of our libraries
uint16_t __attribute__((weak)) _crc16_update(uint16_t crc, uint8_t a) 
{
  crc ^= a;
  for (uint8_t i = 0; i < 8; ++i)
  {
    if (crc & 1)
      crc = (crc >> 1) ^ 0xa001;
    else
      crc = (crc >> 1);
  }
  return crc;
}
#endif

//
// Interrupt helper
//
struct InterruptLockHelper
{
    InterruptLockHelper()
    {
        noInterrupts();
    }
    ~InterruptLockHelper()
    {
        interrupts();
    }
};

dht::dhtmodels dht::begin()
{
    return getModel();
}

dht::ReadStatus dht::read()
{
    // read sensor data
    ReadStatus res = _readSensor();

    // check checksum
    if (OK == res)
        res = _checksum();

    // store values
    if (OK == res)
        res = _storeData();
    return res;
}

// dewPoint function NOAA
// reference (1) : http://wahiduddin.net/calc/density_algorithms.htm
// reference (2) : http://www.colorado.edu/geography/weather_station/Geog_site/about.htm

#include <math.h>
RETURNTYPE dht::dewPoint() const
{
    // sloppy but good approximation for 0 ... +70 °C with max. deviation less than 0.25 °C
    const float a = 17.271;
    float b = 2377; //237.7;
    float tempF = temperature;
    float tmp = (a * tempF) / (b + tempF) + log(humidity * 0.001);
    b *= 0.1f;
    float Td = (b * tmp) / (a - tmp);
#if (defined ESP8266)
    return Td;
#else
    return RETURNTYPE(Td * 10);
#endif
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

dht::ReadStatus dht::_checksum()
{
    // this is the default checksum method of the
    // DHT11,22,33,44 and DHT12 sensor:
    // 5th byte is sum of the previous four bytes
    uint8_t sum = (bits[0] + bits[1] + bits[2] + bits[3]) - bits[4];
    if (sum)
        return ERROR_CHECKSUM;
    return OK;
}

// 1wire sensor read
dht::ReadStatus dht1wire::_readSensor()
{
    uint8_t wakeupDelay = DHTLIB_DHT_WAKEUP;
    uint8_t leadingZeroBits = 40 - DHTLIB_DHT_LEADING_ZEROS; // reverse counting...
    if (DHT11 == model)
    {
        wakeupDelay = DHTLIB_DHT11_WAKEUP;
        leadingZeroBits = 40 - DHTLIB_DHT11_LEADING_ZEROS;
    }

    // INIT BUFFERVAR TO RECEIVE DATA
    uint8_t mask = 128;
    uint8_t idx = 0;

    uint8_t data = 0;
    uint8_t state = LOW;
    uint8_t pstate = LOW;
    uint16_t zeroLoop = DHTLIB_TIMEOUT;
    uint16_t delta = 0;

    // replace digitalRead() with Direct Port Reads.
    // reduces footprint ~100 bytes => portability issue?
    // direct port read is about 3x faster
    auto bit = digitalPinToBitMask(pin);
    auto port = digitalPinToPort(pin);
    auto PIR = portInputRegister(port);

    // START READOUT
    // disable interrupts for this...
    InterruptLockHelper __ilh;

    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW); // T-be
    delay(wakeupDelay);
    digitalWrite(pin, HIGH); // T-go
    pinMode(pin, INPUT_PULLUP);

    // wait for pin to drop low
    uint16_t loopCount = DHTLIB_TIMEOUT * 2; // 200uSec max
    //while(digitalRead(pin) == HIGH)
    while ((*PIR & bit) != LOW)
    {
        if (--loopCount == 0)
            return ERROR_CONNECT;
    }

    // GET ACKNOWLEDGE or TIMEOUT
    loopCount = DHTLIB_TIMEOUT;
    // while(digitalRead(pin) == LOW)
    while ((*PIR & bit) == LOW) // T-rel
    {
        if (--loopCount == 0)
            return ERROR_ACK_L;
    }

    loopCount = DHTLIB_TIMEOUT;
    // while(digitalRead(pin) == HIGH)
    while ((*PIR & bit) != LOW) // T-reh
    {
        if (--loopCount == 0)
            return ERROR_ACK_H;
    }

    loopCount = DHTLIB_TIMEOUT;

    // READ THE OUTPUT - 40 BITS => 5 BYTES
    for (uint8_t i = 40; i != 0;)
    {
        // WAIT FOR FALLING EDGE
        state = (*PIR & bit);
        if (state == LOW && pstate != LOW)
        {
            if (i > leadingZeroBits) // DHT22 first 6 bits are all zero !!   DHT11 only 1
            {
                zeroLoop = min(zeroLoop, loopCount);
                delta = (DHTLIB_TIMEOUT - zeroLoop) / 4;
            }
            else if (loopCount <= (zeroLoop - delta)) // long -> one
            {
                data |= mask;
            }
            mask >>= 1;
            if (mask == 0) // next byte
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

dht::ReadStatus dht1wire::_storeData()
{
    if (DHT11 == model)
    {
        // these bits are always zero, masking them reduces errors.
        bits[0] &= 0x7F;
        bits[2] &= 0x7F;

        // CONVERT AND STORE
        humidity = bits[0];    // bits[1] == 0;
        temperature = bits[2]; // bits[3] == 0;
#if (!defined ESP8266)
        humidity *= 10;
        temperature *= 10;
#endif
    }
    else
    {
        // these bits are always zero, masking them reduces errors.
        bits[0] &= 0x03;
        bits[2] &= 0x83;

        // CONVERT AND STORE
        humidity = (bits[0] * 256 + bits[1]);
        temperature = ((bits[2] & 0x7F) * 256 + bits[3]);
        if (bits[2] & 0x80) // negative temperature
            temperature = -temperature;
    }
    return OK;
}

dht::dhtmodels dhti2c::setModel(dhtmodels _model)
{
    id = 0;
    if (_model >= DHT12)
    {
        // both AM2320 and DHT12 use
        id = 0x5c;
    }
    return dht::setModel(_model);
}

dht::dhtmodels dhti2c::begin()
{
    Wire.begin();

    if (model == DHTNONE)
    {
        setModel(DHT12);
    }

    // success? return!
    if (read() == OK)
        return model;

    // if DHT12 failed, try AM2320
    if (model == DHT12)
    {
        setModel(AM2320);
    }
    // if AM2320 failed, try DHT12
    else
    {
        setModel(DHT12);
    }

    // success? return!
    if (read() == OK)
        return model;

    // failure
    return setModel(DHTNONE);
}

dht::ReadStatus dhti2c::_readSensor()
{
    if (getModel() == DHTNONE)
        return ERROR_UNKNOWN;

    // request at least 5 bytes
    uint8_t len = 5;

    // start transmission
    Wire.beginTransmission(id);
    if (getModel() == DHT12)
    {
        Wire.write(0);
    }
    else if (getModel() == AM2320)
    {
        // 1st start of transmission was only Wake up
        Wire.endTransmission();
        delay(1);
        yield();
        // now for real
        Wire.beginTransmission(id);
        Wire.write(0x03); // request
        Wire.write(0x00); // from register 0
        Wire.write(0x04); // 4 bytes for read
        len = 8;          // Read 8 data bytes ( (2 Header)+(2 Hum)+(2 Temp)+(2 CRC16))
        delay(2);         // delay 2ms before read
        yield();
    }
    uint8_t tmp = Wire.endTransmission();
    if (tmp)
        return ERROR_CONNECT;

    // request data
    Wire.requestFrom(id, len);
    for (uint8_t i = 0; i < len; ++i)
        bits[i] = Wire.read();

    delay(1);
        yield();
    if (Wire.available() != 0)
        return ERROR_TIMEOUT;
    return OK;
}

dht::ReadStatus dhti2c::_storeData()
{
    if (getModel() == DHT12)
    {
        // CONVERT AND STORE
        temperature = bits[2] * 10 + bits[3];
        humidity = bits[0] * 10 + bits[1];
    }
    else if (getModel() == AM2320)
    {
        // CONVERT AND STORE
        // bits[4] = MSB (+sign), bits[5] = LSB
        temperature = (((bits[4] & 0x7f) << 8) | bits[5]);
        if (bits[4] & 0x80)
            temperature = -temperature;

        // bits[2] = MSB, bits[3] = LSB
        humidity = ((bits[2] << 8) + bits[3]);
    }
#if (defined ESP8266)
    temperature /= 10.0;
    humidity /= 10.0;
#endif
    return OK;
}

extern uint16_t _crc16_update(uint16_t crc, uint8_t a);
dht::ReadStatus dhti2c::_checksum()
{
    if (getModel() == DHT12)
        return dht::_checksum();
    else if (getModel() == AM2320)
    {
        // AM2320 uses CRC-16/MODBUS, see https://crccalc.com/
        // polynom:     0x8005 => reverse 0xa001
        // crc init:    0xffff
        // reflect-in:  true
        // reflect-out: true
        // xor-out:     0x0000
        // CRC over all sensor data should result in 0
        uint16_t crc = 0xffff;
        for (uint8_t i = 0; i<sizeof(bits); ++i)
            crc = _crc16_update(crc, bits[i]);
        return (0 == crc ? OK : ERROR_CHECKSUM);
    }
    return ERROR_CHECKSUM;
}

dht::ReadStatus dhtdummy::_readSensor()
{
    memcpy(&bits, &dummydata, 4);
    // fake checksum
    bits[4] = bits[0] + bits[1] + bits[2] + bits[3];

    // fake a broken checksum
    if (bits[3] & 0x80)
        bits[4] ^= 0xff;

    // fake also connection error if bit is set
    return (bits[3] & 0x40) ? ERROR_CONNECT : OK;
}

dht::ReadStatus dhtdummy::_storeData()
{
    temperature = bits[0] + bits[1] * 10;
    humidity = bits[2] + (bits[3] & 0x3f) * 10;
    return OK;
}
