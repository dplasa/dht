/*
 * dht.h
 *
 * Version 0.61
 *
 * DHTxx Temperature and humidity sensor library for Arduino.
 *
 * See http://playground.arduino.cc/main/DHT11Lib 
 * See http://playground.arduino.cc/main/DHTLib 
 * See https://www.mischianti.org/2019/01/01/dht12-library-en/
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
 *      * Merged in code from DHTlib for DHT22,33,44 ...
 *      * Merged in code from https://github.com/Bobadas/DHT12_library_Arduino
 *        for DHT12 (I2C communication) support
 *                                              - Version 0.60 (27/04/2020)
 *      * Merged in code from https://www.mischianti.org/2019/01/01/dht12-library-en/
 *        and from https://github.com/EngDial/AM2320/
 *        for DHT12/AM2320 support
 *                                              - Version 0.61 (02/06/2020)
 *      * float data types on esp8266
 *      * auto-detect DHT12/AM2320 feature
 */

#ifndef dht_xx_h
#define dht_xx_h

#include <stdint.h>

#if (defined ESP8266)
#define RETURNTYPE float           // use float on esp8266
#else
#define RETURNTYPE int16_t         // use integer Value*10 as return
#endif

/*
 * dht
 *
 * A base class that modes a DHTxx or AMD2320 humidity/temperature sensors
 * 
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

    // An enumeration for the model of the sensor.
    enum dhtmodels {
	    DHTNONE,
        DHTDUMMY,
        DHT11, 
        DHT22, 
        DHT33, 
        DHT44,
        DHT12,
        AM2320
    };

    /*
     * dht
     *
     * Constructs a new object that communicates with a DHTxx sensor
     * over the given pin: DHT11, 22, 33 and 44 can be used that way
     * using the derived class dht1wire
     * 
     */
    dht() = default;
    virtual ~dht() = default;

    /*
     * begin
     *
     * Call before reading from the sensor. If possible, checks for the 
     * connected sensor type and returns the detected model. 
     * 
     */
    virtual dhtmodels begin();

    /*
     * read
     *
     * Get the humidity and temperature from the sensor.
     * Returns OK if the update was successful, ERROR_TIMEOUT if it times out
     * waiting for a response from the sensor, or ERROR_CHECKSUM if the
     * calculated checksum doesn't match the checksum provided by the sensor.
     * 
     */
    ReadStatus read();

    /*
     * getHumidity
     *
     * Gets the last read relative humidity percentage.
     */
    inline RETURNTYPE getHumidity() const {
        return humidity;
    }

    /*
     * getTemperature
     *
     * Gets the last read temperature value in degrees Celsius.
     */
    inline RETURNTYPE getTemperature() const {
        return temperature;
    }

    /*
     * dewPoint
     *
     * Calculate the dew point temperatur Td from current temperatur and humidity
     */
    RETURNTYPE dewPoint() const;

    /*
     * get Model type
     *
     */
    inline dhtmodels getModel() const {
        return model;
    }
    /*
     * set Model type
     *
     */
    virtual dhtmodels setModel(dhtmodels _model)
    {
        return (model = _model);
    }

protected:
    // The last read humidity value
    RETURNTYPE humidity;

    // The last read temperature value
    RETURNTYPE temperature;

    // the sensor type
    dhtmodels model;

    // raw bits
    uint8_t bits[8];

    virtual ReadStatus _checksum();

private:
    virtual ReadStatus _readSensor() = 0;
    virtual ReadStatus _storeData() = 0;
};

/*
 * dht1pin
 *
 * A DHTxx humidity/temperature sensor over 1-wire, xx=11,22,33,44
 * 
 */

class dht1wire : public dht
{
public:
    /*
     * Constructs a new object that communicates with a DHTxx sensor
     * over the given pin: DHT11, 22, 33 and 44 can be used that way.
     */
    dht1wire(dhtmodels _model, uint8_t _pin) : pin(_pin) 
    {
        setModel(_model);
    }
private:
    // The pin over which we communicate with the sensor
    uint8_t pin;
    
    virtual ReadStatus _readSensor();
    virtual ReadStatus _storeData();
};

/*
 * dhti2c
 *
 * A DHT12 or AM2320 humidity/temperature sensor over I2C
 * 
 */

class dhti2c : public dht
{
public:
    /*
     * Constructs a new object that communicates with a i2c sensor
     * By default set type NONE to auto-detect by begin()
     */
    dhti2c(dhtmodels _model = DHTNONE)
    {
        setModel(_model);
    }

    virtual dhtmodels begin();
    virtual dhtmodels setModel(dhtmodels _model);
private:
    // If we use the I2C Bus the Device ID (default 0x5c DHT12 / 0xb8 AM2320)
    uint8_t id;
    virtual ReadStatus _readSensor();
    virtual ReadStatus _storeData();
    virtual ReadStatus _checksum();
};

/*
 * a DHT dummy
 *
 */

class dhtdummy : public dht
{
public:
   dhtdummy(uint32_t &_dummydata) : dummydata(_dummydata) 
   {
       setModel(DHTDUMMY);
   }
private:
    uint32_t &dummydata;
    virtual ReadStatus _readSensor();
    virtual ReadStatus _storeData();
};

#endif
