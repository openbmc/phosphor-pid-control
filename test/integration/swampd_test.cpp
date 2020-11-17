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

void SwampdTest::initiateTempService(std::string serviceName,
                                     SdBusDuration microsecondsToRun)
{
    assert(isMapperxStarted());
    tempService = std::make_shared<SensorService>(serviceName, mockBus,
                                                  microsecondsToRun);
}

void SwampdTest::startTempService()
{
    tempService->start();
}

void SwampdTest::stopTempService()
{
    tempService->stop();
}

void SwampdTest::initiateFanService(std::string serviceName,
                                    SdBusDuration microsecondsToRun)
{
    assert(isMapperxStarted());
    fanService =
        std::make_shared<FanService>(serviceName, mockBus, microsecondsToRun);
}

void SwampdTest::startFanService()
{
    fanService->start();
}

void SwampdTest::stopFanService()
{
    fanService->stop();
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
    swampdDaemon.start(warmUpMilisec);
}

void SwampdTest::addNewFanObject(
    const std::string& path, const SensorProperties& fanSensorVals,
    const FanPwmProperties& fanPwmVals,
    FanSensorValChangeHandler fanSensorValChangeHandler,
    FanPwmChangeHandler fanPwmChangeHandler)
{
    fanService->addFan(path, fanSensorVals, fanPwmVals,
                       fanSensorValChangeHandler, fanPwmChangeHandler);
}

void SwampdTest::addNewFanObject(
    const std::string& path, const SensorProperties& fanSensorVals,
    const FanPwmProperties& fanPwmVals,
    FanSensorValChangeHandler fanSensorValChangeHandler)
{
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
    tempService->addSensor(path, tempSensorVals, changeRatePercentage);
}

void SwampdTest::buildDefaultZone()
{
    for (auto const& [fobjPath, fobj] : fanService->objectRepo)
    {
        zones[fobjPath] = {};
        for (auto const& [tobjPath, tobj] : tempService->objectRepo)
        {
            zones[fobjPath].push_back(tobjPath);
        }
    }
}
