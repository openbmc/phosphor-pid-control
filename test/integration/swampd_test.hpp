#pragma once

#include "common/integration_test.hpp"
#include "common/services/fan_server.hpp"
#include "common/services/sensor_server.hpp"

#include <sdbusplus/test/integration/daemon_manager.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

using openbmc::test::integration::IntegrationTest;
using sdbusplus::test::integration::Daemon;

class SwampdDaemon : public Daemon
{
  public:
    SwampdDaemon(const SwampdDaemon&) = delete;
    SwampdDaemon& operator=(const SwampdDaemon&) = delete;
    SwampdDaemon(SwampdDaemon&&) = delete;
    SwampdDaemon& operator=(SwampdDaemon&&) = delete;

    SwampdDaemon(const char* confPath = "conf.json") :
        Daemon({defaultSwampdPath, confArgName, confPath}){};

    SwampdDaemon(const char* confPath, const char* logsDirPath) :
        SwampdDaemon(confPath, logsDirPath, defaultSwampdPath){};

    SwampdDaemon(const char* confPath, const char* logsDirPath,
                 const char* swampdPath) :
        Daemon(
            {swampdPath, confArgName, confPath, logDirArgName, logsDirPath}){};

  private:
    static constexpr char confArgName[] = "-c";
    static constexpr char logDirArgName[] = "-l";
    static constexpr char defaultSwampdPath[] = "swampd";
};

class SwampdTest : public IntegrationTest
{
  public:
    SwampdTest(const char* swampdConfPath);

    SwampdTest(const char* swampdConfPath, const char* logsDirPath);

    void startTempService(std::string serviceName,
                          SdBusDuration microsecondsToRun);

    void startFanService(std::string serviceName);

    void startSwampd(int warmUpMilisec = 2000);

    void addNewFanObject(const std::string& path,
                         const SensorProperties& fanSensorVals,
                         const FanPwmProperties& fanPwmVals,
                         FanSensorValChangeHandler fanSensorValChangeHandler,
                         FanPwmChangeHandler fanPwmChangeHandler);

    void addNewFanObject(const std::string& path,
                         const SensorProperties& fanSensorVals,
                         const FanPwmProperties& fanPwmVals,
                         FanSensorValChangeHandler fanSensorValChangeHandler);

    void addNewFanObject(const std::string& path,
                         const SensorProperties& fanSensorVals,
                         const FanPwmProperties& fanPwmVals);

    void addNewTempObject(const std::string& path,
                          const SensorProperties& tempSensorVals,
                          double changeRatePercentage = 0.001);

    void buildDefaultZone();

    FanSensorValChangeHandler defaultOnFanSensorValChange =
        [this](std::string fanObjectPath, double oldValue, double newValue,
               double range) {
            if (range == 0)
            {
                range = oldValue;
            }
            double change = -1.0 * ((newValue - oldValue) / range);
            for (auto const& tempObj : zones[fanObjectPath])
            {
                this->tempService->changeSensorVal(change, tempObj);
            }
        };

    std::shared_ptr<FanService> fanService;
    std::shared_ptr<SensorService> tempService;

  private:
    bool isFanServiceStarted();
    bool isTempServiceStarted();

    SwampdDaemon swampdDaemon;
    std::unordered_map<std::string, std::vector<std::string>> zones;
};
