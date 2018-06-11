#include "pid/zone.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sdbusplus/test/sdbus_mock.hpp>
#include <vector>

#include "sensors/manager.hpp"
#include "test/helpers.hpp"

using ::testing::_;
using ::testing::IsNull;
using ::testing::Return;
using ::testing::StrEq;

static std::string modeInterface = "xyz.openbmc_project.Control.Mode";

namespace {

TEST(PidZoneConstructorTest, BoringConstructorTest) {
    // Build a boring PID Zone.

    sdbusplus::SdBusMock sdbus_mock_passive, sdbus_mock_host, sdbus_mock_mode;
    auto bus_mock_passive = sdbusplus::get_mocked_new(&sdbus_mock_passive);
    auto bus_mock_host = sdbusplus::get_mocked_new(&sdbus_mock_host);
    auto bus_mock_mode = sdbusplus::get_mocked_new(&sdbus_mock_mode);

    EXPECT_CALL(sdbus_mock_host,
                sd_bus_add_object_manager(
                    IsNull(),
                    _,
                    StrEq("/xyz/openbmc_project/extsensors")))
    .WillOnce(Return(0));

    SensorManager m(std::move(bus_mock_passive),
                     std::move(bus_mock_host));

    bool defer = true;
    const char *objPath = "/path/";
    int64_t zone = 1;
    float minThermalRpm = 1000.0;
    float failSafePercent = 0.75;

    int i;
    std::vector<std::string> properties;
    SetupDbusObject(&sdbus_mock_mode, defer, objPath, modeInterface,
                    properties, &i);

    PIDZone p(zone, minThermalRpm, failSafePercent, m, bus_mock_mode, objPath,
              defer);
    // Success.
}

}

class PidZoneTest : public ::testing::Test {
    protected:
        PidZoneTest()
        : property_index(),
          properties(),
          sdbus_mock_passive(),
          sdbus_mock_host(),
          sdbus_mock_mode()
        {
            EXPECT_CALL(sdbus_mock_host,
                sd_bus_add_object_manager(
                    IsNull(),
                    _,
                    StrEq("/xyz/openbmc_project/extsensors")))
            .WillOnce(Return(0));

            auto bus_mock_passive =
                sdbusplus::get_mocked_new(&sdbus_mock_passive);
            auto bus_mock_host = sdbusplus::get_mocked_new(&sdbus_mock_host);
            auto bus_mock_mode = sdbusplus::get_mocked_new(&sdbus_mock_mode);

            // Compiler weirdly not happy about just instantiating mgr(...);
            SensorManager m(std::move(bus_mock_passive),
                            std::move(bus_mock_host));
            mgr = std::move(m);

            SetupDbusObject(&sdbus_mock_mode, defer, objPath, modeInterface,
                            properties, &property_index);

            zone = std::make_unique<PIDZone>(zoneId, minThermalRpm,
                                             failSafePercent, mgr,
                                             bus_mock_mode, objPath, defer);
        }

        // unused
        int property_index;
        std::vector<std::string> properties;

        sdbusplus::SdBusMock sdbus_mock_passive;
        sdbusplus::SdBusMock sdbus_mock_host;
        sdbusplus::SdBusMock sdbus_mock_mode;
        int64_t zoneId = 1;
        float minThermalRpm = 1000.0;
        float failSafePercent = 0.75;
        bool defer = true;
        const char *objPath = "/path/";
        SensorManager mgr;

        std::unique_ptr<PIDZone> zone;
};

TEST_F(PidZoneTest, getZoneIdTest) {
    // Verifies the zoneId returned is what we expect.

    EXPECT_EQ(zoneId, zone->getZoneId());
}

TEST_F(PidZoneTest, getsetManualModeTest) {
    // Verifies that manual mode is false by default and can be set.
    EXPECT_FALSE(zone->getManualMode());

    zone->setManualMode(true);
    EXPECT_TRUE(zone->getManualMode());
}

TEST_F(PidZoneTest, RpmSetPointsTest) {
    // Tests addRPMSetPoint, clearRPMSetPoints, determineMaxRPMRequest
    // and getMinThermalRpmSetPt.

    // At least one value must be above the minimum thermal setpoint used in
    // the constructor otherwise it'll choose that value
    std::vector<float> values = {100, 200, 300, 400, 500, 5000};
    for (auto v : values)
    {
        zone->addRPMSetPoint(v);
    }

    // This will pull the maximum RPM setpoint request.
    zone->determineMaxRPMRequest();

    EXPECT_EQ(5000, zone->getMaxRPMRequest());

    // Clear the values, so it'll choose the minimum thermal setpoint.
    zone->clearRPMSetPoints();

    zone->determineMaxRPMRequest();
    EXPECT_EQ(zone->getMinThermalRpmSetPt(), zone->getMaxRPMRequest());
}

TEST_F(PidZoneTest, getFailSafePrecentTest) {
    // Verify the value used to create the object is stored.
    EXPECT_EQ(failSafePercent, zone->getFailSafePercent());
}

// Things to verify:
// getFailSafeMode
// addFanPID
// addThermalPID
// getCachedValue
// addFanInput
// addThermalInput
// updateFanTelemetry
// updateSensors
// initializeCache
// process_fans
// process_thermals
// getSensor
// manual
// failSafe
