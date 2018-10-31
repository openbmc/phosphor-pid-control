#pragma once

#include <sdbusplus/bus.hpp>

struct ThreadParams
{
    sdbusplus::bus::bus& bus;
    std::string name;
};

void busThread(struct ThreadParams& params);
