#pragma once

#include "pid/zone.hpp"

/* Given a zone, run through the loops. */
void pidControlThread(PIDZone* zone);
