#include "dbus/dbusutil.hpp"

#include <sdbusplus/message/native_types.hpp>
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
using ::testing::ElementsAreArray;
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

class FindSensorsPathsTest : public ::testing::Test
{
  protected:
    static std::string path(const std::string& name)
    {
        return (sdbusplus::object_path(SensorValue::namespace_path::value) /
                SensorValue::namespace_path::temperature / name)
            .str;
    }

    const std::unordered_map<std::string, std::string> sensors = {
        {path("SENTINEL_DOME_SLOT_1_DIMM_A0_TEMP_C"), "a0"},
        {path("SENTINEL_DOME_SLOT_1_DIMM_B11_TEMP_C"), "b11"},
        {path("SENTINEL_DOME_SLOT_1_DIMM_C0_TEMP_C"), "c0"},
        {path("SENTINEL_DOME_SLOT_2_DIMM_A0_TEMP_C"), "slot2"},
        {path("SENTINEL_DOME_SLOT_1_MB_CPU_TEMP_C"), "cpu"},
        {path("SENTINEL_DOME_SLOT_1_MB_FIO_TEMP_C"), "fio"},
        {path("SENTINEL_DOME_SLOT_1_MB_SSD_BOOT_TEMP_C"), "ssd"},
    };

    std::vector<std::pair<std::string, std::string>> results;
};

// A name written with regex syntax stands for every sensor it describes.
TEST_F(FindSensorsPathsTest, RegexNameMatchesAllSensorsItDescribes)
{
    EXPECT_TRUE(findSensors(
        sensors, R"(SENTINEL_DOME_SLOT_1_DIMM_[AB]\d+_TEMP_C)", results));
    std::vector<std::pair<std::string, std::string>> expected = {
        {path("SENTINEL_DOME_SLOT_1_DIMM_A0_TEMP_C"), "a0"},
        {path("SENTINEL_DOME_SLOT_1_DIMM_B11_TEMP_C"), "b11"},
    };
    EXPECT_THAT(results, UnorderedElementsAreArray(expected));

    results.clear();
    EXPECT_TRUE(findSensors(sensors, "SENTINEL_DOME_SLOT_1_MB_(CPU|FIO)_TEMP_C",
                            results));
    expected = {
        {path("SENTINEL_DOME_SLOT_1_MB_CPU_TEMP_C"), "cpu"},
        {path("SENTINEL_DOME_SLOT_1_MB_FIO_TEMP_C"), "fio"},
    };
    EXPECT_THAT(results, UnorderedElementsAreArray(expected));

    results.clear();
    EXPECT_TRUE(
        findSensors(sensors, "SENTINEL_DOME_SLOT_1_MB_.*_TEMP_C", results));
    expected = {
        {path("SENTINEL_DOME_SLOT_1_MB_CPU_TEMP_C"), "cpu"},
        {path("SENTINEL_DOME_SLOT_1_MB_FIO_TEMP_C"), "fio"},
        {path("SENTINEL_DOME_SLOT_1_MB_SSD_BOOT_TEMP_C"), "ssd"},
    };
    EXPECT_THAT(results, UnorderedElementsAreArray(expected));
}

// A name may include the directory of the sensor to tell apart sensors of
// different types that share the last part of their path.
TEST(FindSensorsNamespaceTest, NameWithDirectoryMatchesOnlyThatDirectory)
{
    const std::string tach =
        (sdbusplus::object_path(SensorValue::namespace_path::value) /
         SensorValue::namespace_path::fan_tach / "Fan_1")
            .str;
    const std::string temp =
        (sdbusplus::object_path(SensorValue::namespace_path::value) /
         SensorValue::namespace_path::temperature / "Fan_1")
            .str;
    const std::unordered_map<std::string, std::string> sensors = {
        {tach, "tach"},
        {temp, "temp"},
    };
    std::vector<std::pair<std::string, std::string>> results;

    EXPECT_TRUE(findSensors(sensors, "fan_tach/Fan_1", results));
    const std::vector<std::pair<std::string, std::string>> expected = {
        {tach, "tach"},
    };
    EXPECT_THAT(results, ElementsAreArray(expected));
}

// The names of one controller are resolved into the same vector, so the sensors
// found for an earlier name stay in it.
TEST_F(FindSensorsPathsTest, SensorsOfSeveralNamesAreCollectedInOneVector)
{
    findSensors(sensors, "SENTINEL_DOME_SLOT_1_MB_CPU_TEMP_C", results);
    findSensors(sensors, "SENTINEL_DOME_SLOT_1_NO_SUCH_SENSOR", results);
    findSensors(sensors, "SENTINEL_DOME_SLOT_1_MB_FIO_TEMP_C", results);

    const std::vector<std::pair<std::string, std::string>> expected = {
        {path("SENTINEL_DOME_SLOT_1_MB_CPU_TEMP_C"), "cpu"},
        {path("SENTINEL_DOME_SLOT_1_MB_FIO_TEMP_C"), "fio"},
    };
    EXPECT_THAT(results, ElementsAreArray(expected));
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
