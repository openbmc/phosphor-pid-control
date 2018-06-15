#include "util.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

TEST(UtilTest, WriteTypeEmptyString_ReturnsNONE) {
    // Verify it responds to an empty string.

    EXPECT_EQ(IOInterfaceType::NONE, GetWriteInterfaceType(""));
}

TEST(UtilTest, WriteTypeNonePath_ReturnsNONE) {
    // Verify it responds to a path of "None"

    EXPECT_EQ(IOInterfaceType::NONE, GetWriteInterfaceType("None"));
}

TEST(UtilTest, WriteTypeSysfs_ReturnsSYSFS) {
    // Verify the sysfs type is determined with an expected path

    std::string path = "/sys/devices/asfdadsf";
    EXPECT_EQ(IOInterfaceType::SYSFS, GetWriteInterfaceType(path));
}

TEST(UtilTest, WriteTypeUnknown_ReturnsUNKNOWN) {
    // Verify it reports unknown by default.

    std::string path = "/xyz/openbmc_project";
    EXPECT_EQ(IOInterfaceType::UNKNOWN, GetWriteInterfaceType(path));
}

TEST(UtilTest, ReadTypeEmptyString_ReturnsNONE) {
    // Verify it responds to an empty string.

    EXPECT_EQ(IOInterfaceType::NONE, GetReadInterfaceType(""));
}

TEST(UtilTest, ReadTypeNonePath_ReturnsNONE) {
    // Verify it responds to a path of "None"

    EXPECT_EQ(IOInterfaceType::NONE, GetReadInterfaceType("None"));
}

TEST(UtilTest, ReadTypeExternalSensors_ReturnsEXTERNAL) {
    // Verify it responds to a path that represents a host sensor.

    std::string path = "/xyz/openbmc_project/extsensors/temperature/fleeting0";
    EXPECT_EQ(IOInterfaceType::EXTERNAL, GetReadInterfaceType(path));
}

TEST(UtilTest, ReadTypeOpenBMCSensor_ReturnsDBUSPASSIVE) {
    // Verify it responds to a path that represents a dbus sensor.

    std::string path = "/xyz/openbmc_project/sensors/fan_tach/fan1";
    EXPECT_EQ(IOInterfaceType::DBUSPASSIVE, GetReadInterfaceType(path));
}

TEST(UtilTest, ReadTypeSysfsPath_ReturnsSYSFS) {
    // Verify the sysfs type is determined with an expected path

    std::string path = "/sys/devices/asdf/asdf0";
    EXPECT_EQ(IOInterfaceType::SYSFS, GetReadInterfaceType(path));
}

TEST(UtilTest, ReadTypeUnknownDefault_ReturnsUNKNOWN) {
    // Verify it reports unknown by default.

    std::string path = "asdf09as0df9a0fd";
    EXPECT_EQ(IOInterfaceType::UNKNOWN, GetReadInterfaceType(path));
}
