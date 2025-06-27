/**
 * Copyright 2022 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "logging.hpp"

#include "../tuning.hpp"
#include "pid.hpp"

#include <chrono>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <utility>

namespace pid_control
{
namespace ec
{

// Redundant log entries only once every 60 seconds
static constexpr int logThrottle = 60 * 1000;

static std::map<std::string, PidCoreLog> nameToLog;

static bool CharValid(const std::string::value_type& ch)
{
    // Intentionally avoiding invoking locale support here
    if ((ch >= 'A') && (ch <= 'Z'))
    {
        return true;
    }
    if ((ch >= 'a') && (ch <= 'z'))
    {
        return true;
    }
    if ((ch >= '0') && (ch <= '9'))
    {
        return true;
    }
    return false;
}

static std::string StrClean(const std::string& str)
{
    std::string res;
    size_t len = str.size();
    for (size_t i = 0; i < len; ++i)
    {
        const auto& c = str[i];
        if (!(CharValid(c)))
        {
            continue;
        }
        res += c;
    }
    return res;
}

static void DumpContextHeader(std::ofstream& file)
{
    file << "epoch_ms,input,setpoint,error";
    file << ",proportionalTerm";
    file << ",integralTerm1,integralTerm2";
    file << ",derivativeTerm";
    file << ",feedFwdTerm,output1,output2";
    file << ",minOut,maxOut";
    file << ",integralTerm3,output3";
    file << ",integralTerm,output";
    file << "\n" << std::flush;
}

static void DumpContextData(std::ofstream& file,
                            const std::chrono::milliseconds& msNow,
                            const PidCoreContext& pc)
{
    file << msNow.count();
    file << "," << pc.input << "," << pc.setpoint << "," << pc.error;
    file << "," << pc.proportionalTerm;
    file << "," << pc.integralTerm1 << "," << pc.integralTerm2;
    file << "," << pc.derivativeTerm;
    file << "," << pc.feedFwdTerm << "," << pc.output1 << "," << pc.output2;
    file << "," << pc.minOut << "," << pc.maxOut;
    file << "," << pc.integralTerm3 << "," << pc.output3;
    file << "," << pc.integralTerm << "," << pc.output;
    file << "\n" << std::flush;
}

static void DumpCoeffsHeader(std::ofstream& file)
{
    file << "epoch_ms,ts,integral,lastOutput";
    file << ",proportionalCoeff,integralCoeff";
    file << ",derivativeCoeff";
    file << ",feedFwdOffset,feedFwdGain";
    file << ",integralLimit.min,integralLimit.max";
    file << ",outLim.min,outLim.max";
    file << ",slewNeg,slewPos";
    file << ",positiveHysteresis,negativeHysteresis";
    file << "\n" << std::flush;
}

static void DumpCoeffsData(std::ofstream& file,
                           const std::chrono::milliseconds& msNow,
                           pid_info_t* pidinfoptr)
{
    // Save some typing
    const auto& p = *pidinfoptr;

    file << msNow.count();
    file << "," << p.ts << "," << p.integral << "," << p.lastOutput;
    file << "," << p.proportionalCoeff << "," << p.integralCoeff;
    file << "," << p.derivativeCoeff;
    file << "," << p.feedFwdOffset << "," << p.feedFwdGain;
    file << "," << p.integralLimit.min << "," << p.integralLimit.max;
    file << "," << p.outLim.min << "," << p.outLim.max;
    file << "," << p.slewNeg << "," << p.slewPos;
    file << "," << p.positiveHysteresis << "," << p.negativeHysteresis;
    file << "\n" << std::flush;
}

void LogInit(const std::string& name, pid_info_t* pidinfoptr)
{
    if (!coreLoggingEnabled)
    {
        // PID logging not enabled by configuration, silently do nothing
        return;
    }

    if (name.empty())
    {
        std::cerr << "PID logging disabled because PID does not have a name\n";
        return;
    }

    std::string cleanName = StrClean(name);
    if (cleanName.empty())
    {
        std::cerr << "PID logging disabled because PID name is unusable: "
                  << name << "\n";
        return;
    }

    auto iterExisting = nameToLog.find(name);

    if (iterExisting != nameToLog.end())
    {
        std::cerr << "PID logging reusing existing file: " << name << "\n";
    }
    else
    {
        // Multiple names could collide to the same clean name
        // Make sure clean name is not already used
        for (const auto& iter : nameToLog)
        {
            if (iter.second.nameClean == cleanName)
            {
                std::cerr << "PID logging disabled because of name collision: "
                          << name << "\n";
                return;
            }
        }

        std::string filec = loggingPath + "/pidcore." + cleanName;
        std::string filef = loggingPath + "/pidcoeffs." + cleanName;

        std::ofstream outc;
        std::ofstream outf;

        outc.open(filec);
        if (!(outc.good()))
        {
            std::cerr << "PID logging disabled because unable to open file: "
                      << filec << "\n";
            return;
        }

        outf.open(filef);
        if (!(outf.good()))
        {
            // Be sure to clean up all previous initialization
            outf.close();

            std::cerr << "PID logging disabled because unable to open file: "
                      << filef << "\n";
            return;
        }

        PidCoreLog newLog;

        // All good, commit to doing logging by moving into the map
        newLog.nameOriginal = name;
        newLog.nameClean = cleanName;
        newLog.fileContext = std::move(outc);
        newLog.fileCoeffs = std::move(outf);

        // The streams within this object are not copyable, must move them
        nameToLog[name] = std::move(newLog);

        // This must now succeed, as it must be in the map
        iterExisting = nameToLog.find(name);

        // Write headers only when creating files for the first time
        DumpContextHeader(iterExisting->second.fileContext);
        DumpCoeffsHeader(iterExisting->second.fileCoeffs);

        std::cerr << "PID logging initialized: " << name << "\n";
    }

    auto msNow = LogTimestamp();

    // Write the coefficients only once per PID loop initialization
    // If they change, caller will reinitialize the PID loops
    DumpCoeffsData(iterExisting->second.fileCoeffs, msNow, pidinfoptr);

    // Force the next logging line to be logged
    iterExisting->second.lastLog = iterExisting->second.lastLog.zero();
    iterExisting->second.lastContext = PidCoreContext();
}

PidCoreLog* LogPeek(const std::string& name)
{
    auto iter = nameToLog.find(name);
    if (iter != nameToLog.end())
    {
        return &(iter->second);
    }

    return nullptr;
}

void LogContext(PidCoreLog& pidLog, const std::chrono::milliseconds& msNow,
                const PidCoreContext& coreContext)
{
    bool shouldLog = false;

    if (pidLog.lastLog == pidLog.lastLog.zero())
    {
        // It is the first time
        shouldLog = true;
    }
    else
    {
        auto since = msNow - pidLog.lastLog;
        if (since.count() >= logThrottle)
        {
            // It has been long enough since the last time
            shouldLog = true;
        }
    }

    if (pidLog.lastContext != coreContext)
    {
        // The content is different
        shouldLog = true;
    }

    if (!shouldLog)
    {
        return;
    }

    pidLog.lastLog = msNow;
    pidLog.lastContext = coreContext;

    DumpContextData(pidLog.fileContext, msNow, coreContext);
}

std::chrono::milliseconds LogTimestamp(void)
{
    auto clockNow = std::chrono::high_resolution_clock::now();
    auto msNow = std::chrono::duration_cast<std::chrono::milliseconds>(
        clockNow.time_since_epoch());
    return msNow;
}

} // namespace ec
} // namespace pid_control
