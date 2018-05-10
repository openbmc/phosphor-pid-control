#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>

#include "sensors/sensor.hpp"

/*
 * The SensorManager holds all sensors across all zones.
 */
class SensorManager
{
  public:
    SensorManager() :
        _passiveListeningBus(std::move(sdbusplus::bus::new_default())),
        _hostSensorBus(std::move(sdbusplus::bus::new_default()))
    {
        // Create a manger for the sensor root because we own it.
        static constexpr auto SensorRoot = "/xyz/openbmc_project/extsensors";
        sdbusplus::server::manager::manager(_hostSensorBus, SensorRoot);
    }

    /*
     * Add a Sensor to the Manager.
     */
    void addSensor(std::string type, std::string name,
                   std::unique_ptr<Sensor> sensor)
    {
        _sensorMap[name] = std::move(sensor);

        auto entry = _sensorTypeList.find(type);
        if (entry == _sensorTypeList.end())
        {
            _sensorTypeList[type] = {};
        }

        _sensorTypeList[type].push_back(name);
    }

    // TODO(venture): Should implement read/write by name.
    std::unique_ptr<Sensor>& getSensor(std::string name)
    {
        return _sensorMap.at(name);
    }

    sdbusplus::bus::bus& getPassiveBus(void)
    {
        return _passiveListeningBus;
    }

    sdbusplus::bus::bus& getHostBus(void)
    {
        return _hostSensorBus;
    }

  private:
    std::map<std::string, std::unique_ptr<Sensor>> _sensorMap;
    std::map<std::string, std::vector<std::string>> _sensorTypeList;

    sdbusplus::bus::bus _passiveListeningBus;
    sdbusplus::bus::bus _hostSensorBus;
};

std::shared_ptr<SensorManager>
BuildSensors(std::map<std::string, struct sensor>& Config);

std::shared_ptr<SensorManager> BuildSensorsFromConfig(std::string& path);
