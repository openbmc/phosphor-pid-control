#pragma once

#include "interfaces.hpp"

#include <chrono>
#include <string>

/**
 * Abstract base class for all sensors.
 */
class Sensor
{
  public:
    Sensor(const std::string& name, int64_t timeout) :
        _name(name), _timeout(timeout)
    {
    }

    virtual ~Sensor()
    {
    }

    virtual ReadReturn read(void) = 0;
    virtual void write(double value) = 0;
    virtual bool getFailed(void)
    {
        return false;
    };

    std::string getName(void) const
    {
        return _name;
    }

    /* Returns the configurable timeout period
     * for this sensor in seconds (undecorated).
     */
    int64_t getTimeout(void) const
    {
        return _timeout;
    }

  private:
    std::string _name;
    int64_t _timeout;
};
