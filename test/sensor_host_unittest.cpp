#include "sensors/host.hpp"

#include <chrono>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>

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

    std::unique_ptr<Sensor> s = HostSensor::CreateTemp(
        name, timeout, bus_mock, objPath, defer);


}

TEST(HostSensorTest, VerifyWriteThenReadMatches) {
    // Verify that when value is updated, the information matches
    // what we expect when read back.
}
