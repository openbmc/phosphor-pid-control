#include "dbus/dbusactiveread.hpp"
#include "test/dbushelper_mock.hpp"

#include <sdbusplus/test/sdbus_mock.hpp>

#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace pid_control
{
namespace
{

using ::testing::_;
using ::testing::Invoke;
using ::testing::NotNull;

TEST(DbusActiveReadTest, BoringConstructorTest)
{
    // Verify we can construct it.

    sdbusplus::SdBusMock sdbus_mock;
    auto bus_mock = sdbusplus::get_mocked_new(&sdbus_mock);
    auto helper = std::make_unique<DbusHelperMock>();
    std::string path = "/asdf";
    std::string service = "asdfasdf.asdfasdf";

    DbusActiveRead ar(bus_mock, path, service, std::move(helper));
}

TEST(DbusActiveReadTest, Read_VerifyCallsToDbusForValue)
{
    // Verify it calls to get the value from dbus when requested.

    sdbusplus::SdBusMock sdbus_mock;
    auto bus_mock = sdbusplus::get_mocked_new(&sdbus_mock);
    auto helper = std::make_unique<DbusHelperMock>();
    std::string path = "/asdf";
    std::string service = "asdfasdf.asdfasdf";

    EXPECT_CALL(*helper, getProperties(service, path, NotNull()))
        .WillOnce(Invoke([&]([[maybe_unused]] const std::string& service,
                             [[maybe_unused]] const std::string& path,
                             SensorProperties* prop) {
            prop->scale = -3;
            prop->value = 10000;
            prop->unit = "x";
        }));

    DbusActiveRead ar(bus_mock, path, service, std::move(helper));

    ReadReturn r = ar.read();
    EXPECT_EQ(10, r.value);
}

// WARN: getProperties will raise an exception on failure
// Instead of just not updating the value.

} // namespace
} // namespace pid_control
