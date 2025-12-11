#pragma once

#include "interfaces.hpp"

#include <cstdint>
#include <string>

namespace pid_control
{

/**
 * Abstract base class for all sensors.
 */
class Sensor
{
  public:
    /**
     * Given a sensor's type, return the default timeout value.
     * A timeout of 0 means there isn't a timeout for this sensor.
     * By default a fan sensor isn't checked for a timeout, whereas
     * any of sensor is meant to be sampled once per second.  By default.
     *
     * @param[in] type - the sensor type (e.g. fan)
     * @return the default timeout for that type (in seconds).
     */
    static int64_t getDefaultTimeout(const std::string& type)
    {
        return (type == "fan") ? 0 : 2;
    }

    Sensor(const std::string& name, int64_t timeout,
           bool ignoreFailIfHostOff = false) :
        _name(name), _timeout(timeout),
        _ignoreFailIfHostOff(ignoreFailIfHostOff)
    {}

    virtual ~Sensor() = default;

    virtual ReadReturn read(void) = 0;
    virtual void write(double value) = 0;

    virtual void write(double value, bool force, int64_t* written)
    {
        (void)force;
        (void)written;
        return write(value);
    }

    virtual bool getFailed(void)
    {
        return false;
    };

    virtual FailureReason getFailReason(void)
    {
        return FailureReason::Unknown;
    }

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

    bool getIgnoreFailIfHostOff(void) const
    {
        return _ignoreFailIfHostOff;
    }

  private:
    std::string _name;
    int64_t _timeout;
    bool _ignoreFailIfHostOff;
};

} // namespace pid_control
