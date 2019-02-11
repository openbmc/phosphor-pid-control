#include "build/buildjson.hpp"

#include "errors/exception.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void validateJson(const json& data)
{
    if (data.find("sensors") == data.end())
    {
        throw ConfigurationException("KeyError: 'sensors' not found");
    }

    if (data.find("zones") == data.end())
    {
        throw ConfigurationException("KeyError: 'zones' not found");
    }
}

json parseValidateJson(const std::string& path)
{
    std::ifstream jsonFile(path);
    if (!jsonFile.is_open())
    {
        throw ConfigurationException("Unable to open json file");
    }

    auto data = json::parse(jsonFile, nullptr, false);
    if (data.is_discarded())
    {
        throw ConfigurationException("Invalid json - parse failed");
    }

    /* Check the data. */
    validateJson(data);

    return data;
}
