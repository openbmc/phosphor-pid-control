#include "swampd_test.hpp"

#include <chrono>

#include <gtest/gtest.h>

using namespace std::literals::chrono_literals;
static const auto secondsToRunTest = 12s;
static const auto uSecondsToRunTest =
    std::chrono::microseconds(secondsToRunTest);

static constexpr char swampdConfPath[] = "conf.json";
static constexpr char swampdConfPathTwoSensors[] = "conf2.json";

static constexpr char defaultTempServiceName[] =
    "xyz.openbmc_project.Hwmon-1644477290.Hwmon0";
static constexpr char defaultFanServiceName[] =
    "xyz.openbmc_project.Hwmon-1644477290.Hwmon1";
static constexpr char defaultTempObjPath[] =
    "/xyz/openbmc_project/sensors/temperature/sensor0";
static constexpr char defaultFanObjPath[] =
    "/xyz/openbmc_project/sensors/fan_tach/fan0";

static constexpr char defaultTemp2ObjPath[] =
    "/xyz/openbmc_project/sensors/temperature/sensor1";
static constexpr char defaultFan2ObjPath[] =
    "/xyz/openbmc_project/sensors/fan_tach/fan1";

static const SensorProperties defaultTempVals{
    {"Value", 18.4},   {"MaxValue", 100.4},
    {"MinValue", 0.4}, {"Unit", MockValue::Unit::DegreesC},
    {"Scale", 0.0},
};

static const SensorProperties defaultFanSensorVals{
    {"Value", 2500.4},
    {"MaxValue", 11000.4},
    {"MinValue", 13.4},
    {"Unit", MockValue::Unit::RPMS},
    {"Scale", 0.0}};

static const FanPwmProperties defaultFanPwmVals{{"Target", uint64_t(70)}};

static const SensorProperties defaultTemp2Vals{
    {"Value", 22.4},   {"MaxValue", 100.0},
    {"MinValue", 0.0}, {"Unit", MockValue::Unit::DegreesC},
    {"Scale", 0.0},
};

static const SensorProperties defaultFan2SensorVals{
    {"Value", 1501.0},
    {"MaxValue", 11000.0},
    {"MinValue", 13.0},
    {"Unit", MockValue::Unit::RPMS},
    {"Scale", 0.0}};

static const FanPwmProperties defaultFan2PwmVals{{"Target", uint64_t(21)}};

TEST(SwampDFanControl, SingleSensorTest)
{
    SwampdTest test(swampdConfPath);
    test.initiateFanService(defaultFanServiceName, uSecondsToRunTest);
    test.initiateTempService(defaultTempServiceName, uSecondsToRunTest);

    test.addNewTempObject(defaultTempObjPath, defaultTempVals);
    test.addNewFanObject(defaultFanObjPath, defaultFanSensorVals,
                         defaultFanPwmVals);
    test.buildDefaultZone();

    test.startFanService();
    test.startTempService();

    test.expectPropsChangedSignal(defaultFanObjPath, MockFanPwm::interface)
        ->atLeast(1);

    test.startSwampd();

    test.runFor(uSecondsToRunTest);

    EXPECT_GE(test.fanService->getMainObject().getMockBase()->target(), 100);
}

TEST(SwampDFanControl, MultiSensorsTest)
{
    SwampdTest test(swampdConfPathTwoSensors);
    test.initiateFanService(defaultFanServiceName, uSecondsToRunTest);
    test.initiateTempService(defaultTempServiceName, uSecondsToRunTest);

    test.addNewTempObject(defaultTempObjPath, defaultTempVals);
    test.addNewFanObject(defaultFanObjPath, defaultFanSensorVals,
                         defaultFanPwmVals);

    test.addNewTempObject(defaultTemp2ObjPath, defaultTemp2Vals);
    test.addNewFanObject(defaultFan2ObjPath, defaultFan2SensorVals,
                         defaultFan2PwmVals);

    test.buildDefaultZone();

    test.startFanService();
    test.startTempService();

    test.expectPropsChangedSignal(defaultFanObjPath, MockFanPwm::interface)
        ->atLeast(1);
    test.expectPropsChangedSignal(defaultFan2ObjPath, MockFanPwm::interface)
        ->atLeast(1);

    test.startSwampd();

    test.runFor(uSecondsToRunTest);

    EXPECT_GE(
        test.fanService->getObject(defaultFanObjPath).getMockBase()->target(),
        80);
    EXPECT_GE(
        test.fanService->getObject(defaultFan2ObjPath).getMockBase()->target(),
        80);

    EXPECT_LE(
        test.fanService->getObject(defaultFanObjPath).getMockBase()->target(),
        100);
    EXPECT_LE(
        test.fanService->getObject(defaultFan2ObjPath).getMockBase()->target(),
        100);
}
