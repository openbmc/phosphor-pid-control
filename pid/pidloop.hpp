#pragma once

#include "pid/zone_interface.hpp"

#include <boost/asio/steady_timer.hpp>

namespace pid_control
{

/**
 * Main pid control loop for a given zone.
 * This function calls itself indefinitely in an async loop to calculate
 * fan outputs based on thermal inputs.
 *
 * @param[in] zone - ptr to the ZoneInterface implementation for this loop.
 * @param[in] timer - boost timer used for async callback.
 * @param[in] isCanceling - bool ptr to indicate whether pidControlLoop is being
 * canceled.
 * @param[in] first - boolean to denote if initialization needs to be run.
 * @param[in] ms100cnt - loop timer counter.
 */
void pidControlLoop(std::shared_ptr<ZoneInterface> zone,
                    std::shared_ptr<boost::asio::steady_timer> timer,
                    const bool* isCanceling, bool first = true,
                    int ms100cnt = 0);

} // namespace pid_control
