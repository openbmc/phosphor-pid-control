#pragma once

#include <sdbusplus/bus.hpp>

struct ThreadParams
{
    sdbusplus::bus::bus& bus;
    std::string name;
};

void BusThread(struct ThreadParams& params);
