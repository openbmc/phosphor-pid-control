#include "swampd_test.hpp"

#include "common/integration_test.hpp"
#include "common/services/fan_server.hpp"
#include "common/services/sensor_server.hpp"

#include <sdbusplus/test/integration/private_bus.hpp>

#include <cassert>
#include <memory>
#include <string>

SwampdTest::SwampdTest(const char* swampdConfPath) :
    IntegrationTest(), swampdDaemon(swampdConfPath)
{}

SwampdTest::SwampdTest(const char* swampdConfPath, const char* logsDirPath) :
    IntegrationTest(), swampdDaemon(swampdConfPath, logsDirPath)
{}

void SwampdTest::startTempService(std::string serviceName,
                                  SdBusDuration microsecondsToRun)
{
    assert(isMapperxStarted());
    tempService = std::make_shared<SensorService>(getBus(), serviceName,
                                                  microsecondsToRun);
}

void SwampdTest::startFanService(std::string serviceName)
{
    assert(isMapperxStarted());
    fanService = std::make_shared<FanService>(getBus(), serviceName);
}

bool SwampdTest::isFanServiceStarted()
{
    return fanService != nullptr && fanService->isStarted();
}

bool SwampdTest::isTempServiceStarted()
{
    return tempService != nullptr && tempService->isStarted();
}

void SwampdTest::startSwampd(int warmUpMilisec)
{
    assert(isFanServiceStarted());
    assert(isTempServiceStarted());
    swampdDaemon.start(warmUpMilisec);
}

void SwampdTest::addNewFanObject(
    const std::string& path, const SensorProperties& fanSensorVals,
    const FanPwmProperties& fanPwmVals,
    FanSensorValChangeHandler fanSensorValChangeHandler,
    FanPwmChangeHandler fanPwmChangeHandler)
{
    assert(isFanServiceStarted());

    fanService->addFan(path, fanSensorVals, fanPwmVals,
                       fanSensorValChangeHandler, fanPwmChangeHandler);
}

void SwampdTest::addNewFanObject(
    const std::string& path, const SensorProperties& fanSensorVals,
    const FanPwmProperties& fanPwmVals,
    FanSensorValChangeHandler fanSensorValChangeHandler)
{
    assert(isFanServiceStarted());
    fanService->addFan(path, fanSensorVals, fanPwmVals,
                       fanSensorValChangeHandler);
}

void SwampdTest::addNewFanObject(const std::string& path,
                                 const SensorProperties& fanSensorVals,
                                 const FanPwmProperties& fanPwmVals)
{
    addNewFanObject(path, fanSensorVals, fanPwmVals,
                    defaultOnFanSensorValChange);
}

void SwampdTest::addNewTempObject(const std::string& path,
                                  const SensorProperties& tempSensorVals,
                                  double changeRatePercentage)
{
    assert(isTempServiceStarted());
    tempService->addSensor(path, tempSensorVals, changeRatePercentage);
}

void SwampdTest::buildDefaultZone()
{
    assert(isFanServiceStarted());
    assert(isTempServiceStarted());
    for (auto const& [fobjPath, fobj] : fanService->objectRepo)
    {
        zones[fobjPath] = {};
        for (auto const& [tobjPath, tobj] : tempService->objectRepo)
        {
            zones[fobjPath].push_back(tobjPath);
        }
    }
}
