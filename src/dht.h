/*
 * dht.h
 *
 * Version 0.52
 *
 * DHTxx Temperature and humidity sensor library for Arduino.
 *
 * See http://playground.arduino.cc/main/DHT11Lib 
 * See http://playground.arduino.cc/main/DHTLib 
 *
 * License:   GPL v3 (http://www.gnu.org/licenses/gpl.html)
 * Datasheet: http://www.micro4you.com/files/sensor/DHT11.pdf
 *
 * Modification History:
 *   - George Hadjikyriacou                     - Version 0.1 (??)
 *   - SimKard                                  - Version 0.2 (24/11/2010)
 *   - Rob Tillaart                             - Version 0.3 (28/03/2011)
 *       * Added comments
 *       * Removed all non-DHT11 specific code
 *       * Added References
 *   - Rob Tillaart                             - Version 0.4 (17/03/2012)
 *       * Added Arduino 1.0 support
 *   - Rob Tillaart                             - Version 0.4.1 (19/05/2012)
 *       * Added error codes
 *   - Andy Dalton                              - Version 0.5 (13/09/2013)
 *       * Replaced magic numbers with named constants
 *       * Factored out repeated code into a helper method.
 *       * Made pin a parameter to the constructor instead of a parameter
 *         to read()
 *       * Changed the error codes to an enumeration
 *       * Update the temp/humidity only if the checksum is correct
 *       * Added more comments
 *   - Daniel Plasa                             - Version 0.52 (13/09/2016)
 *      * Merged in a dew point function
 *      * Merged in  code from DHTlib for DHT22,33,44 ...
 *      * Merged in code from https://github.com/Bobadas/DHT12_library_Arduino
 *        for DHT12 (I2C communication) support
*/

#ifndef dht_xx_h
#define dht_xx_h

#include <stdint.h>

/*
 * dht
 *
 * A base class that modes a DHTxx humidity/temperature sensor.
 */
class dht {
public:
    // An enumeration modeling the read status of the sensor.
    enum ReadStatus {
        OK,
        ERROR_CHECKSUM,
        ERROR_TIMEOUT,
        ERROR_CONNECT,
        ERROR_ACK_L,
        ERROR_ACK_H,
        ERROR_UNKNOWN,
    };

    // An enumeration modeling the model of the sensor.
    enum dhtmodels {
        DHT11, 
        DHT22, 
        DHT33, 
        DHT44,
        DHT12,
    };

    /*
     * dht
     *
     * Constructs a new object that communicates with a DHTxx sensor
     * over the given pin: DHT11, 22, 33 and 44 can be used that way.
     */
    dht(uint8_t _Pin, dhtmodels _Model) : 
      humidity(-1), temperature(-1), pin(_Pin), model(_Model) 
    {}

    /*
     * read
     *
     * Update the humidity and temperature of this object from the sensor.
     * Returns OK if the update was successful, ERROR_TIMEOUT if it times out
     * waiting for a response from the sensor, or ERROR_CHECKSUM if the
     * calculated checksum doesn't match the checksum provided by the sensor.
     */
    ReadStatus read();

    /*
     * getHumidity
     *
     * Gets the last read relative humidity percentage.
     */
    inline int getHumidity() const {
        return this->humidity;
    }

    /*
     * getTemperature
     *
     * Gets the last read temperature value in degrees Celsius.
     */
    inline int getTemperature() const {
        return this->temperature;
    }

    /*
     * dewPoint
     *
     * Calculate the dew point temperatur Td from current temperatur and humidity
     */
    float dewPoint() const;

protected:
    // The last read humidity value
    int humidity;

    // The last read temperature value
    int temperature;

    // The pin over which we communicate with the sensor
    // If sensor is a DHT12 we use the I2C Bus and pin will hold the Device ID (default 0x5c)
    uint8_t pin;
    
    // the sensor type
    dhtmodels model;

    // raw data
    uint8_t bits[5];

private:
    // private read and store function
    virtual ReadStatus _readSensor(uint8_t wakeupDelay, uint8_t leadingZeroBits) = 0;
    virtual ReadStatus _storeData() = 0;
    virtual ReadStatus _checksum();
};

/*
 * dht1pin
 *
 * A DHTxx humidity/temperature sensor over 1-wire, xx=11,22,33,44
 */

class dht1wire : public dht
{
public:
    /*
     * Constructs a new object that communicates with a DHTxx sensor
     * over the given pin: DHT11, 22, 33 and 44 can be used that way.
     */
    dht1wire(uint8_t _pin, dhtmodels _model) : dht(_pin, _model) {}
private:
    virtual ReadStatus _readSensor(uint8_t wakeupDelay, uint8_t leadingZeroBits);
    virtual ReadStatus _storeData();
};


/*
 * dht12
 *
 * A DHT12 humidity/temperature sensor over I2C
 */

class dht12 : public dht
{
public:
    /*
     * Constructs a new object that communicates with a DHT12 sensor
     * By default the I2C bus adress is 0x5c
     */
    dht12(uint8_t _id = 0x5c);
private:
    virtual ReadStatus _readSensor(uint8_t, uint8_t);
    virtual ReadStatus _storeData();
};


#endif
