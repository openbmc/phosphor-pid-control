#pragma once

#include "interfaces.hpp"
#include "sensor.hpp"

#include <memory>
#include <string>

namespace pid_control
{

/*
 * A Sensor that can use any reader or writer you provide.
 */
class PluggableSensor : public Sensor
{
  public:
    PluggableSensor(const std::string& name, int64_t timeout,
                    std::unique_ptr<ReadInterface> reader,
                    std::unique_ptr<WriteInterface> writer) :
        Sensor(name, timeout), _reader(std::move(reader)),
        _writer(std::move(writer))
    {}

    ReadReturn read(void) override;
    void write(double value) override;
    void write(double value, bool force, int64_t* written) override;
    bool getFailed(void) override;
    std::string getFailReason(void) override;

  private:
    std::unique_ptr<ReadInterface> _reader;
    std::unique_ptr<WriteInterface> _writer;
};

} // namespace pid_control
