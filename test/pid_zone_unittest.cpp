#include "conf.hpp"
#include "failsafeloggers/builder.hpp"
#include "interfaces.hpp"
#include "pid/ec/pid.hpp"
#include "pid/pidcontroller.hpp"
#include "pid/zone.hpp"
#include "pid/zone_interface.hpp"
#include "sensors/manager.hpp"
#include "sensors/sensor.hpp"
#include "test/controller_mock.hpp"
#include "test/helpers.hpp"
#include "test/sensor_mock.hpp"

#include <systemd/sd-bus.h>

#include <sdbusplus/test/sdbus_mock.hpp>
#include <xyz/openbmc_project/Control/Mode/common.hpp>
#include <xyz/openbmc_project/Debug/Pid/ThermalPower/common.hpp>
#include <xyz/openbmc_project/Debug/Pid/Zone/common.hpp>
#include <xyz/openbmc_project/Object/Enable/common.hpp>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace pid_control
{
namespace
{

using ::testing::_;
using ::testing::IsNull;
using ::testing::Return;
using ::testing::StrEq;

using ControlMode = sdbusplus::common::xyz::openbmc_project::control::Mode;
using DebugPidZone = sdbusplus::common::xyz::openbmc_project::Debug::Pid::Zone;
using DebugThermalPower =
    sdbusplus::common::xyz::openbmc_project::Debug::Pid::ThermalPower;
using ObjectEnable = sdbusplus::common::xyz::openbmc_project::object::Enable;

namespace
{

TEST(PidZoneConstructorTest, BoringConstructorTest)
{
    // Build a PID Zone.

    sdbusplus::SdBusMock sdbus_mock_passive, sdbus_mock_host, sdbus_mock_mode,
        sdbus_mock_enable;
    auto bus_mock_passive = sdbusplus::get_mocked_new(&sdbus_mock_passive);
    auto bus_mock_host = sdbusplus::get_mocked_new(&sdbus_mock_host);
    auto bus_mock_mode = sdbusplus::get_mocked_new(&sdbus_mock_mode);
    auto bus_mock_enable = sdbusplus::get_mocked_new(&sdbus_mock_enable);

    EXPECT_CALL(sdbus_mock_host,
                sd_bus_add_object_manager(
                    IsNull(), _, StrEq("/xyz/openbmc_project/extsensors")))
        .WillOnce(Return(0));

    SensorManager m(bus_mock_passive, bus_mock_host);

    bool defer = true;
    bool accSetPoint = false;
    const char* objPath = "/path/";
    int64_t zone = 1;
    double minThermalOutput = 1000.0;
    double failSafePercent = 100;
    conf::CycleTime cycleTime;

    double d;
    std::vector<std::string> properties;
    SetupDbusObject(&sdbus_mock_mode, defer, objPath, ControlMode::interface,
                    properties, &d);
    SetupDbusObject(&sdbus_mock_mode, defer, objPath, DebugPidZone::interface,
                    properties, &d);

    std::string sensorname = "temp1";
    std::string pidsensorpath =
        "/xyz/openbmc_project/settings/fanctrl/zone1/" + sensorname;

    double de;
    std::vector<std::string> propertiesenable;
    SetupDbusObject(&sdbus_mock_enable, defer, pidsensorpath.c_str(),
                    ObjectEnable::interface, propertiesenable, &de);

    DbusPidZone p(zone, minThermalOutput, failSafePercent, cycleTime, m,
                  bus_mock_mode, objPath, defer, accSetPoint);
    // Success.
}

} // namespace

class PidZoneTest : public ::testing::Test
{
  protected:
    PidZoneTest() :
        properties(), sdbus_mock_passive(), sdbus_mock_host(),
        sdbus_mock_mode(), sdbus_mock_enable()
    {
        EXPECT_CALL(sdbus_mock_host,
                    sd_bus_add_object_manager(
                        IsNull(), _, StrEq("/xyz/openbmc_project/extsensors")))
            .WillOnce(Return(0));

        auto bus_mock_passive = sdbusplus::get_mocked_new(&sdbus_mock_passive);
        auto bus_mock_host = sdbusplus::get_mocked_new(&sdbus_mock_host);
        auto bus_mock_mode = sdbusplus::get_mocked_new(&sdbus_mock_mode);
        auto bus_mock_enable = sdbusplus::get_mocked_new(&sdbus_mock_enable);

        mgr = SensorManager(bus_mock_passive, bus_mock_host);

        SetupDbusObject(&sdbus_mock_mode, defer, objPath,
                        ControlMode::interface, properties, &property_index);
        SetupDbusObject(&sdbus_mock_mode, defer, objPath,
                        DebugPidZone::interface, properties, &property_index);

        SetupDbusObject(&sdbus_mock_enable, defer, pidsensorpath.c_str(),
                        ObjectEnable::interface, propertiesenable,
                        &propertyenable_index);

        zone = std::make_unique<DbusPidZone>(
            zoneId, minThermalOutput, failSafePercent, cycleTime, *mgr,
            bus_mock_mode, objPath, defer, accSetPoint);
    }

    // unused
    double property_index{};
    std::vector<std::string> properties;
    double propertyenable_index;
    std::vector<std::string> propertiesenable;

    sdbusplus::SdBusMock sdbus_mock_passive;
    sdbusplus::SdBusMock sdbus_mock_host;
    sdbusplus::SdBusMock sdbus_mock_mode;
    sdbusplus::SdBusMock sdbus_mock_enable;
    int64_t zoneId = 1;
    double minThermalOutput = 1000.0;
    double failSafePercent = 100;
    double setpoint = 50.0;
    bool defer = true;
    bool accSetPoint = false;
    const char* objPath = "/path/";
    std::optional<SensorManager> mgr;
    conf::CycleTime cycleTime;

    std::string sensorname = "temp1";
    std::string sensorType = "temp";
    std::string pidsensorpath =
        "/xyz/openbmc_project/settings/fanctrl/zone1/" + sensorname;

    std::unique_ptr<DbusPidZone> zone;
};

TEST_F(PidZoneTest, GetZoneId_ReturnsExpected)
{
    // Verifies the zoneId returned is what we expect.

    EXPECT_EQ(zoneId, zone->getZoneID());
}

TEST_F(PidZoneTest, GetAndSetManualModeTest_BehavesAsExpected)
{
    // Verifies that the zone starts in manual mode.  Verifies that one can set
    // the mode.
    EXPECT_FALSE(zone->getManualMode());

    zone->setManualMode(true);
    EXPECT_TRUE(zone->getManualMode());
}

TEST_F(PidZoneTest, AddPidControlProcessGetAndSetEnableTest_BehavesAsExpected)
{
    // Verifies that the zone starts in enable mode.  Verifies that one can set
    // enable the mode.
    auto bus_mock_enable = sdbusplus::get_mocked_new(&sdbus_mock_enable);

    EXPECT_CALL(sdbus_mock_mode, sd_bus_emit_properties_changed_strv(
                                     IsNull(), StrEq(pidsensorpath.c_str()),
                                     StrEq(ObjectEnable::interface), NotNull()))
        .Times(::testing::AnyNumber())
        .WillOnce(Invoke(
            [&]([[maybe_unused]] sd_bus* bus, [[maybe_unused]] const char* path,
                [[maybe_unused]] const char* interface, const char** names) {
                EXPECT_STREQ("Enable", names[0]);
                return 0;
            }));

    zone->addPidControlProcess(sensorname, sensorType, setpoint,
                               bus_mock_enable, pidsensorpath.c_str(), defer);
    EXPECT_TRUE(zone->isPidProcessEnabled(sensorname));
}

TEST_F(PidZoneTest, SetManualMode_RedundantWritesEnabledOnceAfterManualMode)
{
    // Tests adding a fan PID controller to the zone, and verifies it's
    // touched during processing.

    std::unique_ptr<PIDController> tpid =
        std::make_unique<ControllerMock>("fan1", zone.get());
    ControllerMock* tmock = reinterpret_cast<ControllerMock*>(tpid.get());

    // Access the internal pid configuration to clear it out (unrelated to the
    // test).
    [[maybe_unused]] ec::pid_info_t* info = tpid->getPIDInfo();

    zone->addFanPID(std::move(tpid));

    EXPECT_CALL(*tmock, setptProc()).WillOnce(Return(10.0));
    EXPECT_CALL(*tmock, inputProc()).WillOnce(Return(11.0));
    EXPECT_CALL(*tmock, outputProc(_));

    // while zone is in auto mode redundant writes should be disabled
    EXPECT_FALSE(zone->getRedundantWrite());

    // but switching from manual to auto enables a single redundant write
    zone->setManualMode(true);
    zone->setManualMode(false);
    EXPECT_TRUE(zone->getRedundantWrite());

    // after one iteration of a pid loop redundant write should be cleared
    zone->processFans();
    EXPECT_FALSE(zone->getRedundantWrite());
}

TEST_F(PidZoneTest, RpmSetPoints_AddMaxClear_BehaveAsExpected)
{
    // Tests addSetPoint, clearSetPoints, determineMaxSetPointRequest
    // and getMinThermalSetPoint.

    // Need to add pid control process for the zone that can enable
    // the process and add the set point.
    auto bus_mock_enable = sdbusplus::get_mocked_new(&sdbus_mock_enable);

    EXPECT_CALL(sdbus_mock_mode, sd_bus_emit_properties_changed_strv(
                                     IsNull(), StrEq(pidsensorpath.c_str()),
                                     StrEq(ObjectEnable::interface), NotNull()))
        .Times(::testing::AnyNumber())
        .WillOnce(Invoke(
            [&]([[maybe_unused]] sd_bus* bus, [[maybe_unused]] const char* path,
                [[maybe_unused]] const char* interface, const char** names) {
                EXPECT_STREQ("Enable", names[0]);
                return 0;
            }));

    zone->addPidControlProcess(sensorname, sensorType, setpoint,
                               bus_mock_enable, pidsensorpath.c_str(), defer);

    // At least one value must be above the minimum thermal setpoint used in
    // the constructor otherwise it'll choose that value
    std::vector<double> values = {100, 200, 300, 400, 500, 5000};

    for (auto v : values)
    {
        zone->addSetPoint(v, sensorname);
    }

    // This will pull the maximum RPM setpoint request.
    zone->determineMaxSetPointRequest();
    EXPECT_EQ(5000, zone->getMaxSetPointRequest());

    // Clear the values, so it'll choose the minimum thermal setpoint.
    zone->clearSetPoints();

    // This will go through the RPM set point values and grab the maximum.
    zone->determineMaxSetPointRequest();
    EXPECT_EQ(zone->getMinThermalSetPoint(), zone->getMaxSetPointRequest());
}

TEST_F(PidZoneTest, RpmSetPoints_AddBelowMinimum_BehavesAsExpected)
{
    // Tests adding several RPM setpoints, however, they're all lower than the
    // configured minimal thermal setpoint RPM value.

    // Need to add pid control process for the zone that can enable
    // the process and add the set point.
    auto bus_mock_enable = sdbusplus::get_mocked_new(&sdbus_mock_enable);

    EXPECT_CALL(sdbus_mock_mode, sd_bus_emit_properties_changed_strv(
                                     IsNull(), StrEq(pidsensorpath.c_str()),
                                     StrEq(ObjectEnable::interface), NotNull()))
        .Times(::testing::AnyNumber())
        .WillOnce(Invoke(
            [&]([[maybe_unused]] sd_bus* bus, [[maybe_unused]] const char* path,
                [[maybe_unused]] const char* interface, const char** names) {
                EXPECT_STREQ("Enable", names[0]);
                return 0;
            }));

    zone->addPidControlProcess(sensorname, sensorType, setpoint,
                               bus_mock_enable, pidsensorpath.c_str(), defer);

    std::vector<double> values = {100, 200, 300, 400, 500};

    for (auto v : values)
    {
        zone->addSetPoint(v, sensorname);
    }

    // This will pull the maximum RPM setpoint request.
    zone->determineMaxSetPointRequest();

    // Verifies the value returned in the minimal thermal rpm set point.
    EXPECT_EQ(zone->getMinThermalSetPoint(), zone->getMaxSetPointRequest());
}

TEST_F(PidZoneTest, GetFailSafePercent_SingleFailedReturnsExpected)
{
    // Tests when only one sensor failed and the sensor's failsafe duty is zero,
    // and verify that the sensor name is empty and failsafe duty is PID zone's
    // failsafe duty.

    std::vector<std::string> input1 = {"temp1"};
    std::vector<std::string> input2 = {"temp2"};
    std::vector<std::string> input3 = {"temp3"};
    std::vector<double> values = {0, 0, 0};

    zone->addPidFailSafePercent(input1, values[0]);
    zone->addPidFailSafePercent(input2, values[1]);
    zone->addPidFailSafePercent(input3, values[2]);

    zone->markSensorMissing("temp1", "Sensor threshold asserted");

    EXPECT_EQ(failSafePercent, zone->getFailSafePercent());

    std::map<std::string, std::pair<std::string, double>> failSensorList =
        zone->getFailSafeSensors();
    EXPECT_EQ(1U, failSensorList.size());
    EXPECT_EQ("Sensor threshold asserted", failSensorList["temp1"].first);
    EXPECT_EQ(failSafePercent, failSensorList["temp1"].second);
}

TEST_F(PidZoneTest, GetFailSafePercent_MultiFailedReturnsExpected)
{
    // Tests when multi sensor failed, and verify the final failsafe's sensor
    // name and duty as expected.

    std::vector<std::string> input1 = {"temp1"};
    std::vector<std::string> input2 = {"temp2"};
    std::vector<std::string> input3 = {"temp3"};
    std::vector<double> values = {60, 80, 70};

    zone->addPidFailSafePercent(input1, values[0]);
    zone->addPidFailSafePercent(input2, values[1]);
    zone->addPidFailSafePercent(input3, values[2]);

    zone->markSensorMissing("temp1", "Sensor threshold asserted");
    zone->markSensorMissing("temp2", "Sensor reading bad");
    zone->markSensorMissing("temp3", "Sensor unavailable");

    EXPECT_EQ(80, zone->getFailSafePercent());

    std::map<std::string, std::pair<std::string, double>> failSensorList =
        zone->getFailSafeSensors();
    EXPECT_EQ(3U, failSensorList.size());
    EXPECT_EQ("Sensor threshold asserted", failSensorList["temp1"].first);
    EXPECT_EQ(60, failSensorList["temp1"].second);
    EXPECT_EQ("Sensor reading bad", failSensorList["temp2"].first);
    EXPECT_EQ(80, failSensorList["temp2"].second);
    EXPECT_EQ("Sensor unavailable", failSensorList["temp3"].first);
    EXPECT_EQ(70, failSensorList["temp3"].second);
}

TEST_F(PidZoneTest, ThermalInputs_FailsafeToValid_ReadsSensors)
{
    // This test will add a couple thermal inputs, and verify that the zone
    // initializes into failsafe mode, and will read each sensor.

    // Disable failsafe logger for the unit test.
    std::unordered_map<int64_t, std::shared_ptr<ZoneInterface>> empty_zone_map;
    buildFailsafeLoggers(empty_zone_map, 0);

    std::string name1 = "temp1";
    int64_t timeout = 1;

    std::unique_ptr<Sensor> sensor1 =
        std::make_unique<SensorMock>(name1, timeout);
    SensorMock* sensor_ptr1 = reinterpret_cast<SensorMock*>(sensor1.get());

    std::string name2 = "temp2";
    std::unique_ptr<Sensor> sensor2 =
        std::make_unique<SensorMock>(name2, timeout);
    SensorMock* sensor_ptr2 = reinterpret_cast<SensorMock*>(sensor2.get());

    std::string type = "unchecked";
    mgr->addSensor(type, name1, std::move(sensor1));
    EXPECT_EQ(mgr->getSensor(name1), sensor_ptr1);
    mgr->addSensor(type, name2, std::move(sensor2));
    EXPECT_EQ(mgr->getSensor(name2), sensor_ptr2);

    // Now that the sensors exist, add them to the zone.
    zone->addThermalInput(name1, false);
    zone->addThermalInput(name2, false);

    // Initialize Zone
    zone->initializeCache();

    // Verify now in failsafe mode.
    EXPECT_TRUE(zone->getFailSafeMode());

    ReadReturn r1;
    r1.value = 10.0;
    r1.updated = std::chrono::high_resolution_clock::now();
    EXPECT_CALL(*sensor_ptr1, read()).WillOnce(Return(r1));

    ReadReturn r2;
    r2.value = 11.0;
    r2.updated = std::chrono::high_resolution_clock::now();
    EXPECT_CALL(*sensor_ptr2, read()).WillOnce(Return(r2));

    // Read the sensors, this will put the values into the cache.
    zone->updateSensors();

    // We should no longer be in failsafe mode.
    EXPECT_FALSE(zone->getFailSafeMode());

    EXPECT_EQ(r1.value, zone->getCachedValue(name1));
    EXPECT_EQ(r2.value, zone->getCachedValue(name2));
}

TEST_F(PidZoneTest, FanInputTest_VerifiesFanValuesCached)
{
    // This will add a couple fan inputs, and verify the values are cached.

    // Disable failsafe logger for the unit test.
    std::unordered_map<int64_t, std::shared_ptr<ZoneInterface>> empty_zone_map;
    buildFailsafeLoggers(empty_zone_map, 0);

    std::string name1 = "fan1";
    int64_t timeout = 2;

    std::unique_ptr<Sensor> sensor1 =
        std::make_unique<SensorMock>(name1, timeout);
    SensorMock* sensor_ptr1 = reinterpret_cast<SensorMock*>(sensor1.get());

    std::string name2 = "fan2";
    std::unique_ptr<Sensor> sensor2 =
        std::make_unique<SensorMock>(name2, timeout);
    SensorMock* sensor_ptr2 = reinterpret_cast<SensorMock*>(sensor2.get());

    std::string type = "unchecked";
    mgr->addSensor(type, name1, std::move(sensor1));
    EXPECT_EQ(mgr->getSensor(name1), sensor_ptr1);
    mgr->addSensor(type, name2, std::move(sensor2));
    EXPECT_EQ(mgr->getSensor(name2), sensor_ptr2);

    // Now that the sensors exist, add them to the zone.
    zone->addFanInput(name1, false);
    zone->addFanInput(name2, false);

    // Initialize Zone
    zone->initializeCache();

    ReadReturn r1;
    r1.value = 10.0;
    r1.updated = std::chrono::high_resolution_clock::now();
    EXPECT_CALL(*sensor_ptr1, read()).WillOnce(Return(r1));

    ReadReturn r2;
    r2.value = 11.0;
    r2.updated = std::chrono::high_resolution_clock::now();
    EXPECT_CALL(*sensor_ptr2, read()).WillOnce(Return(r2));

    // Method under test will read through each fan sensor for the zone and
    // cache the values.
    zone->updateFanTelemetry();

    EXPECT_EQ(r1.value, zone->getCachedValue(name1));
    EXPECT_EQ(r2.value, zone->getCachedValue(name2));
}

TEST_F(PidZoneTest, ThermalInput_ValueTimeoutEntersFailSafeMode)
{
    // On the second updateSensors call, the updated timestamp will be beyond
    // the timeout limit.

    // Disable failsafe logger for the unit test.
    std::unordered_map<int64_t, std::shared_ptr<ZoneInterface>> empty_zone_map;
    buildFailsafeLoggers(empty_zone_map, 0);

    int64_t timeout = 1;

    std::string name1 = "temp1";
    std::unique_ptr<Sensor> sensor1 =
        std::make_unique<SensorMock>(name1, timeout);
    SensorMock* sensor_ptr1 = reinterpret_cast<SensorMock*>(sensor1.get());

    std::string name2 = "temp2";
    std::unique_ptr<Sensor> sensor2 =
        std::make_unique<SensorMock>(name2, timeout);
    SensorMock* sensor_ptr2 = reinterpret_cast<SensorMock*>(sensor2.get());

    std::string type = "unchecked";
    mgr->addSensor(type, name1, std::move(sensor1));
    EXPECT_EQ(mgr->getSensor(name1), sensor_ptr1);
    mgr->addSensor(type, name2, std::move(sensor2));
    EXPECT_EQ(mgr->getSensor(name2), sensor_ptr2);

    zone->addThermalInput(name1, false);
    zone->addThermalInput(name2, false);

    // Initialize Zone
    zone->initializeCache();

    // Verify now in failsafe mode.
    EXPECT_TRUE(zone->getFailSafeMode());

    ReadReturn r1;
    r1.value = 10.0;
    r1.updated = std::chrono::high_resolution_clock::now();
    EXPECT_CALL(*sensor_ptr1, read()).WillOnce(Return(r1));

    ReadReturn r2;
    r2.value = 11.0;
    r2.updated = std::chrono::high_resolution_clock::now();
    EXPECT_CALL(*sensor_ptr2, read()).WillOnce(Return(r2));

    zone->updateSensors();
    EXPECT_FALSE(zone->getFailSafeMode());

    // Ok, so we're not in failsafe mode, so let's set updated to the past.
    // sensor1 will have an updated field older than its timeout value, but
    // sensor2 will be fine. :D
    r1.updated -= std::chrono::seconds(3);
    r2.updated = std::chrono::high_resolution_clock::now();

    EXPECT_CALL(*sensor_ptr1, read()).WillOnce(Return(r1));
    EXPECT_CALL(*sensor_ptr2, read()).WillOnce(Return(r2));

    // Method under test will read each sensor.  One sensor's value is older
    // than the timeout for that sensor and this triggers failsafe mode.
    zone->updateSensors();
    EXPECT_TRUE(zone->getFailSafeMode());
}

TEST_F(PidZoneTest, ThermalInput_MissingIsAcceptableNoFailSafe)
{
    // This is similar to the above test, but because missingIsAcceptable
    // is set for sensor1, the zone should not enter failsafe mode when
    // only sensor1 goes missing.
    // However, sensor2 going missing should still trigger failsafe mode.

    // Disable failsafe logger for the unit test.
    std::unordered_map<int64_t, std::shared_ptr<ZoneInterface>> empty_zone_map;
    buildFailsafeLoggers(empty_zone_map, 0);

    int64_t timeout = 1;

    std::string name1 = "temp1";
    std::unique_ptr<Sensor> sensor1 =
        std::make_unique<SensorMock>(name1, timeout);
    SensorMock* sensor_ptr1 = reinterpret_cast<SensorMock*>(sensor1.get());

    std::string name2 = "temp2";
    std::unique_ptr<Sensor> sensor2 =
        std::make_unique<SensorMock>(name2, timeout);
    SensorMock* sensor_ptr2 = reinterpret_cast<SensorMock*>(sensor2.get());

    std::string type = "unchecked";
    mgr->addSensor(type, name1, std::move(sensor1));
    EXPECT_EQ(mgr->getSensor(name1), sensor_ptr1);
    mgr->addSensor(type, name2, std::move(sensor2));
    EXPECT_EQ(mgr->getSensor(name2), sensor_ptr2);

    // Only sensor1 has MissingIsAcceptable enabled for it
    zone->addThermalInput(name1, true);
    zone->addThermalInput(name2, false);

    // Initialize Zone
    zone->initializeCache();

    // As sensors are not initialized, zone should be in failsafe mode
    EXPECT_TRUE(zone->getFailSafeMode());

    // r1 not populated here, intentionally, to simulate a sensor that
    // is not available yet, perhaps takes a long time to start up.
    ReadReturn r1;
    EXPECT_CALL(*sensor_ptr1, read()).WillOnce(Return(r1));

    ReadReturn r2;
    r2.value = 11.0;
    r2.updated = std::chrono::high_resolution_clock::now();
    EXPECT_CALL(*sensor_ptr2, read()).WillOnce(Return(r2));

    zone->updateSensors();

    // Only sensor2 has been initialized here. Failsafe should be false,
    // because sensor1 MissingIsAcceptable so it is OK for it to go missing.
    EXPECT_FALSE(zone->getFailSafeMode());

    r1.value = 10.0;
    r1.updated = std::chrono::high_resolution_clock::now();

    EXPECT_CALL(*sensor_ptr1, read()).WillOnce(Return(r1));
    EXPECT_CALL(*sensor_ptr2, read()).WillOnce(Return(r2));
    zone->updateSensors();

    // Both sensors are now properly initialized
    EXPECT_FALSE(zone->getFailSafeMode());

    // Ok, so we're not in failsafe mode, so let's set updated to the past.
    // sensor1 will have an updated field older than its timeout value, but
    // sensor2 will be fine. :D
    r1.updated -= std::chrono::seconds(3);
    r2.updated = std::chrono::high_resolution_clock::now();

    EXPECT_CALL(*sensor_ptr1, read()).WillOnce(Return(r1));
    EXPECT_CALL(*sensor_ptr2, read()).WillOnce(Return(r2));
    zone->updateSensors();

    // MissingIsAcceptable is true for sensor1, so the zone should not be
    // thrown into failsafe mode.
    EXPECT_FALSE(zone->getFailSafeMode());

    // Do the same thing, but for the opposite sensors: r1 is good,
    // but r2 is set to some time in the past.
    r1.updated = std::chrono::high_resolution_clock::now();
    r2.updated -= std::chrono::seconds(3);

    EXPECT_CALL(*sensor_ptr1, read()).WillOnce(Return(r1));
    EXPECT_CALL(*sensor_ptr2, read()).WillOnce(Return(r2));
    zone->updateSensors();

    // Now, the zone should be in failsafe mode, because sensor2 does not
    // have MissingIsAcceptable set true, it is still subject to failsafe.
    EXPECT_TRUE(zone->getFailSafeMode());

    r1.updated = std::chrono::high_resolution_clock::now();
    r2.updated = std::chrono::high_resolution_clock::now();

    EXPECT_CALL(*sensor_ptr1, read()).WillOnce(Return(r1));
    EXPECT_CALL(*sensor_ptr2, read()).WillOnce(Return(r2));
    zone->updateSensors();

    // The failsafe mode should cease, as both sensors are good again.
    EXPECT_FALSE(zone->getFailSafeMode());
}

TEST_F(PidZoneTest, FanInputTest_FailsafeToValid_ReadsSensors)
{
    // This will add a couple fan inputs, and verify the values are cached.

    // Disable failsafe logger for the unit test.
    std::unordered_map<int64_t, std::shared_ptr<ZoneInterface>> empty_zone_map;
    buildFailsafeLoggers(empty_zone_map, 0);

    std::string name1 = "fan1";
    int64_t timeout = 2;

    std::unique_ptr<Sensor> sensor1 =
        std::make_unique<SensorMock>(name1, timeout);
    SensorMock* sensor_ptr1 = reinterpret_cast<SensorMock*>(sensor1.get());

    std::string name2 = "fan2";
    std::unique_ptr<Sensor> sensor2 =
        std::make_unique<SensorMock>(name2, timeout);
    SensorMock* sensor_ptr2 = reinterpret_cast<SensorMock*>(sensor2.get());

    std::string type = "unchecked";
    mgr->addSensor(type, name1, std::move(sensor1));
    EXPECT_EQ(mgr->getSensor(name1), sensor_ptr1);
    mgr->addSensor(type, name2, std::move(sensor2));
    EXPECT_EQ(mgr->getSensor(name2), sensor_ptr2);

    // Now that the sensors exist, add them to the zone.
    zone->addFanInput(name1, false);
    zone->addFanInput(name2, false);

    // Initialize Zone
    zone->initializeCache();

    // Verify now in failsafe mode.
    EXPECT_TRUE(zone->getFailSafeMode());

    ReadReturn r1;
    r1.value = 10.0;
    r1.updated = std::chrono::high_resolution_clock::now();
    EXPECT_CALL(*sensor_ptr1, read()).WillOnce(Return(r1));

    ReadReturn r2;
    r2.value = 11.0;
    r2.updated = std::chrono::high_resolution_clock::now();
    EXPECT_CALL(*sensor_ptr2, read()).WillOnce(Return(r2));

    // Method under test will read through each fan sensor for the zone and
    // cache the values.
    zone->updateFanTelemetry();

    // We should no longer be in failsafe mode.
    EXPECT_FALSE(zone->getFailSafeMode());

    EXPECT_EQ(r1.value, zone->getCachedValue(name1));
    EXPECT_EQ(r2.value, zone->getCachedValue(name2));
}

TEST_F(PidZoneTest, FanInputTest_ValueTimeoutEntersFailSafeMode)
{
    // This will add a couple fan inputs, and verify the values are cached.

    // Disable failsafe logger for the unit test.
    std::unordered_map<int64_t, std::shared_ptr<ZoneInterface>> empty_zone_map;
    buildFailsafeLoggers(empty_zone_map, 0);

    std::string name1 = "fan1";
    int64_t timeout = 2;

    std::unique_ptr<Sensor> sensor1 =
        std::make_unique<SensorMock>(name1, timeout);
    SensorMock* sensor_ptr1 = reinterpret_cast<SensorMock*>(sensor1.get());

    std::string name2 = "fan2";
    std::unique_ptr<Sensor> sensor2 =
        std::make_unique<SensorMock>(name2, timeout);
    SensorMock* sensor_ptr2 = reinterpret_cast<SensorMock*>(sensor2.get());

    std::string type = "unchecked";
    mgr->addSensor(type, name1, std::move(sensor1));
    EXPECT_EQ(mgr->getSensor(name1), sensor_ptr1);
    mgr->addSensor(type, name2, std::move(sensor2));
    EXPECT_EQ(mgr->getSensor(name2), sensor_ptr2);

    // Now that the sensors exist, add them to the zone.
    zone->addFanInput(name1, false);
    zone->addFanInput(name2, false);

    // Initialize Zone
    zone->initializeCache();

    // Verify now in failsafe mode.
    EXPECT_TRUE(zone->getFailSafeMode());

    ReadReturn r1;
    r1.value = 10.0;
    r1.updated = std::chrono::high_resolution_clock::now();
    EXPECT_CALL(*sensor_ptr1, read()).WillOnce(Return(r1));

    ReadReturn r2;
    r2.value = 11.0;
    r2.updated = std::chrono::high_resolution_clock::now();
    EXPECT_CALL(*sensor_ptr2, read()).WillOnce(Return(r2));

    // Method under test will read through each fan sensor for the zone and
    // cache the values.
    zone->updateFanTelemetry();

    // We should no longer be in failsafe mode.
    EXPECT_FALSE(zone->getFailSafeMode());

    r1.updated -= std::chrono::seconds(3);
    r2.updated = std::chrono::high_resolution_clock::now();

    EXPECT_CALL(*sensor_ptr1, read()).WillOnce(Return(r1));
    EXPECT_CALL(*sensor_ptr2, read()).WillOnce(Return(r2));

    zone->updateFanTelemetry();
    EXPECT_TRUE(zone->getFailSafeMode());
}

TEST_F(PidZoneTest, GetSensorTest_ReturnsExpected)
{
    // One can grab a sensor from the manager through the zone.

    // Disable failsafe logger for the unit test.
    std::unordered_map<int64_t, std::shared_ptr<ZoneInterface>> empty_zone_map;
    buildFailsafeLoggers(empty_zone_map, 0);

    int64_t timeout = 1;

    std::string name1 = "temp1";
    std::unique_ptr<Sensor> sensor1 =
        std::make_unique<SensorMock>(name1, timeout);
    SensorMock* sensor_ptr1 = reinterpret_cast<SensorMock*>(sensor1.get());

    std::string type = "unchecked";
    mgr->addSensor(type, name1, std::move(sensor1));
    EXPECT_EQ(mgr->getSensor(name1), sensor_ptr1);

    zone->addThermalInput(name1, false);

    // Verify method under test returns the pointer we expect.
    EXPECT_EQ(mgr->getSensor(name1), zone->getSensor(name1));
}

TEST_F(PidZoneTest, AddThermalPIDTest_VerifiesThermalPIDsProcessed)
{
    // Tests adding a thermal PID controller to the zone, and verifies it's
    // touched during processing.

    std::unique_ptr<PIDController> tpid =
        std::make_unique<ControllerMock>("thermal1", zone.get());
    ControllerMock* tmock = reinterpret_cast<ControllerMock*>(tpid.get());

    // Access the internal pid configuration to clear it out (unrelated to the
    // test).
    [[maybe_unused]] ec::pid_info_t* info = tpid->getPIDInfo();

    zone->addThermalPID(std::move(tpid));

    EXPECT_CALL(*tmock, setptProc()).WillOnce(Return(10.0));
    EXPECT_CALL(*tmock, inputProc()).WillOnce(Return(11.0));
    EXPECT_CALL(*tmock, outputProc(_));

    // Method under test will, for each thermal PID, call setpt, input, and
    // output.
    zone->processThermals();
}

TEST_F(PidZoneTest, AddFanPIDTest_VerifiesFanPIDsProcessed)
{
    // Tests adding a fan PID controller to the zone, and verifies it's
    // touched during processing.

    std::unique_ptr<PIDController> tpid =
        std::make_unique<ControllerMock>("fan1", zone.get());
    ControllerMock* tmock = reinterpret_cast<ControllerMock*>(tpid.get());

    // Access the internal pid configuration to clear it out (unrelated to the
    // test).
    [[maybe_unused]] ec::pid_info_t* info = tpid->getPIDInfo();

    zone->addFanPID(std::move(tpid));

    EXPECT_CALL(*tmock, setptProc()).WillOnce(Return(10.0));
    EXPECT_CALL(*tmock, inputProc()).WillOnce(Return(11.0));
    EXPECT_CALL(*tmock, outputProc(_));

    // Method under test will, for each fan PID, call setpt, input, and output.
    zone->processFans();
}

TEST_F(PidZoneTest, ManualModeDbusTest_VerifySetManualBehavesAsExpected)
{
    // The manual(bool) method is inherited from the dbus mode interface.

    // Verifies that someone doesn't remove the internal call to the dbus
    // object from which we're inheriting.
    EXPECT_CALL(sdbus_mock_mode, sd_bus_emit_properties_changed_strv(
                                     IsNull(), StrEq(objPath),
                                     StrEq(ControlMode::interface), NotNull()))
        .WillOnce(Invoke(
            [&]([[maybe_unused]] sd_bus* bus, [[maybe_unused]] const char* path,
                [[maybe_unused]] const char* interface, const char** names) {
                EXPECT_STREQ("Manual", names[0]);
                return 0;
            }));

    // Method under test will set the manual mode to true and broadcast this
    // change on dbus.
    zone->manual(true);
    EXPECT_TRUE(zone->getManualMode());
}

TEST_F(PidZoneTest, FailsafeDbusTest_VerifiesReturnsExpected)
{
    // This property is implemented by us as read-only, such that trying to
    // write to it will have no effect.

    // Disable failsafe logger for the unit test.
    std::unordered_map<int64_t, std::shared_ptr<ZoneInterface>> empty_zone_map;
    buildFailsafeLoggers(empty_zone_map, 0);

    EXPECT_EQ(zone->failSafe(), zone->getFailSafeMode());
}

} // namespace
} // namespace pid_control
