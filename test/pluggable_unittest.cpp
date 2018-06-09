#include "sensors/pluggable.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "test/readinterface_mock.hpp"
#include "test/writeinterface_mock.hpp"

TEST(PluggableSensorTest, BoringConstructorTest) {
    // Build a boring Pluggable Sensor.

    int64_t min = 0;
    int64_t max = 255;

    std::unique_ptr<ReadInterface> ri = std::make_unique<ReadInterfaceMock>();
    std::unique_ptr<WriteInterface> wi =
        std::make_unique<WriteInterfaceMock>(min, max);

    std::string name = "name";
    int64_t timeout = 1;

    PluggableSensor p(name, timeout, std::move(ri), std::move(wi));
    // Successfully created it.
}
