#include "pid/buildjson.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

TEST(ZoneFromJson, emptyZone)
{
    // There is a zone key, but it's empty.
    // This is technically invalid.

    std::map<int64_t, PIDConf> pidConfig;
    std::map<int64_t, struct ZoneConfig> zoneConfig;

    auto j2 = R"(
      {
        "zones": []
      }
    )"_json;

    std::tie(pidConfig, zoneConfig) = buildPIDsFromJson(j2);
}
