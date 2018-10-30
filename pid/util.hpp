#pragma once

#include "ec/pid.hpp"

/*
 * Given a configuration structure, fill out the information we use within the
 * PID loop.
 */
void initializePIDStruct(ec::pid_info_t* info, const ec::pidinfo& initial);

void dumpPIDStruct(ec::pid_info_t* info);
