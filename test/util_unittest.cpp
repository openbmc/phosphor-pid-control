#include "util.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

TEST(UtilTest, WriteTypeEmptyString) {
    // Verify it responds to an empty string.

    EXPECT_EQ(IOInterfaceType::NONE, GetWriteInterfaceType(""));
}

TEST(UtilTest, WriteTypeNonePath) {
    // Verify it responds to a path of "None"

    EXPECT_EQ(IOInterfaceType::NONE, GetWriteInterfaceType("None"));
}

TEST(UtilTest, WriteTypeSysfs) {
    // Verify the sysfs type is determined with an expected path

    std::string path = "/sys/devices/asfdadsf";
    EXPECT_EQ(IOInterfaceType::SYSFS, GetWriteInterfaceType(path));
}

TEST(UtilTest, WriteTypeUnknown) {
    // Verify it reports unknown by default.

    std::string path = "/xyz/openbmc_project";
    EXPECT_EQ(IOInterfaceType::UNKNOWN, GetWriteInterfaceType(path));
}

