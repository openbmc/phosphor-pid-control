#include "sensors/host.hpp"

#include <chrono>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <sdbusplus/test/sdbus_mock.hpp>
#include <string>
#include <vector>

#include "test/helpers.hpp"

using ::testing::IsNull;
using ::testing::Return;
using ::testing::StrEq;

TEST(HostSensorTest, BoringConstructorTest) {
    // WARN: The host sensor is not presently meant to be created this way,
    // TODO: Can I move the constructor into private?
}

TEST(HostSensorTest, CreateHostTempSensorTest) {
    // The normal case for this sensor is to be a temperature sensor, where
    // the value is treated as a margin sensor.

    sdbusplus::SdBusMock sdbus_mock;
    auto bus_mock = sdbusplus::get_mocked_new(&sdbus_mock);
    std::string name = "fleeting0";
    int64_t timeout = 1;
    const char *objPath = "/asdf/asdf0";
    bool defer = false;
    std::string interface = "xyz.openbmc_project.Sensor.Value";

    // Scale is the only property we change in the code.  Also, emit_object_added isn't called twice.
    // Temperature is the default for type, and everything, so those aren't updated.
    std::vector<std::string> properties = {"Scale"};
    int i;

    SetupDbusObject(
        &sdbus_mock,
        defer,
        objPath,
        interface,
        properties,
        &i);

    // Why does it only update one property..?

    EXPECT_CALL(sdbus_mock,
                sd_bus_emit_object_removed(IsNull(), StrEq(objPath)))
        .WillOnce(Return(0));

    std::unique_ptr<Sensor> s = HostSensor::CreateTemp(
        name, timeout, bus_mock, objPath, defer);
}

TEST(HostSensorTest, VerifyWriteThenReadMatches) {
    // Verify that when value is updated, the information matches
    // what we expect when read back.
}
