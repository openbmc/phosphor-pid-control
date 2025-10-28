#include "dbus/dbusutil.hpp"

#include <xyz/openbmc_project/Sensor/Value/common.hpp>

#include <cstdint>
#include <map>
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

using SensorValue = sdbusplus::common::xyz::openbmc_project::sensor::Value;

using ::testing::ContainerEq;
using ::testing::Eq;
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

INSTANTIATE_TEST_SUITE_P(
    GetSensorPathTests, GetSensorPathTest,
    ::testing::Values(
        std::make_tuple("fan", "0",
                        std::format("{}/{}/0",
                                    SensorValue::namespace_path::value,
                                    SensorValue::namespace_path::fan_tach)),
        std::make_tuple("as", "we",
                        std::format("{}/unknown/we",
                                    SensorValue::namespace_path::value)),
        std::make_tuple("margin", "9",
                        std::format("{}/{}/9",
                                    SensorValue::namespace_path::value,
                                    SensorValue::namespace_path::temperature)),
        std::make_tuple("temp", "123",
                        std::format("{}/{}/123",
                                    SensorValue::namespace_path::value,
                                    SensorValue::namespace_path::temperature)),
        std::make_tuple("power", "9000",
                        std::format("{}/{}/9000",
                                    SensorValue::namespace_path::value,
                                    SensorValue::namespace_path::power)),
        std::make_tuple("powersum", "total",
                        std::format("{}/{}/total",
                                    SensorValue::namespace_path::value,
                                    SensorValue::namespace_path::power))));

class FindSensorsTest : public ::testing::Test
{
  protected:
    const std::unordered_map<std::string, std::string> sensors = {
        {"/abcd/_a", "b"}, {"_a", "c"},          {"/abcd_a", "d"},
        {"/_a_a", "e"},    {"one/slash", "one"}, {"other_/slash", "other"},
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
    const std::string target = "_a";

    EXPECT_TRUE(findSensors(sensors, target, results));

    std::vector<std::pair<std::string, std::string>> expected_results = {
        {"/abcd/_a", "b"},
    };

    EXPECT_THAT(results, UnorderedElementsAreArray(expected_results));
}

TEST_F(FindSensorsTest, MultipleMatches)
{
    const std::string target = "slash";
    EXPECT_TRUE(findSensors(sensors, target, results));

    std::vector<std::pair<std::string, std::string>> expected_results = {
        {"one/slash", "one"},
        {"other_/slash", "other"},
    };

    EXPECT_THAT(results, UnorderedElementsAreArray(expected_results));
}

TEST(GetZoneIndexTest, ZoneAlreadyAssigned)
{
    std::map<std::string, int64_t> zones = {
        {"a", 0},
    };
    const std::map<std::string, int64_t> expected_zones = {
        {"a", 0},
    };

    EXPECT_THAT(getZoneIndex("a", zones), Eq(0));
    EXPECT_THAT(zones, ContainerEq(expected_zones));
}

TEST(GetZoneIndexTest, ZoneNotYetAssignedZeroBased)
{
    /* This calls into setZoneIndex, but is a case hit by getZoneIndex. */
    std::map<std::string, int64_t> zones;
    const std::map<std::string, int64_t> expected_zones = {
        {"a", 0},
    };

    EXPECT_THAT(getZoneIndex("a", zones), Eq(0));
    EXPECT_THAT(zones, ContainerEq(expected_zones));
}

TEST(SetZoneIndexTest, ZoneAlreadyAssigned)
{
    std::map<std::string, int64_t> zones = {
        {"a", 0},
    };
    const std::map<std::string, int64_t> expected_zones = {
        {"a", 0},
    };

    EXPECT_THAT(setZoneIndex("a", zones, 0), Eq(0));
    EXPECT_THAT(zones, ContainerEq(expected_zones));
}

TEST(SetZoneIndexTest, ZoneNotYetAssignedEmptyListZeroBased)
{
    constexpr int64_t index = 0;
    std::map<std::string, int64_t> zones;
    const std::map<std::string, int64_t> expected_zones = {
        {"a", index},
    };

    EXPECT_THAT(setZoneIndex("a", zones, index), Eq(index));
    EXPECT_THAT(zones, ContainerEq(expected_zones));
}

TEST(SetZoneIndexTest, ZoneNotYetAssignedEmptyListNonZeroBased)
{
    constexpr int64_t index = 5;
    std::map<std::string, int64_t> zones;
    const std::map<std::string, int64_t> expected_zones = {
        {"a", index},
    };

    EXPECT_THAT(setZoneIndex("a", zones, index), Eq(index));
    EXPECT_THAT(zones, ContainerEq(expected_zones));
}

TEST(SetZoneIndexTest, ZoneListNotEmptyAssignsNextIndexZeroBased)
{
    std::map<std::string, int64_t> zones = {
        {"a", 0},
    };
    const std::map<std::string, int64_t> expected_zones = {
        {"a", 0},
        {"b", 1},
    };

    EXPECT_THAT(setZoneIndex("b", zones, 0), Eq(1));
    EXPECT_THAT(zones, ContainerEq(expected_zones));
}

TEST(SetZoneIndexTest, ZoneListNotEmptyAssignsNextIndexNonZeroBased)
{
    std::map<std::string, int64_t> zones = {
        {"a", 0},
    };
    const std::map<std::string, int64_t> expected_zones = {
        {"a", 0},
        {"b", 5},
    };

    EXPECT_THAT(setZoneIndex("b", zones, 5), Eq(5));
    EXPECT_THAT(zones, ContainerEq(expected_zones));
}

TEST(SetZoneIndexTest, ZoneListNotEmptyAssignsIntoGap)
{
    std::map<std::string, int64_t> zones = {
        {"a", 0},
        {"b", 5},
    };
    const std::map<std::string, int64_t> expected_zones = {
        {"a", 0},
        {"c", 1},
        {"b", 5},
    };

    EXPECT_THAT(setZoneIndex("c", zones, 0), Eq(1));
    EXPECT_THAT(zones, ContainerEq(expected_zones));
}

} // namespace
} // namespace pid_control
