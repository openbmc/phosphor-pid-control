#pragma once

#include <boost/container/flat_map.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <variant>

constexpr const char* PROPERTIES_INTERFACE = "org.freedesktop.DBus.Properties";
constexpr const char* HOST_STATE_BUSNAME = "xyz.openbmc_project.State.Host0";
constexpr const char* HOST_STATE_INTERFACE = "xyz.openbmc_project.State.Host";
constexpr const char* HOST_STATE_PATH = "/xyz/openbmc_project/state/host0";
constexpr const char* CURRENT_HOST_STATE_PROPERTY = "CurrentHostState";

class HostStateMonitor
{
  public:
    static HostStateMonitor& getInstance();
    static HostStateMonitor& getInstance(sdbusplus::bus_t& bus);

    ~HostStateMonitor() = default;

    // Delete copy constructor and assignment operator
    HostStateMonitor(const HostStateMonitor&) = delete;
    HostStateMonitor& operator=(const HostStateMonitor&) = delete;

    // Delete move constructor and assignment operator for singleton
    HostStateMonitor(HostStateMonitor&&) = delete;
    HostStateMonitor& operator=(HostStateMonitor&&) = delete;

    void startMonitoring();
    void stopMonitoring();
    bool isPowerOn() const
    {
        return powerStatusOn;
    }

  private:
    explicit HostStateMonitor(sdbusplus::bus_t& bus);

    void handleStateChange(sdbusplus::message_t& message);
    void getInitialState();

    sdbusplus::bus_t& bus;
    bool powerStatusOn;
    std::unique_ptr<sdbusplus::bus::match_t> hostStateMatch;
};

// Implementation
inline HostStateMonitor& HostStateMonitor::getInstance()
{
    static sdbusplus::bus_t defaultBus = sdbusplus::bus::new_default();
    return getInstance(defaultBus);
}

inline HostStateMonitor& HostStateMonitor::getInstance(sdbusplus::bus_t& bus)
{
    static HostStateMonitor instance(bus);
    return instance;
}

inline HostStateMonitor::HostStateMonitor(sdbusplus::bus_t& bus) :
    bus(bus), powerStatusOn(false), hostStateMatch(nullptr)
{
    getInitialState();
}

inline void HostStateMonitor::startMonitoring()
{
    if (hostStateMatch == nullptr)
    {
        using namespace sdbusplus::bus::match::rules;

        hostStateMatch = std::make_unique<sdbusplus::bus::match_t>(
            bus,
            propertiesChangedNamespace(HOST_STATE_PATH, HOST_STATE_INTERFACE),
            [this](sdbusplus::message_t& message) {
                handleStateChange(message);
            });
    }
}

inline void HostStateMonitor::stopMonitoring()
{
    hostStateMatch.reset();
}

inline void HostStateMonitor::handleStateChange(sdbusplus::message_t& message)
{
    std::string objectName;
    boost::container::flat_map<std::string, std::variant<std::string>> values;

    try
    {
        message.read(objectName, values);

        auto findState = values.find(CURRENT_HOST_STATE_PROPERTY);
        if (findState != values.end())
        {
            const std::string& stateValue =
                std::get<std::string>(findState->second);
            bool newPowerStatus = stateValue.ends_with(".Running");

            if (newPowerStatus != powerStatusOn)
            {
                powerStatusOn = newPowerStatus;
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to handle host state change: " << e.what()
                  << std::endl;
    }
}

inline void HostStateMonitor::getInitialState()
{
    try
    {
        auto method = bus.new_method_call(HOST_STATE_BUSNAME, HOST_STATE_PATH,
                                          PROPERTIES_INTERFACE, "Get");
        method.append(HOST_STATE_INTERFACE, CURRENT_HOST_STATE_PROPERTY);

        auto reply = bus.call(method);
        auto currentState = reply.unpack<std::variant<std::string>>();

        const std::string& stateValue = std::get<std::string>(currentState);
        powerStatusOn = stateValue.ends_with(".Running");
    }
    catch (const std::exception& e)
    {
        powerStatusOn = false;
    }
}
