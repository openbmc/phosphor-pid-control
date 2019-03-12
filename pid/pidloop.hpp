#pragma once

#include "pid/zone.hpp"

#include <boost/asio/steady_timer.hpp>

/* Given a zone, run through the loops. */
void pidControlLoop(PIDZone* zone, boost::asio::steady_timer& timer,
                    bool first = true, int ms100cnt = 0);
