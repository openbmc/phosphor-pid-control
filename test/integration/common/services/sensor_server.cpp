#include "common/services/sensor_server.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/sdbus.hpp>
#include <sdbusplus/test/integration/mock_object.hpp>
#include <sdbusplus/test/integration/mock_service.hpp>
#include <xyz/openbmc_project/Sensor/Value/mock_server.hpp>

#include <iostream>
#include <map>
#include <memory>
#include <string>

#include "gmock/gmock.h"

using sdbusplus::test::integration::MockObject;
using sdbusplus::test::integration::MockService;
using sdbusplus::xyz::openbmc_project::Sensor::server::MockValue;
using ::testing::NiceMock;

using SensorProperties = std::map<std::string, MockValue::PropertiesVariant>;

SensorObject::SensorObject(sdbusplus::bus::bus& bus, const std::string& path,
                           const SensorProperties& vals,
                           double changeRatePercentage) :
    MockObject(bus, path),
    MockSensorObjectBase(bus, path.c_str()), changeRate(changeRatePercentage)
{
    for (auto const& [key, val] : vals)
    {
        setPropertyByName(key, val);
    }
}

void SensorObject::toggleSensorVal()
{
    changeSensorVal(changeRate);
}

void SensorObject::changeSensorVal(double percentage)
{
    double nextSensorVal = value() * (1 + percentage);
    value(nextSensorVal);
}

SensorObject::~SensorObject()
{
    std::cout << getPath() << " final properties: {" << std::endl;
    std::cout << "value"
              << " : " << value() << std::endl;
    std::cout << "}" << std::endl;
}

SensorService::SensorService(std::shared_ptr<sdbusplus::bus::bus> bus,
                             std::string name,
                             SdBusDuration microsecondsToRun) :
    MockServiceActive(bus, name, microsecondsToRun)
{
    started = true;
}

SensorService::~SensorService()
{
    started = false;
}

void SensorService::addSensor(const std::string& objectPath,
                              const SensorProperties& vals,
                              double changeRatePercentage)
{
    addObject(std::make_shared<NiceMock<SensorObject>>(
        getBus(), objectPath, vals, changeRatePercentage));
}

static SensorObject& getSensorObject(std::shared_ptr<MockObject> obj)
{
    return *(std::dynamic_pointer_cast<SensorObject>(obj));
}

SensorObject& SensorService::getObject(std::string path)
{
    return getSensorObject(objectRepo[path]);
}

SensorObject& SensorService::getMainObject()
{
    return getObject(mainObjectPath);
}

void SensorService::proceed()
{
    for (auto const& [path, object] : objectRepo)
    {
        getSensorObject(object).toggleSensorVal();
    }
    MockServiceActive::proceed();
}

void SensorService::changeSensorVal(double percentage, std::string objectPath)
{
    SensorObject& sobj = getObject(objectPath);
    sobj.changeSensorVal(percentage);
}

void SensorService::changeMainSensorVal(double percentage)
{
    changeSensorVal(percentage, mainObjectPath);
}
