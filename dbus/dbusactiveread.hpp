#pragma once

#include <memory>
#include <string>

#include <sdbusplus/bus.hpp>

#include "interfaces.hpp"

/*
 * This ReadInterface will actively reach out over dbus upon calling read to
 * get the value from whomever owns the associated dbus path.
 */
class DbusActiveRead : public ReadInterface
{
  public:
    DbusActiveRead(sdbusplus::bus::bus& bus, const std::string& path,
                   const std::string& service) :
        ReadInterface(),
        _bus(bus), _path(path), _service(service)
    {
    }

    ReadReturn read(void) override;

  private:
    sdbusplus::bus::bus& _bus;
    const std::string _path;
    const std::string _service; // the sensor service.
};
