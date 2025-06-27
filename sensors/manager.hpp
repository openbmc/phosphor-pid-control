#pragma once

#include "sensors/sensor.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace pid_control
{

/*
 * The SensorManager holds all sensors across all zones.
 */
class SensorManager
{
  public:
    SensorManager(sdbusplus::bus_t& pass, sdbusplus::bus_t& host) :
        _passiveListeningBus(pass), _hostSensorBus(host)
    {
        // manager gets its interface from the bus. :D
        sdbusplus::server::manager_t(_hostSensorBus, SensorRoot);
    }

    SensorManager() = delete;
    ~SensorManager() = default;
    SensorManager(const SensorManager&) = delete;
    SensorManager& operator=(const SensorManager&) = delete;
    SensorManager(SensorManager&&) = default;
    SensorManager& operator=(SensorManager&&) = default;

    /*
     * Add a Sensor to the Manager.
     */
    void addSensor(const std::string& type, const std::string& name,
                   std::unique_ptr<Sensor> sensor);

    // TODO(venture): Should implement read/write by name.
    Sensor* getSensor(const std::string& name) const
    {
        return _sensorMap.at(name).get();
    }

    sdbusplus::bus_t& getPassiveBus(void)
    {
        return _passiveListeningBus;
    }

    sdbusplus::bus_t& getHostBus(void)
    {
        return _hostSensorBus;
    }

  private:
    std::map<std::string, std::unique_ptr<Sensor>> _sensorMap;
    std::map<std::string, std::vector<std::string>> _sensorTypeList;

    std::reference_wrapper<sdbusplus::bus_t> _passiveListeningBus;
    std::reference_wrapper<sdbusplus::bus_t> _hostSensorBus;

    static constexpr auto SensorRoot = "/xyz/openbmc_project/extsensors";
};

} // namespace pid_control
