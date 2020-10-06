#include "dbus/dbusutil.hpp"

#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace pid_control
{
namespace
{

using ::testing::StrEq;
using ::testing::UnorderedElementsAreArray;

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

class FindSensorsTest : public ::testing::Test
{
  protected:
    const std::unordered_map<std::string, std::string> sensors = {
        {"path_a", "b"},
        {"apple", "juice"},
        {"other_le", "thing"},
    };

    std::vector<std::pair<std::string, std::string>> results;
};

TEST_F(FindSensorsTest, NoMatches)
{
    const std::string target = "abcd";

    EXPECT_FALSE(findSensors(sensors, target, results));
}

TEST_F(FindSensorsTest, OneMatches)
{
    const std::string target = "a";

    EXPECT_TRUE(findSensors(sensors, target, results));

    std::vector<std::pair<std::string, std::string>> expected_results = {
        {"path_a", "b"},
    };

    EXPECT_THAT(results, UnorderedElementsAreArray(expected_results));
}

TEST_F(FindSensorsTest, MultipleMatches)
{
    const std::string target = "le";
    EXPECT_TRUE(findSensors(sensors, target, results));

    std::vector<std::pair<std::string, std::string>> expected_results = {
        {"apple", "juice"},
        {"other_le", "thing"},
    };

    EXPECT_THAT(results, UnorderedElementsAreArray(expected_results));
}

} // namespace
} // namespace pid_control
