#pragma once

#include "conf.hpp"
#include "dbushelper_interface.hpp"
#include "dbuspassiveredundancy.hpp"
#include "interfaces.hpp"

#include <systemd/sd-bus.h>

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

namespace pid_control
{

int dbusHandleSignal(sd_bus_message* msg, void* data, sd_bus_error* err);

/*
 * This ReadInterface will passively listen for Value updates from whomever
 * owns the associated dbus object.
 *
 * This requires another modification in phosphor-dbus-interfaces that will
 * signal a value update every time it's read instead of only when it changes
 * to help us:
 * - ensure we're still receiving data (since we don't control the reader)
 * - simplify stale data detection
 * - simplify error detection
 */
class DbusPassive : public ReadInterface
{
  public:
    static std::unique_ptr<ReadInterface> createDbusPassive(
        sdbusplus::bus_t& bus, const std::string& type, const std::string& id,
        std::unique_ptr<DbusHelperInterface> helper,
        const conf::SensorConfig* info,
        const std::shared_ptr<DbusPassiveRedundancy>& redundancy);

    DbusPassive(sdbusplus::bus_t& bus, const std::string& type,
                const std::string& id,
                std::unique_ptr<DbusHelperInterface> helper,
                const SensorProperties& settings, bool failed,
                const std::string& path,
                const std::shared_ptr<DbusPassiveRedundancy>& redundancy);

    ReadReturn read(void) override;
    bool getFailed(void) const override;
    std::string getFailReason(void) const override;

    void updateValue(double value, bool force);
    void setValue(double value, double unscaled);
    void setValue(double value);

    void setFailed(bool value);
    void setFunctional(bool value);
    void setAvailable(bool value);

    int64_t getScale(void);
    std::string getID(void);
    double getMax(void);
    double getMin(void);

  private:
    sdbusplus::bus::match_t _signal;
    int64_t _scale;
    std::string _id; // for debug identification
    std::unique_ptr<DbusHelperInterface> _helper;

    std::mutex _lock;
    double _value = 0;
    double _unscaled = 0;
    double _max = 0;
    double _min = 0;
    bool _failed = false;
    bool _functional = true;
    bool _available = true;
    bool _unavailableAsFailed = true;

    bool _typeMargin = false;
    bool _typeFan = false;
    bool _badReading = false;
    bool _marginHot = false;

    std::string path;
    std::shared_ptr<DbusPassiveRedundancy> redundancy;
    /* The last time the value was refreshed, not necessarily changed. */
    std::chrono::high_resolution_clock::time_point _updated;
};

int handleSensorValue(sdbusplus::message_t& msg, DbusPassive* owner);

} // namespace pid_control
