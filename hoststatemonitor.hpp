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
#include <optional>
#include <string>
#include <variant>

using HostState = sdbusplus::common::xyz::openbmc_project::state::Host;

constexpr const char* propertiesInterface = "org.freedesktop.DBus.Properties";
constexpr const char* hostStatePath = "/xyz/openbmc_project/state/host";

class HostStateMonitor
{
  public:
    static void initializeAll(sdbusplus::bus_t& bus);
    static void stopAll();
    static std::optional<std::reference_wrapper<HostStateMonitor>> getInstance(
        uint64_t slotId = 0);

    explicit HostStateMonitor(sdbusplus::bus_t& bus,
                              const std::string& hostObjectPath,
                              uint64_t slotId);
    ~HostStateMonitor() = default;

    // Delete copy constructor and assignment operator
    HostStateMonitor(const HostStateMonitor&) = delete;
    HostStateMonitor& operator=(const HostStateMonitor&) = delete;

    // Delete move constructor and assignment operator
    HostStateMonitor(HostStateMonitor&&) = delete;
    HostStateMonitor& operator=(HostStateMonitor&&) = delete;

    void startMonitoring();
    void stopMonitoring();
    void getInitialState();
    bool isPowerOn() const
    {
        return powerStatusOn;
    }

    uint64_t getSlotId() const
    {
        return slotId;
    }

    void setPowerStatus(bool status)
    {
        powerStatusOn = status;
    }

  private:
    void handleStateChange(sdbusplus::message_t& message);

    static uint64_t parseSlotIdFromPath(const std::string& path);
    static void handleInterfacesAdded(
        sdbusplus::bus_t& bus, const std::string& path,
        const std::unordered_map<
            std::string,
            std::unordered_map<std::string, std::variant<std::string>>>&
            interfaces);
    static void setupInterfacesAddedMatch(sdbusplus::bus_t& bus);
    static void discoverExistingHosts(sdbusplus::bus_t& bus);

    sdbusplus::bus_t& bus;
    uint64_t slotId;
    std::string hostPath;
    bool powerStatusOn;
    std::unique_ptr<sdbusplus::bus::match_t> hostStateMatch;
    inline static std::unordered_map<uint64_t,
                                     std::unique_ptr<HostStateMonitor>>
        instances;
    inline static std::unique_ptr<sdbusplus::bus::match_t> interfacesAddedMatch;
};

inline void HostStateMonitor::initializeAll(sdbusplus::bus_t& bus)
{
    setupInterfacesAddedMatch(bus);
    discoverExistingHosts(bus);
}

inline void HostStateMonitor::setupInterfacesAddedMatch(sdbusplus::bus_t& bus)
{
    using namespace sdbusplus::bus::match::rules;
    interfacesAddedMatch = std::make_unique<sdbusplus::bus::match_t>(
        bus, interfacesAdded("/xyz/openbmc_project/state"),
        [&bus](sdbusplus::message_t& message) {
            auto [path, interfaces] = message.unpack<
                sdbusplus::message::object_path,
                std::unordered_map<
                    std::string,
                    std::unordered_map<std::string,
                                       std::variant<std::string>>>>();

            handleInterfacesAdded(bus, path.str, interfaces);
        });
}

inline void HostStateMonitor::handleInterfacesAdded(
    sdbusplus::bus_t& bus, const std::string& pathStr,
    const std::unordered_map<
        std::string,
        std::unordered_map<std::string, std::variant<std::string>>>& interfaces)
{
    auto findIface = interfaces.find(HostState::interface);
    if (findIface == interfaces.end())
    {
        return;
    }

    if (!pathStr.starts_with(hostStatePath))
    {
        return;
    }

    uint64_t slotId = 0;
    try
    {
        slotId = parseSlotIdFromPath(pathStr);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Unable to parse slotId from path " << pathStr << ": "
                  << e.what() << std::endl;
        return;
    }

    if (instances.find(slotId) != instances.end())
    {
        return;
    }

    auto monitor = std::make_unique<HostStateMonitor>(bus, pathStr, slotId);
    const auto& properties = findIface->second;
    auto findState =
        properties.find(HostState::property_names::current_host_state);
    if (findState != properties.end())
    {
        const std::string& stateValue =
            std::get<std::string>(findState->second);
        monitor->setPowerStatus(stateValue.ends_with(".Running"));
    }

    instances.emplace(slotId, std::move(monitor));
}

inline void HostStateMonitor::discoverExistingHosts(sdbusplus::bus_t& bus)
{
    using ObjectMapper = sdbusplus::common::xyz::openbmc_project::ObjectMapper;

    auto mapper = bus.new_method_call(
        ObjectMapper::default_service, ObjectMapper::instance_path,
        ObjectMapper::interface, ObjectMapper::method_names::get_sub_tree);
    mapper.append("/xyz/openbmc_project/state", 0,
                  std::array<const char*, 1>{HostState::interface});

    auto resp = bus.call(mapper);
    auto respData = resp.unpack<std::unordered_map<
        std::string,
        std::unordered_map<std::string, std::vector<std::string>>>>();

    for (const auto& [pathStr, _] : respData)
    {
        if (!pathStr.starts_with(hostStatePath))
        {
            continue;
        }

        uint64_t slotId = 0;
        try
        {
            slotId = parseSlotIdFromPath(pathStr);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Unable to parse slotId from path " << pathStr << ": "
                      << e.what() << std::endl;
            continue;
        }

        if (instances.find(slotId) != instances.end())
        {
            continue;
        }

        auto monitor = std::make_unique<HostStateMonitor>(bus, pathStr, slotId);
        monitor->getInitialState();
        instances.emplace(slotId, std::move(monitor));
    }
}

inline void HostStateMonitor::stopAll()
{
    interfacesAddedMatch.reset();

    for (auto& [slotId, monitor] : instances)
    {
        if (monitor)
        {
            monitor->stopMonitoring();
        }
    }

    instances.clear();
}

inline uint64_t HostStateMonitor::parseSlotIdFromPath(const std::string& path)
{
    return std::stoull(path.substr(std::strlen(hostStatePath)));
}

inline std::optional<std::reference_wrapper<HostStateMonitor>>
    HostStateMonitor::getInstance(uint64_t slotId)
{
    auto it = instances.find(slotId);
    if (it == instances.end())
    {
        return std::nullopt;
    }

    return *it->second;
}

inline HostStateMonitor::HostStateMonitor(
    sdbusplus::bus_t& bus, const std::string& hostObjectPath, uint64_t slotId) :
    bus(bus), slotId(slotId), hostPath(hostObjectPath), powerStatusOn(false),
    hostStateMatch(nullptr)
{
    startMonitoring();
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
    try
    {
        auto [objectName, values] =
            message.unpack<std::string,
                           boost::container::flat_map<
                               std::string, std::variant<std::string>>>();

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
        std::cerr << "Failed to handle host " << slotId
                  << " state change: " << e.what() << std::endl;
    }
}

inline void HostStateMonitor::getInitialState()
{
    std::string service =
        "xyz.openbmc_project.State.Host" + std::to_string(slotId);

    try
    {
        auto method = bus.new_method_call(service.c_str(), hostPath.c_str(),
                                          propertiesInterface, "Get");
        method.append(HostState::interface,
                      HostState::property_names::current_host_state);

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
