#pragma once

#include <chrono>

namespace pid_control
{

struct ReadReturn
{
    double value;
    std::chrono::high_resolution_clock::time_point updated;

    bool operator==(const ReadReturn& rhs) const
    {
        return (this->value == rhs.value && this->updated == rhs.updated);
    }
};

/*
 * A ReadInterface is a plug-in for the PluggableSensor and anyone implementing
 * this basically is providing a way to read a sensor.
 */
class ReadInterface
{
  public:
    ReadInterface()
    {}

    virtual ~ReadInterface()
    {}

    virtual ReadReturn read(void) = 0;

    virtual bool getFailed(void) const
    {
        return false;
    }
};

/*
 * A WriteInterface is a plug-in for the PluggableSensor and anyone implementing
 * this basically is providing a way to write a sensor.
 */
class WriteInterface
{
  public:
    WriteInterface(int64_t min, int64_t max) : _min(min), _max(max)
    {}

    virtual ~WriteInterface()
    {}

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
