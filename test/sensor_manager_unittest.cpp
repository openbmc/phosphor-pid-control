#include "sensors/manager.hpp"
#include "test/sensor_mock.hpp"

#include <sdbusplus/test/sdbus_mock.hpp>

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

TEST(SensorManagerTest, BoringConstructorTest)
{
    // Build a boring SensorManager.

    sdbusplus::SdBusMock sdbusMockPassive, sdbusMockHost;
    auto busMockPassive = sdbusplus::get_mocked_new(&sdbusMockPassive);
    auto busMockHost = sdbusplus::get_mocked_new(&sdbusMockHost);

    EXPECT_CALL(sdbusMockHost,
                sd_bus_add_object_manager(
                    IsNull(), _, StrEq("/xyz/openbmc_project/extsensors")))
        .WillOnce(Return(0));

    SensorManager s(busMockPassive, busMockHost);
    // Success
}

TEST(SensorManagerTest, AddSensorInvalidTypeTest)
{
    // AddSensor doesn't validate the type of sensor you're adding, because
    // ultimately it doesn't care -- but if we decide to change that this
    // test will start failing :D

    sdbusplus::SdBusMock sdbusMockPassive, sdbusMockHost;
    auto busMockPassive = sdbusplus::get_mocked_new(&sdbusMockPassive);
    auto busMockHost = sdbusplus::get_mocked_new(&sdbusMockHost);

    EXPECT_CALL(sdbusMockHost,
                sd_bus_add_object_manager(
                    IsNull(), _, StrEq("/xyz/openbmc_project/extsensors")))
        .WillOnce(Return(0));

    SensorManager s(busMockPassive, busMockHost);

    std::string name = "name";
    std::string type = "invalid";
    int64_t timeout = 1;
    std::unique_ptr<Sensor> sensor =
        std::make_unique<SensorMock>(name, timeout);
    Sensor* sensorPtr = sensor.get();

    s.addSensor(type, name, std::move(sensor));
    EXPECT_EQ(s.getSensor(name), sensorPtr);
}

} // namespace
} // namespace pid_control
