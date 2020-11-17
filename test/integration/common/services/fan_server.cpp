#include "common/services/fan_server.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/test/integration/mock_object.hpp>
#include <sdbusplus/test/integration/mock_service.hpp>
#include <xyz/openbmc_project/Control/FanPwm/mock_server.hpp>
#include <xyz/openbmc_project/Sensor/Value/mock_server.hpp>

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>

#include "gmock/gmock.h"

using sdbusplus::test::integration::MockObject;
using sdbusplus::test::integration::MockService;
using sdbusplus::xyz::openbmc_project::Control::server::MockFanPwm;
using sdbusplus::xyz::openbmc_project::Sensor::server::MockValue;

using ::testing::NiceMock;

static auto buildFanTargetPropsMatch(const std::string& path)
{
    return sdbusplus::bus::match::rules::interface(
               "org.freedesktop.DBus.Properties") +
           sdbusplus::bus::match::rules::path(path) +
           sdbusplus::bus::match::rules::member("PropertiesChanged") +
           sdbusplus::bus::match::rules::argN(0, MockFanPwm::interface);
}

static auto buildFanSensorPropsMatch(const std::string& path)
{
    return sdbusplus::bus::match::rules::interface(
               "org.freedesktop.DBus.Properties") +
           sdbusplus::bus::match::rules::path(path) +
           sdbusplus::bus::match::rules::member("PropertiesChanged") +
           sdbusplus::bus::match::rules::argN(0, MockValue::interface);
}

FanObject::FanObject(const std::string& path,
                     const SensorProperties& sensorVals,
                     const FanPwmProperties& fanPwmVals,
                     FanSensorValChangeHandler fanSensorValChangeHanlder,
                     FanPwmChangeHandler fanPWMChangeHandler) :
    MockObject(path),
    initialSensorVals(sensorVals), initialFanPwmVals(fanPwmVals),
    onFanSensorValChange(fanSensorValChangeHanlder),
    onFanPWMChange(fanPWMChangeHandler)
{}

FanObject::FanObject(const std::string& path,
                     const SensorProperties& sensorVals,
                     const FanPwmProperties& fanPwmVals,
                     FanSensorValChangeHandler fanSensorValChangeHanlder) :
    FanObject(path, sensorVals, fanPwmVals, fanSensorValChangeHanlder,
              std::bind(&FanObject::defaultOnFanPWMChange, this,
                        std::placeholders::_1, std::placeholders::_2,
                        std::placeholders::_3, std::placeholders::_4))
{}

FanObject::~FanObject()
{
    std::cout << getPath() << " final properties: {" << std::endl;
    std::cout << "value"
              << " : " << mockBase->value() << std::endl;
    std::cout << "target"
              << " : " << mockBase->target() << std::endl;
    std::cout << "}" << std::endl;
}

void FanObject::start(sdbusplus::bus::bus& bus)
{
    MockObject::start(bus);

    mockBase =
        std::make_shared<NiceMock<MockFanObjectBase>>(bus, getPath().c_str());
    for (auto const& [key, val] : initialSensorVals)
    {
        mockBase->MockValue::setPropertyByName(key, val);
        if (key == "Value")
        {
            prevValue = std::get<double>(val);
        }
    }

    for (auto const& [key, val] : initialFanPwmVals)
    {
        mockBase->MockFanPwm::setPropertyByName(key, val);
        prevTarget = std::get<0>(val);
    }
    sensorValueRange = mockBase->maxValue() - mockBase->minValue();

    fanTargetPropsMatch = std::make_shared<sdbusplus::bus::match::match>(
        bus, buildFanTargetPropsMatch(getPath()),
        std::move([this](sdbusplus::message::message& message) {
            this->catchFanTargetChange(message);
        }));

    fanSensorPropsMatch = std::make_shared<sdbusplus::bus::match::match>(
        bus, buildFanSensorPropsMatch(getPath()),
        std::move([this](sdbusplus::message::message& message) {
            this->catchFanSensorValChange(message);
        }));
}

std::shared_ptr<MockFanObjectBase> FanObject::getMockBase()
{
    return mockBase;
}

void FanObject::defaultOnFanPWMChange(std::string path, double pTarget,
                                      double newTarget, double targetRange)
{
    if (targetRange == 0)
    {
        targetRange = pTarget;
    }
    double percentage = (newTarget - pTarget) / targetRange;
    std::cout << "target for " << path << " is changed from " << pTarget
              << " to " << newTarget << std::endl;
    changeFanSensorVal(percentage);
}

void FanObject::catchFanTargetChange(sdbusplus::message::message& message)
{
    std::string objectName;
    FanPwmProperties values;
    message.read(objectName, values);
    uint64_t newTarget = std::get<0>(values["Target"]);
    onFanPWMChange(getPath(), static_cast<double>(prevTarget),
                   static_cast<double>(newTarget),
                   static_cast<double>(maxPWMTarget - minPWMTarget));
    prevTarget = newTarget;
}

void FanObject::catchFanSensorValChange(sdbusplus::message::message& message)
{
    std::string objectName;
    SensorProperties values;
    message.read(objectName, values);
    if (values.find("Value") != values.end())
    {
        double newValue = std::get<double>(values["Value"]);
        onFanSensorValChange(getPath(), prevValue, newValue, sensorValueRange);
        prevValue = newValue;
    }
}

void FanObject::changeFanSensorVal(double percentage)
{
    double nextFanSensorValue = mockBase->value() * (1 + percentage);
    mockBase->value(nextFanSensorValue);
}

FanService::FanService(std::string name, std::shared_ptr<PrivateBus> privateBus,
                       SdBusDuration microsecondsToRun) :
    MockService(name, privateBus, microsecondsToRun)
{}

void FanService::addFan(const std::string& objectPath,
                        const SensorProperties& sensorVals,
                        const FanPwmProperties& fanPwmVals,
                        FanSensorValChangeHandler onFanSensorValChange,
                        FanPwmChangeHandler onFanPWMChange)
{
    addObject(std::make_shared<NiceMock<FanObject>>(
        objectPath, sensorVals, fanPwmVals, onFanSensorValChange,
        onFanPWMChange));
}

void FanService::addFan(const std::string& objectPath,
                        const SensorProperties& sensorVals,
                        const FanPwmProperties& fanPwmVals,
                        FanSensorValChangeHandler onFanSensorValChange)
{
    addObject(std::make_shared<NiceMock<FanObject>>(
        objectPath, sensorVals, fanPwmVals, onFanSensorValChange));
}

FanObject& FanService::getObject(std::string path)
{
    return *(std::dynamic_pointer_cast<FanObject>(objectRepo[path]));
}

FanObject& FanService::getMainObject()
{
    return getObject(mainObjectPath);
}

void FanService::changeSensorVal(double percentage)
{
    changeSensorVal(percentage, mainObjectPath);
}

void FanService::changeSensorVal(double percentage, std::string objectPath)
{
    FanObject& fobj = getObject(objectPath);
    fobj.changeFanSensorVal(percentage);
}
