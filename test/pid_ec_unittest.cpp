// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright 2024 Google Inc

#include "pid/ec/pid.hpp"

#include <cmath>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

namespace pid_control
{
namespace ec
{
namespace
{

// Helper: create a basic pid_info_t with given coefficients
pid_info_t makePidInfo(double kp = 0.0, double ki = 0.0, double kd = 0.0,
                       double ffOffset = 0.0, double ffGain = 0.0,
                       double ts = 0.1)
{
    pid_info_t info;
    info.ts = ts;
    info.proportionalCoeff = kp;
    info.integralCoeff = ki;
    info.derivativeCoeff = kd;
    info.feedFwdOffset = ffOffset;
    info.feedFwdGain = ffGain;
    // No output limits by default
    info.outLim.min = std::numeric_limits<double>::lowest();
    info.outLim.max = std::numeric_limits<double>::max();
    info.integralLimit.min = std::numeric_limits<double>::lowest();
    info.integralLimit.max = std::numeric_limits<double>::max();
    return info;
}

// =========================================================================
// TC01: Incremental first call output equals delta value
// =========================================================================
TEST(PidEcTest, IncrementalFirstCall)
{
    auto info = makePidInfo(1.0, 0.0, 0.0);
    info.incrementalMode = true;
    // initialized=false, lastError=0, lastOutput=0

    double output = pid(&info, 0.0, 10.0);
    // error = 10, deltaP = 1.0 * (10 - 0) = 10
    // output = 0 + 10 = 10
    EXPECT_DOUBLE_EQ(output, 10.0);
    EXPECT_TRUE(info.initialized);
}

// =========================================================================
// TC02: Incremental steady state output unchanged
// =========================================================================
TEST(PidEcTest, IncrementalSteadyState)
{
    auto info = makePidInfo(1.0, 0.0, 0.0);
    info.incrementalMode = true;
    info.initialized = true;
    info.lastError = 0.0;
    info.lastInput = 10.0;
    info.lastLastInput = 10.0;
    info.lastOutput = 50.0;

    double output = pid(&info, 10.0, 10.0);
    // error = 0, deltaP = 1.0 * (0 - 0) = 0
    // output = 50 + 0 = 50
    EXPECT_DOUBLE_EQ(output, 50.0);
}

// =========================================================================
// TC03: Incremental pure proportional response
// =========================================================================
TEST(PidEcTest, IncrementalPureProportional)
{
    auto info = makePidInfo(2.0, 0.0, 0.0);
    info.incrementalMode = true;
    // initialized=false, lastError=0, lastOutput=0

    double output = pid(&info, 0.0, 5.0);
    // error = 5, deltaP = 2.0 * (5 - 0) = 10
    // output = 0 + 10 = 10
    EXPECT_DOUBLE_EQ(output, 10.0);
}

// =========================================================================
// TC04: Incremental pure integral response
// =========================================================================
TEST(PidEcTest, IncrementalPureIntegral)
{
    auto info = makePidInfo(0.0, 1.0, 0.0, 0.0, 0.0, 0.1);
    info.incrementalMode = true;
    // initialized=false, lastOutput=0

    double output = pid(&info, 0.0, 10.0);
    // error = 10, deltaI = 1.0 * 10 * 0.1 = 1.0
    // output = 0 + 1.0 = 1.0
    EXPECT_DOUBLE_EQ(output, 1.0);
}

// =========================================================================
// TC05: Incremental pure derivative response (derivative on input)
// =========================================================================
TEST(PidEcTest, IncrementalPureDerivative)
{
    auto info = makePidInfo(0.0, 0.0, 1.0, 0.0, 0.0, 0.1);
    info.incrementalMode = true;
    info.initialized = true;
    info.lastInput = 2.0;
    info.lastLastInput = 0.0;
    info.lastOutput = 0.0;

    double output = pid(&info, 10.0, 10.0);
    // error = 0, deltaP = 0, deltaI = 0
    // deltaD = -1.0 * (10 - 2*2 + 0) / 0.1 = -1.0 * 6 / 0.1 = -60
    // output = 0 + (-60) = -60
    EXPECT_DOUBLE_EQ(output, -60.0);
}

// =========================================================================
// TC06: Incremental pure feed-forward response
// =========================================================================
TEST(PidEcTest, IncrementalPureFeedForward)
{
    auto info = makePidInfo(0.0, 0.0, 0.0, 0.0, 1.0);
    info.incrementalMode = true;
    info.initialized = true;
    info.lastSetpoint = 0.0;
    info.lastOutput = 0.0;

    double output = pid(&info, 0.0, 10.0);
    // FF[k] = (10+0)*1.0 = 10, FF[k-1] = (0+0)*1.0 = 0
    // deltaFF = 10 - 0 = 10
    // output = 0 + 10 = 10
    EXPECT_DOUBLE_EQ(output, 10.0);
}

// =========================================================================
// TC07: Incremental vs positional equivalence with constant setpoint
// When setpoint is constant, derivative-on-error and derivative-on-input
// are equivalent, so positional and incremental should match.
// =========================================================================
TEST(PidEcTest, IncrementalVsPositionalEquivalenceConstantSetpoint)
{
    auto infoPos = makePidInfo(0.5, 0.1, 0.05, 0.0, 0.2, 0.1);
    infoPos.incrementalMode = false;

    auto infoInc = makePidInfo(0.5, 0.1, 0.05, 0.0, 0.2, 0.1);
    infoInc.incrementalMode = true;

    // Input sequence: varying input, constant setpoint
    std::vector<std::pair<double, double>> testPoints = {
        {0.0, 10.0}, {2.0, 10.0}, {5.0, 10.0}, {8.0, 10.0}, {10.0, 10.0},
        {12.0, 10.0}, {14.0, 10.0}, {12.0, 10.0}, {10.0, 10.0}, {8.0, 10.0},
    };

    for (const auto& [input, setpoint] : testPoints)
    {
        double outPos = pid(&infoPos, input, setpoint);
        double outInc = pid(&infoInc, input, setpoint);
        EXPECT_NEAR(outPos, outInc, 1e-9)
            << "Mismatch at input=" << input << ", setpoint=" << setpoint;
    }
}

// =========================================================================
// TC08: Incremental output clamping behavior
// When output limits are set, the incremental PID output must stay within
// [outLim.min, outLim.max]. Equivalence with positional holds while no
// clamping occurs; after clamping, the two algorithms diverge because
// positional recalculates from scratch while incremental accumulates from
// the clamped value (see spec section 2.7).
// =========================================================================
TEST(PidEcTest, IncrementalOutputClampBehavior)
{
    auto infoInc = makePidInfo(0.5, 0.1, 0.05, 0.0, 0.2, 0.1);
    infoInc.incrementalMode = true;
    infoInc.outLim.min = 0.0;
    infoInc.outLim.max = 50.0;

    std::vector<std::pair<double, double>> testPoints = {
        {0.0, 10.0}, {2.0, 10.0}, {5.0, 10.0}, {8.0, 10.0}, {10.0, 10.0},
        {12.0, 10.0}, {14.0, 10.0}, {12.0, 10.0}, {10.0, 10.0}, {8.0, 10.0},
    };

    for (const auto& [input, setpoint] : testPoints)
    {
        double outInc = pid(&infoInc, input, setpoint);
        EXPECT_GE(outInc, 0.0)
            << "Output below outLim.min at input=" << input;
        EXPECT_LE(outInc, 50.0)
            << "Output above outLim.max at input=" << input;
    }
}

// =========================================================================
// TC09: Incremental slew rate behavior
// When slew rate is set, the incremental PID output change per step must
// stay within [slewNeg*ts, slewPos*ts]. Equivalence with positional holds
// while no slew clamping occurs; after slew clamping, the two algorithms
// diverge (see spec section 2.7).
// =========================================================================
TEST(PidEcTest, IncrementalSlewRateBehavior)
{
    auto infoInc = makePidInfo(0.5, 0.1, 0.05, 0.0, 0.2, 0.1);
    infoInc.incrementalMode = true;
    infoInc.slewNeg = -5.0;
    infoInc.slewPos = 5.0;

    std::vector<std::pair<double, double>> testPoints = {
        {0.0, 10.0}, {2.0, 10.0}, {5.0, 10.0}, {8.0, 10.0}, {10.0, 10.0},
        {12.0, 10.0}, {14.0, 10.0}, {12.0, 10.0}, {10.0, 10.0}, {8.0, 10.0},
    };

    double prevOutput = 0.0;
    bool first = true;
    for (const auto& [input, setpoint] : testPoints)
    {
        double outInc = pid(&infoInc, input, setpoint);
        if (!first && infoInc.initialized)
        {
            double delta = outInc - prevOutput;
            EXPECT_GE(delta, -5.0 * 0.1 - 1e-9)
                << "Slew rate (neg) violated at input=" << input;
            EXPECT_LE(delta, 5.0 * 0.1 + 1e-9)
                << "Slew rate (pos) violated at input=" << input;
        }
        prevOutput = outInc;
        first = false;
    }
}

// =========================================================================
// TC10: Incremental output upper clamp
// =========================================================================
TEST(PidEcTest, IncrementalOutputUpperClamp)
{
    auto info = makePidInfo(1.0, 0.0, 0.0);
    info.incrementalMode = true;
    info.initialized = true;
    info.lastError = 0.0;
    info.lastOutput = 95.0;
    info.outLim.max = 100.0;
    info.outLim.min = std::numeric_limits<double>::lowest();

    double output = pid(&info, 0.0, 20.0);
    // error = 20, deltaP = 1.0 * (20 - 0) = 20
    // theoretical output = 95 + 20 = 115, clamped to 100
    EXPECT_DOUBLE_EQ(output, 100.0);
}

// =========================================================================
// TC11: Incremental output lower clamp
// =========================================================================
TEST(PidEcTest, IncrementalOutputLowerClamp)
{
    auto info = makePidInfo(1.0, 0.0, 0.0);
    info.incrementalMode = true;
    info.initialized = true;
    info.lastError = 0.0;
    info.lastOutput = 5.0;
    info.outLim.min = 0.0;
    info.outLim.max = std::numeric_limits<double>::max();

    double output = pid(&info, 20.0, 0.0);
    // error = -20, deltaP = 1.0 * (-20 - 0) = -20
    // theoretical output = 5 + (-20) = -15, clamped to 0
    EXPECT_DOUBLE_EQ(output, 0.0);
}

// =========================================================================
// TC12: Incremental integral limit clamp
// =========================================================================
TEST(PidEcTest, IncrementalIntegralLimit)
{
    auto info = makePidInfo(0.0, 1.0, 0.0, 0.0, 0.0, 0.1);
    info.incrementalMode = true;
    info.integralLimit.max = 10.0;
    info.integralLimit.min = -10.0;

    // Run many iterations to accumulate integral
    for (int i = 0; i < 200; i++)
    {
        pid(&info, 0.0, 10.0);
    }

    // integral should be clamped to 10.0
    EXPECT_LE(info.integral, 10.0);
    EXPECT_GE(info.integral, -10.0);
}

// =========================================================================
// TC13: Incremental positive slew rate limit
// =========================================================================
TEST(PidEcTest, IncrementalPositiveSlewLimit)
{
    auto info = makePidInfo(1.0, 0.0, 0.0);
    info.incrementalMode = true;
    info.initialized = true;
    info.lastError = 0.0;
    info.lastOutput = 0.0;
    info.slewPos = 2.0;
    info.ts = 0.1;

    double output = pid(&info, 0.0, 10.0);
    // error = 10, deltaP = 1.0 * (10 - 0) = 10
    // slew limit: max delta = 2.0 * 0.1 = 0.2
    // output = 0 + 0.2 = 0.2
    EXPECT_DOUBLE_EQ(output, 0.2);
}

// =========================================================================
// TC14: Incremental negative slew rate limit
// =========================================================================
TEST(PidEcTest, IncrementalNegativeSlewLimit)
{
    auto info = makePidInfo(1.0, 0.0, 0.0);
    info.incrementalMode = true;
    info.initialized = true;
    info.lastError = 0.0;
    info.lastOutput = 0.0;
    info.slewNeg = -2.0;
    info.ts = 0.1;

    double output = pid(&info, 10.0, 0.0);
    // error = -10, deltaP = 1.0 * (-10 - 0) = -10
    // slew limit: min delta = -2.0 * 0.1 = -0.2
    // output = 0 + (-0.2) = -0.2
    EXPECT_DOUBLE_EQ(output, -0.2);
}

// =========================================================================
// TC15: Entry function dispatches to positional
// =========================================================================
TEST(PidEcTest, DispatchToPositional)
{
    auto info = makePidInfo(1.0, 0.0, 0.0);
    info.incrementalMode = false;
    // initialized=false, lastError=0, lastOutput=0

    double output = pid(&info, 0.0, 10.0);
    // Positional: error=10, P=10, I=0, D=0, FF=0, output=10
    EXPECT_DOUBLE_EQ(output, 10.0);
}

// =========================================================================
// TC16: Entry function dispatches to incremental
// =========================================================================
TEST(PidEcTest, DispatchToIncremental)
{
    auto info = makePidInfo(1.0, 0.0, 0.0);
    info.incrementalMode = true;
    // initialized=false, lastError=0, lastOutput=0

    double output = pid(&info, 0.0, 10.0);
    // Incremental: error=10, deltaP=10, output=0+10=10
    EXPECT_DOUBLE_EQ(output, 10.0);
}

// =========================================================================
// TC17: Default mode is positional (backward compatibility)
// =========================================================================
TEST(PidEcTest, DefaultModeIsPositional)
{
    auto info = makePidInfo(1.0, 0.0, 0.0);
    // incrementalMode defaults to false

    double output = pid(&info, 0.0, 10.0);
    // Should use positional algorithm
    EXPECT_DOUBLE_EQ(output, 10.0);
}

// =========================================================================
// TC18: Mode switch - positional to incremental
// =========================================================================
TEST(PidEcTest, ModeSwitchPositionalToIncremental)
{
    auto info = makePidInfo(1.0, 0.0, 0.0);
    info.incrementalMode = false;

    // Run 3 rounds in positional mode
    double out1 = pid(&info, 0.0, 10.0);
    double out2 = pid(&info, 2.0, 10.0);
    double out3 = pid(&info, 5.0, 10.0);

    // Switch to incremental
    info.incrementalMode = true;

    // Continue running - should produce valid output
    double out4 = pid(&info, 8.0, 10.0);
    double out5 = pid(&info, 10.0, 10.0);

    // Outputs should be finite and reasonable
    EXPECT_TRUE(std::isfinite(out4));
    EXPECT_TRUE(std::isfinite(out5));
}

// =========================================================================
// TC19: Mode switch - incremental to positional
// =========================================================================
TEST(PidEcTest, ModeSwitchIncrementalToPositional)
{
    auto info = makePidInfo(1.0, 0.0, 0.0);
    info.incrementalMode = true;

    // Run 3 rounds in incremental mode
    double out1 = pid(&info, 0.0, 10.0);
    double out2 = pid(&info, 2.0, 10.0);
    double out3 = pid(&info, 5.0, 10.0);

    // Switch to positional
    info.incrementalMode = false;

    // Continue running - should produce valid output
    double out4 = pid(&info, 8.0, 10.0);
    double out5 = pid(&info, 10.0, 10.0);

    // Outputs should be finite and reasonable
    EXPECT_TRUE(std::isfinite(out4));
    EXPECT_TRUE(std::isfinite(out5));
}

} // namespace
} // namespace ec
} // namespace pid_control
