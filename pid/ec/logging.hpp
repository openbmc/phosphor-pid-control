#pragma once

#include "pid.hpp"

#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

namespace pid_control
{
namespace ec
{

// Trivial class for information exported from core PID loop function
struct PidCore
{
    double input;
    double setpoint;
    double error;

    double proportionalTerm;
    double integralTerm1;
    double integralTerm2;

    double feedFwdTerm;
    double output1;
    double output2;

    double minOut;
    double maxOut;

    double integralTerm3;
    double output3;

    double integralTerm;
    double output;

    bool operator!=(const PidCore& rhs)
    {
        // The compiler should allow "= default;" for operator!=
        return (std::memcmp(this, &rhs, sizeof(*this)) != 0);
    }
};

// Optional decorator class for each PID loop, to support logging
// Although this is a trivial class, it ended up needing the Six Horsemen
struct PidLog
{
    std::string nameOriginal;
    std::string nameClean;
    std::ofstream filePid;
    std::ofstream fileCoeffs;
    std::chrono::milliseconds lastLog;
    PidCore lastCore;
    bool moved;

    PidLog() :
        nameOriginal(), nameClean(), filePid(), fileCoeffs(), lastLog(),
        lastCore(), moved(false)
    {}

    PidLog(const PidLog& copy) = delete;

    PidLog& operator=(const PidLog& copy) = delete;

    PidLog(PidLog&& move)
    {
        // Reuse assignment operator below
        *this = std::move(move);
    }

    PidLog& operator=(PidLog&& move)
    {
        nameOriginal = std::move(move.nameOriginal);
        nameClean = std::move(move.nameClean);
        filePid = std::move(move.filePid);
        fileCoeffs = std::move(move.fileCoeffs);
        lastLog = std::move(move.lastLog);
        lastCore = std::move(move.lastCore);

        // Mark the moved object, so destructor knows it was moved
        move.moved = true;
        return *this;
    }

    ~PidLog()
    {
        // Do not close files if ownership was moved to another object
        if (!moved)
        {
            filePid.close();
            fileCoeffs.close();
        }
    }
};

// Initializes logging files, call once per PID loop initialization
void LogInit(const std::string& name, pid_info_t* pidinfoptr);

// Returns PidLog pointer, or nullptr if this PID loop not being logged
PidLog* LogPeek(const std::string& name);

// Logs a line of logging, if different, or it has been long enough
void LogPidCore(PidLog& pidLog, const std::chrono::milliseconds& msNow,
                const PidCore& coreLog);

// Takes a timestamp, suitable for column 1 of logging file output
std::chrono::milliseconds LogTimestamp(void);

} // namespace ec
} // namespace pid_control
