#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/message.hpp>
#include <xyz/openbmc_project/Control/Mode/client.hpp>
#include <xyz/openbmc_project/Sensor/Value/client.hpp>

#include <cstdint>
#include <cstdio>
#include <string>
#include <variant>

/* Fan Control */
static constexpr auto objectPath = "/xyz/openbmc_project/settings/fanctrl/zone";
static constexpr auto busName = "xyz.openbmc_project.State.FanCtrl";
using Value = std::variant<bool>;

using SensorValue = sdbusplus::common::xyz::openbmc_project::sensor::Value;
using ControlMode = sdbusplus::common::xyz::openbmc_project::control::Mode;

/* Host Sensor. */
static constexpr auto sobjectPath =
    "/xyz/openbmc_project/extsensors/margin/sluggish0";
static constexpr auto sbusName = "xyz.openbmc_project.Hwmon.external";
using sValue = std::variant<int64_t>;

static constexpr auto propertiesintf = "org.freedesktop.DBus.Properties";

static void SetHostSensor(void)
{
    int64_t value = 300;
    sValue v{value};

    std::string busname{sbusName};
    auto PropertyWriteBus = sdbusplus::bus::new_system();
    std::string path{sobjectPath};

    auto pimMsg = PropertyWriteBus.new_method_call(
        busname.c_str(), path.c_str(), propertiesintf, "Set");

    pimMsg.append(SensorValue::interface);
    pimMsg.append(SensorValue::property_names::value);
    pimMsg.append(v);

    try
    {
        auto responseMsg = PropertyWriteBus.call(pimMsg);
        fprintf(stderr, "call to Set the host sensor value succeeded.\n");
    }
    catch (const sdbusplus::exception_t& ex)
    {
        fprintf(stderr, "call to Set the host sensor value failed.\n");
    }
}

static std::string GetControlPath(int8_t zone)
{
    return std::string(objectPath) + std::to_string(zone);
}

static void SetManualMode(int8_t zone)
{
    bool setValue = true;

    Value v{setValue};

    std::string busname{busName};
    auto PropertyWriteBus = sdbusplus::bus::new_system();
    std::string path = GetControlPath(zone);

    auto pimMsg = PropertyWriteBus.new_method_call(
        busname.c_str(), path.c_str(), propertiesintf, "Set");

    pimMsg.append(ControlMode::interface);
    pimMsg.append(ControlMode::property_names::manual);
    pimMsg.append(v);

    try
    {
        auto responseMsg = PropertyWriteBus.call(pimMsg);
        fprintf(stderr, "call to Set the manual mode succeeded.\n");
    }
    catch (const sdbusplus::exception_t& ex)
    {
        fprintf(stderr, "call to Set the manual mode failed.\n");
    }
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    int rc = 0;

    int64_t zone = 0x01;

    SetManualMode(zone);
    SetHostSensor();
    return rc;
}
