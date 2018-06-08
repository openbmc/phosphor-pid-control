#include "sensors/manager.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sdbusplus/test/sdbus_mock.hpp>

TEST(SensorManagerTest, BoringConstructorTest) {
    // Build a boring SensorManager.

    sdbusplus::SdBusMock sdbus_mock_passive, sdbus_mock_host;
    auto bus_mock_passive = sdbusplus::get_mocked_new(&sdbus_mock_passive);
    auto bus_mock_host = sdbusplus::get_mocked_new(&sdbus_mock_host);

    SensorManager s(std::move(bus_mock_passive), std::move(bus_mock_host));
    // Success
}
