#pragma once

#include "pid/zone.hpp"

/* Given a zone, run through the loops. */
void PIDControlThread(std::shared_ptr<PIDZone> zone);
