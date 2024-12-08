#pragma once

#include "dbushelper_interface.hpp"
#include "interfaces.hpp"
#include "util.hpp"

#include <sdbusplus/bus.hpp>

#include <memory>
#include <string>

namespace pid_control
{

/*
 * This ReadInterface will actively reach out over dbus upon calling read to
 * get the value from whomever owns the associated dbus path.
 */
class DbusActiveRead : public ReadInterface
{
  public:
    DbusActiveRead(sdbusplus::bus_t& bus, const std::string& path,
                   const std::string& service,
                   std::unique_ptr<DbusHelperInterface> helper) :
        ReadInterface(), _bus(bus), _path(path), _service(service),
        _helper(std::move(helper))
    {}

    ReadReturn read(void) override;

  private:
    // Inform the compiler that this variable might not be used,
    // which suppresses the warning "private field '_bus' is not used"
    [[maybe_unused]] sdbusplus::bus_t& _bus;
    const std::string _path;
    const std::string _service; // the sensor service.
    std::unique_ptr<DbusHelperInterface> _helper;
};

} // namespace pid_control
