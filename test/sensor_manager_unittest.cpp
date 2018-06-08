#include "sensors/manager.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sdbusplus/test/sdbus_mock.hpp>

using ::testing::_;
using ::testing::IsNull;
using ::testing::Return;
using ::testing::StrEq;

TEST(SensorManagerTest, BoringConstructorTest) {
    // Build a boring SensorManager.

    sdbusplus::SdBusMock sdbus_mock_passive, sdbus_mock_host;
    auto bus_mock_passive = sdbusplus::get_mocked_new(&sdbus_mock_passive);
    auto bus_mock_host = sdbusplus::get_mocked_new(&sdbus_mock_host);

    EXPECT_CALL(sdbus_mock_host,
                sd_bus_add_object_manager(
                    IsNull(),
                    _,
                    StrEq("/xyz/openbmc_project/extsensors")))
    .WillOnce(Return(0));

    SensorManager s(std::move(bus_mock_passive), std::move(bus_mock_host));
    // Success
}
