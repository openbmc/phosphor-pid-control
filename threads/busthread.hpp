#pragma once

#include <sdbusplus/bus.hpp>

struct ThreadParams
{
    sdbusplus::bus::bus& _bus;
    std::string _name;
};

void busThread(struct ThreadParams& params);
