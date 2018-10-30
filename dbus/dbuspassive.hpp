#pragma once

#include "dbus/util.hpp"
#include "interfaces.hpp"

#include <chrono>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/server.hpp>
#include <set>
#include <string>
#include <tuple>
#include <vector>

int DbusHandleSignal(sd_bus_message* msg, void* data, sd_bus_error* err);

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
    static std::unique_ptr<ReadInterface>
        createDbusPassive(sdbusplus::bus::bus& bus, const std::string& type,
                          const std::string& id, DbusHelperInterface* helper);

    DbusPassive(sdbusplus::bus::bus& bus, const std::string& type,
                const std::string& id, DbusHelperInterface* helper);

    ReadReturn read(void) override;
    bool getFailed(void) const override;

    void setValue(double value);
    void setFailed(bool value);
    int64_t getScale(void);
    std::string getID(void);

  private:
    sdbusplus::bus::bus& _bus;
    sdbusplus::server::match::match _signal;
    int64_t _scale;
    std::string _id; // for debug identification
    DbusHelperInterface* _helper;

    std::mutex _lock;
    double _value = 0;
    bool _failed = false;
    /* The last time the value was refreshed, not necessarily changed. */
    std::chrono::high_resolution_clock::time_point _updated;
};

int HandleSensorValue(sdbusplus::message::message& msg, DbusPassive* owner);
