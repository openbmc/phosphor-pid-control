#include "build/buildjson.hpp"

#include "errors/exception.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

json parseValidateJson(const std::string& path)
{
    std::ifstream jsonFile(path);
    if (!jsonFile.is_open())
    {
        throw ConfigurationException("Unable to open json file");
    }

    auto data = Json::parse(jsonFile, nullptr, false);
    if (data.is_discarded())
    {
        throw ConfigurationException("Invalid json - parse failed");
    }

    return data;
}
