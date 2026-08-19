#!/usr/bin/env python3
"""
Standalone verification script for incremental PID algorithm.
Simulates the C++ pid_positional and pid_incremental functions to verify
mathematical equivalence and correctness of test cases TC01-TC19.
"""
import math
import sys

def clamp(x, lo, hi):
    return max(lo, min(hi, x))

class PidInfo:
    def __init__(self):
        self.initialized = False
        self.checkHysterWithSetpt = False
        self.ts = 0.0
        self.integral = 0.0
        self.lastOutput = 0.0
        self.lastError = 0.0
        self.lastInput = 0.0
        self.lastLastInput = 0.0
        self.lastSetpoint = 0.0
        self.incrementalMode = False
        self.proportionalCoeff = 0.0
        self.integralCoeff = 0.0
        self.derivativeCoeff = 0.0
        self.feedFwdOffset = 0.0
        self.feedFwdGain = 0.0
        self.integralLimit = (0.0, 0.0)
        self.outLim = (0.0, 0.0)
        self.slewNeg = 0.0
        self.slewPos = 0.0

def pid_positional(p, input_val, setpoint):
    error = setpoint - input_val
    proportionalTerm = p.proportionalCoeff * error

    integralTerm = 0.0
    if p.integralCoeff != 0.0:
        integralTerm = p.integral
        integralTerm += error * p.integralCoeff * p.ts
        integralTerm = clamp(integralTerm, p.integralLimit[0], p.integralLimit[1])

    derivativeTerm = p.derivativeCoeff * ((error - p.lastError) / p.ts)

    feedFwdTerm = (setpoint + p.feedFwdOffset) * p.feedFwdGain

    output = proportionalTerm + integralTerm + derivativeTerm + feedFwdTerm
    output = clamp(output, p.outLim[0], p.outLim[1])

    if p.initialized:
        if p.slewNeg != 0.0:
            minOut = p.lastOutput + p.slewNeg * p.ts
            if output < minOut:
                output = minOut
        if p.slewPos != 0.0:
            maxOut = p.lastOutput + p.slewPos * p.ts
            if output > maxOut:
                output = maxOut
        if p.slewNeg != 0.0 or p.slewPos != 0.0:
            integralTerm = output - proportionalTerm

    integralTerm = clamp(integralTerm, p.integralLimit[0], p.integralLimit[1])
    p.integral = integralTerm
    p.initialized = True
    p.lastError = error
    p.lastOutput = output
    return output

def pid_incremental(p, input_val, setpoint):
    error = setpoint - input_val

    # deltaP
    if p.initialized:
        deltaProportional = p.proportionalCoeff * (error - p.lastError)
    else:
        deltaProportional = p.proportionalCoeff * error

    # deltaI
    deltaIntegral = 0.0
    if p.integralCoeff != 0.0:
        deltaIntegral = p.integralCoeff * error * p.ts
        newIntegral = p.integral + deltaIntegral
        newIntegral = clamp(newIntegral, p.integralLimit[0], p.integralLimit[1])
        deltaIntegral = newIntegral - p.integral
        p.integral = newIntegral

    # deltaD
    if p.initialized:
        deltaDerivative = -p.derivativeCoeff * (
            input_val - 2.0 * p.lastInput + p.lastLastInput) / p.ts
    else:
        # First call: error-based derivative to match positional
        deltaDerivative = p.derivativeCoeff * error / p.ts

    # deltaFF
    feedFwdCurrent = (setpoint + p.feedFwdOffset) * p.feedFwdGain
    if p.initialized:
        feedFwdPrevious = (p.lastSetpoint + p.feedFwdOffset) * p.feedFwdGain
    else:
        feedFwdPrevious = 0.0
    deltaFeedFwd = feedFwdCurrent - feedFwdPrevious

    # Sum deltas
    deltaOutput = deltaProportional + deltaIntegral + deltaDerivative + deltaFeedFwd

    # Apply delta
    if p.initialized:
        output = p.lastOutput + deltaOutput
    else:
        output = deltaOutput

    # Clamp output
    output = clamp(output, p.outLim[0], p.outLim[1])

    # Slew rate
    if p.initialized:
        if p.slewNeg != 0.0:
            minOut = p.lastOutput + p.slewNeg * p.ts
            if output < minOut:
                output = minOut
        if p.slewPos != 0.0:
            maxOut = p.lastOutput + p.slewPos * p.ts
            if output > maxOut:
                output = maxOut

    # Slew back-calculation
    if p.initialized and (p.slewNeg != 0.0 or p.slewPos != 0.0):
        proportionalTerm = p.proportionalCoeff * error
        p.integral = output - proportionalTerm

    # Final integral clamp
    p.integral = clamp(p.integral, p.integralLimit[0], p.integralLimit[1])

    # Update state
    if p.initialized:
        p.lastLastInput = p.lastInput
    else:
        p.lastLastInput = setpoint
    p.lastInput = input_val
    p.lastError = error
    p.lastSetpoint = setpoint
    p.lastOutput = output
    p.initialized = True

    return output

def make_pid(kp=0, ki=0, kd=0, ff_offset=0, ff_gain=0, ts=0.1):
    p = PidInfo()
    p.ts = ts
    p.proportionalCoeff = kp
    p.integralCoeff = ki
    p.derivativeCoeff = kd
    p.feedFwdOffset = ff_offset
    p.feedFwdGain = ff_gain
    p.outLim = (-math.inf, math.inf)
    p.integralLimit = (-math.inf, math.inf)
    return p

def pid_dispatch(p, input_val, setpoint):
    if p.incrementalMode:
        return pid_incremental(p, input_val, setpoint)
    else:
        return pid_positional(p, input_val, setpoint)

# ---- Tests ----
passed = 0
failed = 0

def check(name, condition, detail=""):
    global passed, failed
    if condition:
        passed += 1
        print(f"  PASS: {name}")
    else:
        failed += 1
        print(f"  FAIL: {name} {detail}")

print("=== TC01: Incremental first call ===")
p = make_pid(1.0, 0, 0)
p.incrementalMode = True
out = pid_dispatch(p, 0.0, 10.0)
check("TC01 output==10", abs(out - 10.0) < 1e-9, f"got {out}")
check("TC01 initialized", p.initialized)

print("=== TC02: Incremental steady state ===")
p = make_pid(1.0, 0, 0)
p.incrementalMode = True
p.initialized = True
p.lastError = 0.0
p.lastOutput = 50.0
out = pid_dispatch(p, 10.0, 10.0)
check("TC02 output==50", abs(out - 50.0) < 1e-9, f"got {out}")

print("=== TC03: Incremental pure proportional ===")
p = make_pid(2.0, 0, 0)
p.incrementalMode = True
out = pid_dispatch(p, 0.0, 5.0)
check("TC03 output==10", abs(out - 10.0) < 1e-9, f"got {out}")

print("=== TC04: Incremental pure integral ===")
p = make_pid(0, 1.0, 0, 0, 0, 0.1)
p.incrementalMode = True
out = pid_dispatch(p, 0.0, 10.0)
check("TC04 output==1.0", abs(out - 1.0) < 1e-9, f"got {out}")

print("=== TC05: Incremental pure derivative ===")
p = make_pid(0, 0, 1.0, 0, 0, 0.1)
p.incrementalMode = True
p.initialized = True
p.lastInput = 2.0
p.lastLastInput = 0.0
p.lastOutput = 0.0
out = pid_dispatch(p, 10.0, 10.0)
check("TC05 output==-60", abs(out - (-60.0)) < 1e-9, f"got {out}")

print("=== TC06: Incremental pure feed-forward ===")
p = make_pid(0, 0, 0, 0, 1.0)
p.incrementalMode = True
p.initialized = True
p.lastSetpoint = 0.0
p.lastOutput = 0.0
out = pid_dispatch(p, 0.0, 10.0)
check("TC06 output==10", abs(out - 10.0) < 1e-9, f"got {out}")

print("=== TC07: Equivalence constant setpoint (no limits) ===")
testPoints = [(0,10),(2,10),(5,10),(8,10),(10,10),(12,10),(14,10),(12,10),(10,10),(8,10)]
pPos = make_pid(0.5, 0.1, 0.05, 0, 0.2, 0.1)
pPos.incrementalMode = False
pInc = make_pid(0.5, 0.1, 0.05, 0, 0.2, 0.1)
pInc.incrementalMode = True
all_ok = True
for i, (inp, sp) in enumerate(testPoints):
    outPos = pid_dispatch(pPos, inp, sp)
    outInc = pid_dispatch(pInc, inp, sp)
    if abs(outPos - outInc) > 1e-9:
        print(f"    iter {i}: input={inp}, pos={outPos}, inc={outInc}, diff={outPos-outInc}")
        all_ok = False
check("TC07 equivalence", all_ok)

print("=== TC08: Incremental output clamping behavior ===")
pInc = make_pid(0.5, 0.1, 0.05, 0, 0.2, 0.1)
pInc.incrementalMode = True
pInc.outLim = (0.0, 50.0)
all_ok = True
for i, (inp, sp) in enumerate(testPoints):
    outInc = pid_dispatch(pInc, inp, sp)
    if outInc < 0.0 - 1e-9 or outInc > 50.0 + 1e-9:
        print(f"    iter {i}: input={inp}, out={outInc} - out of [0,50]")
        all_ok = False
check("TC08 output within [0,50]", all_ok)

print("=== TC09: Incremental slew rate behavior ===")
pInc = make_pid(0.5, 0.1, 0.05, 0, 0.2, 0.1)
pInc.incrementalMode = True
pInc.slewNeg = -5.0
pInc.slewPos = 5.0
all_ok = True
prev = 0.0
first = True
for i, (inp, sp) in enumerate(testPoints):
    outInc = pid_dispatch(pInc, inp, sp)
    if not first and pInc.initialized:
        delta = outInc - prev
        if delta < -5.0 * 0.1 - 1e-9 or delta > 5.0 * 0.1 + 1e-9:
            print(f"    iter {i}: input={inp}, delta={delta} - slew violated")
            all_ok = False
    prev = outInc
    first = False
check("TC09 slew rate within [-0.5, 0.5]", all_ok)

print("=== TC10: Output upper clamp ===")
p = make_pid(1.0, 0, 0)
p.incrementalMode = True
p.initialized = True
p.lastError = 0.0
p.lastOutput = 95.0
p.outLim = (-math.inf, 100.0)
out = pid_dispatch(p, 0.0, 20.0)
check("TC10 output==100", abs(out - 100.0) < 1e-9, f"got {out}")

print("=== TC11: Output lower clamp ===")
p = make_pid(1.0, 0, 0)
p.incrementalMode = True
p.initialized = True
p.lastError = 0.0
p.lastOutput = 5.0
p.outLim = (0.0, math.inf)
out = pid_dispatch(p, 20.0, 0.0)
check("TC11 output==0", abs(out - 0.0) < 1e-9, f"got {out}")

print("=== TC12: Integral limit ===")
p = make_pid(0, 1.0, 0, 0, 0, 0.1)
p.incrementalMode = True
p.integralLimit = (-10.0, 10.0)
for _ in range(200):
    pid_dispatch(p, 0.0, 10.0)
check("TC12 integral<=10", p.integral <= 10.0, f"got {p.integral}")
check("TC12 integral>=-10", p.integral >= -10.0, f"got {p.integral}")

print("=== TC13: Positive slew limit ===")
p = make_pid(1.0, 0, 0)
p.incrementalMode = True
p.initialized = True
p.lastError = 0.0
p.lastOutput = 0.0
p.slewPos = 2.0
p.ts = 0.1
out = pid_dispatch(p, 0.0, 10.0)
check("TC13 output==0.2", abs(out - 0.2) < 1e-9, f"got {out}")

print("=== TC14: Negative slew limit ===")
p = make_pid(1.0, 0, 0)
p.incrementalMode = True
p.initialized = True
p.lastError = 0.0
p.lastOutput = 0.0
p.slewNeg = -2.0
p.ts = 0.1
out = pid_dispatch(p, 10.0, 0.0)
check("TC14 output==-0.2", abs(out - (-0.2)) < 1e-9, f"got {out}")

print("=== TC15: Dispatch to positional ===")
p = make_pid(1.0, 0, 0)
p.incrementalMode = False
out = pid_dispatch(p, 0.0, 10.0)
check("TC15 output==10", abs(out - 10.0) < 1e-9, f"got {out}")

print("=== TC16: Dispatch to incremental ===")
p = make_pid(1.0, 0, 0)
p.incrementalMode = True
out = pid_dispatch(p, 0.0, 10.0)
check("TC16 output==10", abs(out - 10.0) < 1e-9, f"got {out}")

print("=== TC17: Default mode is positional ===")
p = make_pid(1.0, 0, 0)
# incrementalMode defaults to False
out = pid_dispatch(p, 0.0, 10.0)
check("TC17 output==10", abs(out - 10.0) < 1e-9, f"got {out}")

print("=== TC18: Mode switch positional->incremental ===")
p = make_pid(1.0, 0, 0)
p.incrementalMode = False
pid_dispatch(p, 0.0, 10.0)
pid_dispatch(p, 2.0, 10.0)
pid_dispatch(p, 5.0, 10.0)
p.incrementalMode = True
out4 = pid_dispatch(p, 8.0, 10.0)
out5 = pid_dispatch(p, 10.0, 10.0)
check("TC18 finite", math.isfinite(out4) and math.isfinite(out5))

print("=== TC19: Mode switch incremental->positional ===")
p = make_pid(1.0, 0, 0)
p.incrementalMode = True
pid_dispatch(p, 0.0, 10.0)
pid_dispatch(p, 2.0, 10.0)
pid_dispatch(p, 5.0, 10.0)
p.incrementalMode = False
out4 = pid_dispatch(p, 8.0, 10.0)
out5 = pid_dispatch(p, 10.0, 10.0)
check("TC19 finite", math.isfinite(out4) and math.isfinite(out5))

print(f"\n=== Results: {passed} passed, {failed} failed ===")
sys.exit(1 if failed > 0 else 0)
