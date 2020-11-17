#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/test/integration/mock_object.hpp>
#include <sdbusplus/test/integration/mock_service.hpp>
#include <xyz/openbmc_project/Sensor/Value/mock_server.hpp>

#include <map>
#include <memory>
#include <string>

using sdbusplus::SdBusDuration;
using sdbusplus::test::integration::MockObject;
using sdbusplus::test::integration::MockServiceActive;
using sdbusplus::xyz::openbmc_project::Sensor::server::MockValue;

using MockSensorObjectBase = sdbusplus::server::object_t<MockValue>;
using SensorProperties = std::map<std::string, MockValue::PropertiesVariant>;

class SensorObject : public MockObject, public MockSensorObjectBase
{
  public:
    SensorObject(sdbusplus::bus::bus& bus, const std::string& path,
                 const SensorProperties& vals,
                 double changeRatePercentage = 0.001);

    void changeSensorVal(double percentage);

    void toggleSensorVal();

    virtual ~SensorObject();

  private:
    double changeRate;
};

class SensorService : public MockServiceActive
{
  public:
    SensorService(std::shared_ptr<sdbusplus::bus::bus> bus, std::string name,
                  SdBusDuration microsecondsToRun);

    ~SensorService();

    void addSensor(const std::string& objectPath, const SensorProperties& vals,
                   double changeRatePercentage = 0.001);

    SensorObject& getObject(std::string path);

    SensorObject& getMainObject();

    void changeSensorVal(double percentage, std::string objectPath);

    void changeMainSensorVal(double percentage);

  protected:
    void proceed() override;
};
