#pragma once

#include <nlohmann/json.hpp>
#include <string>

/**
 * Given a json configuration file, parse and validate it.
 *
 * The json data must be valid, and must contain two keys:
 * sensors, and zones.
 * There must be at least one sensor, and one zone.
 * That one zone must contain at least one PID.
 */
json parseValidateJson(const std::string& path);
