#pragma once

#include "sensor.hpp"
#include "xyz/openbmc_project/Sensor/Value/server.hpp"

#include <memory>
#include <mutex>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>

template <typename... T>
using ServerObject = typename sdbusplus::server::object::object<T...>;

using ValueInterface = sdbusplus::xyz::openbmc_project::Sensor::server::Value;
using ValueObject = ServerObject<ValueInterface>;

/*
 * HostSensor object is a Sensor derivative that also implements a ValueObject,
 * which comes from the dbus as an object that implements Sensor.Value.
 */
class HostSensor : public Sensor, public ValueObject
{
  public:
    static std::unique_ptr<Sensor> CreateTemp(const std::string& name,
                                              int64_t timeout,
                                              sdbusplus::bus::bus& bus,
                                              const char* objPath, bool defer);

    HostSensor(const std::string& name, int64_t timeout,
               sdbusplus::bus::bus& bus, const char* objPath, bool defer) :
        Sensor(name, timeout),
        ValueObject(bus, objPath, defer)
    {
    }

    /* Note: This must be int64_t because it's from ValueObject */
    int64_t value(int64_t value) override;

    ReadReturn read(void) override;
    void write(double value) override;

  private:
    /*
     * _lock will be used to make sure _updated & _value are updated
     * together.
     */
    std::mutex _lock;
    std::chrono::high_resolution_clock::time_point _updated;
    double _value = 0;
};
