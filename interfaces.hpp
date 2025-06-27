#pragma once

#include <chrono>
#include <cstdint>
#include <limits>
#include <string>

namespace pid_control
{

struct ReadReturn
{
    double value = std::numeric_limits<double>::quiet_NaN();
    std::chrono::high_resolution_clock::time_point updated;
    double unscaled = value;

    bool operator==(const ReadReturn& rhs) const
    {
        return ((this->value == rhs.value) && (this->updated == rhs.updated) &&
                (this->unscaled == rhs.unscaled));
    }
};

struct ValueCacheEntry
{
    // This is normalized to (0.0, 1.0) range, using configured min and max
    double scaled;

    // This is the raw value, as received from the input/output sensors
    double unscaled;
};

/*
 * A ReadInterface is a plug-in for the PluggableSensor and anyone implementing
 * this basically is providing a way to read a sensor.
 */
class ReadInterface
{
  public:
    ReadInterface() = default;

    virtual ~ReadInterface() = default;

    virtual ReadReturn read(void) = 0;

    virtual bool getFailed(void) const
    {
        return false;
    }

    virtual std::string getFailReason(void) const
    {
        return "Unimplemented";
    }
};

/*
 * A WriteInterface is a plug-in for the PluggableSensor and anyone implementing
 * this basically is providing a way to write a sensor.
 */
class WriteInterface
{
  public:
    WriteInterface(int64_t min, int64_t max) : _min(min), _max(max) {}

    virtual ~WriteInterface() = default;

    virtual void write(double value) = 0;

    /*
     * A wrapper around write(), with additional parameters.
     * force = true to perform redundant write, even if raw value unchanged.
     * written = non-null to be filled in with the actual raw value written.
     */
    virtual void write(double value, bool force, int64_t* written)
    {
        (void)force;
        (void)written;
        return write(value);
    }

    /*
     * All WriteInterfaces have min/max available in case they want to error
     * check.
     */
    int64_t getMin(void)
    {
        return _min;
    }
    int64_t getMax(void)
    {
        return _max;
    }

  private:
    int64_t _min;
    int64_t _max;
};

} // namespace pid_control
