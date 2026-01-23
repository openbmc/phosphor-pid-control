#pragma once

#include <boost/container/flat_map.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <xyz/openbmc_project/ObjectMapper/common.hpp>
#include <xyz/openbmc_project/State/Host/common.hpp>

#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <variant>

using HostState = sdbusplus::common::xyz::openbmc_project::state::Host;

constexpr const char* PROPERTIES_INTERFACE = "org.freedesktop.DBus.Properties";
constexpr const char* HOST_STATE_PATH_PREFIX = "/xyz/openbmc_project/state/host";

class HostStateMonitor
{
  public:
    static void initializeAll(sdbusplus::bus_t& bus);
    static HostStateMonitor& getInstance(uint64_t slotId = 0);

    explicit HostStateMonitor(sdbusplus::bus_t& bus, uint64_t slotId);
    ~HostStateMonitor() = default;

    // Delete copy constructor and assignment operator
    HostStateMonitor(const HostStateMonitor&) = delete;
    HostStateMonitor& operator=(const HostStateMonitor&) = delete;

    // Delete move constructor and assignment operator
    HostStateMonitor(HostStateMonitor&&) = delete;
    HostStateMonitor& operator=(HostStateMonitor&&) = delete;

    void startMonitoring();
    void stopMonitoring();
    bool isPowerOn() const
    {
        return powerStatusOn;
    }

    uint64_t getSlotId() const
    {
        return slotId;
    }

  private:
    void handleStateChange(sdbusplus::message_t& message);
    void getInitialState();

    sdbusplus::bus_t& bus;
    uint64_t slotId;
    std::string hostPath;
    std::string hostService;
    bool powerStatusOn;
    std::unique_ptr<sdbusplus::bus::match_t> hostStateMatch;
    inline static std::unordered_map<uint64_t, std::unique_ptr<HostStateMonitor>> instances;
};

// Implementation
inline void HostStateMonitor::initializeAll(sdbusplus::bus_t& bus)
{
    using ObjectMapper = sdbusplus::common::xyz::openbmc_project::ObjectMapper;
    using GetSubTreeType = std::unordered_map<
        std::string, std::unordered_map<std::string, std::vector<std::string>>>;

    auto mapper = bus.new_method_call(
        ObjectMapper::default_service, ObjectMapper::instance_path,
        ObjectMapper::interface, ObjectMapper::method_names::get_sub_tree);
    mapper.append("/xyz/openbmc_project/state", 0,
                  std::array<const char*, 1>{HostState::interface});

    auto resp = bus.call(mapper);
    auto respData = resp.unpack<GetSubTreeType>();

    constexpr size_t prefixLen = std::char_traits<char>::length(HOST_STATE_PATH_PREFIX);
    for (const auto& [path, services] : respData)
    {
        if (path.size() <= prefixLen || !path.starts_with(HOST_STATE_PATH_PREFIX))
        {
            continue;
        }

        auto slotId = std::stoull(path.substr(prefixLen));
        auto monitor = std::make_unique<HostStateMonitor>(bus, slotId);
        monitor->startMonitoring();
        instances.emplace(slotId, std::move(monitor));
    }

    // Default to host0 if no hosts discovered
    if (instances.empty())
    {
        auto monitor = std::make_unique<HostStateMonitor>(bus, 0);
        monitor->startMonitoring();
        instances.emplace(0, std::move(monitor));
    }
}

inline HostStateMonitor& HostStateMonitor::getInstance(uint64_t slotId)
{
    auto it = instances.find(slotId);
    if (it == instances.end())
    {
        it = instances.find(0);
    }
    return *(it->second);
}

inline HostStateMonitor::HostStateMonitor(sdbusplus::bus_t& bus,
                                          uint64_t slotId) :
    bus(bus), slotId(slotId), powerStatusOn(false), hostStateMatch(nullptr)
{
    hostPath = "/xyz/openbmc_project/state/host" + std::to_string(slotId);
    hostService = "xyz.openbmc_project.State.Host" + std::to_string(slotId);
    getInitialState();
}

inline void HostStateMonitor::startMonitoring()
{
    if (hostStateMatch)
    {
        return;
    }

    using namespace sdbusplus::bus::match::rules;

    hostStateMatch = std::make_unique<sdbusplus::bus::match_t>(
        bus, propertiesChangedNamespace(hostPath, HostState::interface),
        [this](sdbusplus::message_t& message) { handleStateChange(message); });
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

        auto findState =
            values.find(HostState::property_names::current_host_state);
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
        std::cerr << "Failed to handle host" << slotId << " state change: "
                  << e.what() << std::endl;
    }
}

inline void HostStateMonitor::getInitialState()
{
    try
    {
        auto method = bus.new_method_call(hostService.c_str(), hostPath.c_str(),
                                          PROPERTIES_INTERFACE, "Get");
        method.append(HostState::interface,
                      HostState::property_names::current_host_state);

        auto reply = bus.call(method);
        std::variant<std::string> currentState;
        reply.read(currentState);

        const std::string& stateValue = std::get<std::string>(currentState);
        powerStatusOn = stateValue.ends_with(".Running");
    }
    catch (const std::exception& e)
    {
        powerStatusOn = false;
    }
}
