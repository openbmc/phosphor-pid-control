#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/test/integration/mock_object.hpp>
#include <sdbusplus/test/integration/mock_service.hpp>
#include <sdbusplus/test/integration/private_bus.hpp>
#include <xyz/openbmc_project/Control/FanPwm/mock_server.hpp>
#include <xyz/openbmc_project/Sensor/Value/mock_server.hpp>

#include <functional>
#include <map>
#include <memory>
#include <string>

using sdbusplus::SdBusDuration;
using sdbusplus::test::integration::MockObject;
using sdbusplus::test::integration::MockService;
using sdbusplus::test::integration::PrivateBus;
using sdbusplus::xyz::openbmc_project::Control::server::MockFanPwm;
using sdbusplus::xyz::openbmc_project::Sensor::server::MockValue;

using SensorProperties = std::map<std::string, MockValue::PropertiesVariant>;
using FanPwmProperties = std::map<std::string, MockFanPwm::PropertiesVariant>;
using MockFanObjectBase = sdbusplus::server::object_t<MockValue, MockFanPwm>;
using FanSensorValChangeHandler =
    std::function<void(std::string, double, double, double)>;
using FanPwmChangeHandler =
    std::function<void(std::string, double, double, double)>;

class FanObject : public MockObject
{
  public:
    FanObject(const std::string& path, const SensorProperties& sensorVals,
              const FanPwmProperties& fanPwmVals,
              FanSensorValChangeHandler fanSensorValChangeHanlder,
              FanPwmChangeHandler fanPWMChangeHandler);

    FanObject(const std::string& path, const SensorProperties& sensorVals,
              const FanPwmProperties& fanPwmVals,
              FanSensorValChangeHandler fanSensorValChangeHanlder);

    ~FanObject();

    void start(sdbusplus::bus::bus& bus) override;

    std::shared_ptr<MockFanObjectBase> getMockBase();

    void changeFanSensorVal(double percentage);

  private:
    void catchFanTargetChange(sdbusplus::message::message& message);

    void catchFanSensorValChange(sdbusplus::message::message& message);

    void defaultOnFanPWMChange(std::string path, double pTarget,
                               double newTarget, double targetRange);

    const SensorProperties& initialSensorVals;
    const FanPwmProperties& initialFanPwmVals;

    FanSensorValChangeHandler onFanSensorValChange;
    FanPwmChangeHandler onFanPWMChange;

    std::shared_ptr<sdbusplus::bus::match::match> fanTargetPropsMatch;
    std::shared_ptr<sdbusplus::bus::match::match> fanSensorPropsMatch;
    std::shared_ptr<MockFanObjectBase> mockBase;
    uint64_t prevTarget;
    double prevValue;
    double sensorValueRange;
    static const uint64_t maxPWMTarget = 255;
    static const uint64_t minPWMTarget = 0;
};

class FanService : public MockService
{
  public:
    FanService(std::string name, std::shared_ptr<PrivateBus> privateBus,
               SdBusDuration microsecondsToRun);

    void addFan(const std::string& objectPath,
                const SensorProperties& sensorVals,
                const FanPwmProperties& fanPwmVals,
                FanSensorValChangeHandler onFanSensorValChange,
                FanPwmChangeHandler onFanPWMChange);

    void addFan(const std::string& objectPath,
                const SensorProperties& sensorVals,
                const FanPwmProperties& fanPwmVals,
                FanSensorValChangeHandler onFanSensorValChange);

    FanObject& getObject(std::string path);

    FanObject& getMainObject();

    void changeSensorVal(double percentage);

    void changeSensorVal(double percentage, std::string objectPath);
};
