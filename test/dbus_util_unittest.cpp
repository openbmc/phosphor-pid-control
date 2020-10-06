#include "dbus/dbusutil.hpp"

#include <string>
#include <tuple>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace pid_control
{
namespace
{

using ::testing::StrEq;

class GetSensorPathTest :
    public ::testing::TestWithParam<
        std::tuple<std::string, std::string, std::string>>
{};

TEST_P(GetSensorPathTest, ReturnsExpectedValue)
{
    // type, id, output
    const auto& params = GetParam();
    EXPECT_THAT(getSensorPath(std::get<0>(params), std::get<1>(params)),
                StrEq(std::get<2>(params)));
}

INSTANTIATE_TEST_CASE_P(
    GetSensorPathTests, GetSensorPathTest,
    ::testing::Values(
        std::make_tuple("fan", "0", "/xyz/openbmc_project/sensors/fan_tach/0"),
        std::make_tuple("as", "we", "/xyz/openbmc_project/sensors/unknown/we"),
        std::make_tuple("margin", "9",
                        "/xyz/openbmc_project/sensors/temperature/9"),
        std::make_tuple("temp", "123",
                        "/xyz/openbmc_project/sensors/temperature/123")));

} // namespace
} // namespace pid_control
