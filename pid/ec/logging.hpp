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
struct PidCoreContext
{
    double input;
    double setpoint;
    double error;

    double proportionalTerm;
    double integralTerm1;
    double integralTerm2;

    double derivativeTerm;

    double feedFwdTerm;
    double output1;
    double output2;

    double minOut;
    double maxOut;

    double integralTerm3;
    double output3;

    double integralTerm;
    double output;

    bool operator!=(const PidCoreContext& rhs) const = default;
    bool operator==(const PidCoreContext& rhs) const = default;
};

// Optional decorator class for each PID loop, to support logging
// Although this is a trivial class, it ended up needing the Six Horsemen
struct PidCoreLog
{
    std::string nameOriginal;
    std::string nameClean;
    std::ofstream fileContext;
    std::ofstream fileCoeffs;
    std::chrono::milliseconds lastLog;
    PidCoreContext lastContext;
    bool moved = false;

    PidCoreLog() :
        nameOriginal(), nameClean(), fileContext(), fileCoeffs(), lastLog(),
        lastContext()
    {}

    PidCoreLog(const PidCoreLog& copy) = delete;

    PidCoreLog& operator=(const PidCoreLog& copy) = delete;

    PidCoreLog(PidCoreLog&& move) noexcept
    {
        // Reuse assignment operator below
        *this = std::move(move);
    }

    PidCoreLog& operator=(PidCoreLog&& move) noexcept
    {
        if (this != &move)
        {
            // Move each field individually
            nameOriginal = std::move(move.nameOriginal);
            nameClean = std::move(move.nameClean);
            fileContext = std::move(move.fileContext);
            fileCoeffs = std::move(move.fileCoeffs);
            lastLog = move.lastLog;
            lastContext = move.lastContext;

            // Mark the moved object, so destructor knows it was moved
            move.moved = true;
        }
        return *this;
    }

    ~PidCoreLog()
    {
        // Do not close files if ownership was moved to another object
        if (!moved)
        {
            fileContext.close();
            fileCoeffs.close();
        }
    }
};

// Initializes logging files, call once per PID loop initialization
void LogInit(const std::string& name, pid_info_t* pidinfoptr);

// Returns PidCoreLog pointer, or nullptr if this PID loop not being logged
PidCoreLog* LogPeek(const std::string& name);

// Logs a line of logging, if different, or it has been long enough
void LogContext(PidCoreLog& pidLog, const std::chrono::milliseconds& msNow,
                const PidCoreContext& coreLog);

// Takes a timestamp, suitable for column 1 of logging file output
std::chrono::milliseconds LogTimestamp(void);

} // namespace ec
} // namespace pid_control
