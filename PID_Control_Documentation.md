# PID Control Loop Documentation for phosphor-pid-control

This document provides an overview of the PID control (closed-loop) mechanism
implemented in phosphor-pid-control. It describes the theoretical basis and
implementation details of various PID-related features, including mathematical
formulation, integral wind-up prevention, hysteresis handling, and dynamic
setpoint behavior.

## PID Mathematical Formulation

The fundamental form of the PID control law is expressed as:

$$
u(t) = K_pe(t) + K_i\int_{0}^{t} e(\tau)d\tau + K_d\frac{de(t)}{dt}
$$

$K_p$ : proportional coefficient.

$K_i$ : integral coefficient.

$K_d$ : derivative coefficient.

$u(t)$ : output.

$e(t)$ : error = SP - input.

Next, we expand the integral and derivative terms of the PID equation:

$$
u(t) = K_p \cdot e(t)
      + K_i \cdot \sum_{\tau=0}^{t} e(\tau)\\cdot\Delta t
      + K_d \cdot \frac{e(t) - e(t - \Delta t)}{\Delta t}
$$

Assuming a constant sampling interval $\Delta t$, the summation term can be
simplified accordingly.

$$
u(t) = K_p \cdot e(t)
      + K_i \cdot \big[ e(t) + e(t-\Delta t) + e(t-2\cdot \Delta t) + \cdots + e(0) \big] \cdot \Delta t
      + K_d \cdot \frac{e(t) - e(t - \Delta t)}{\Delta t}
$$

for example assuming $K_p$ = -5, $K_i$ = -2, $K_d$ = 0, $\Delta t$ = 1, Set
point = 83.

| Time | Input | $e(t)$ | Accumulated integral ($K_i \cdot \sum_{\tau=0}^{t} e(\tau)\\cdot\Delta t$) | Output                                                  |
| ---- | ----- | ------ | -------------------------------------------------------------------------- | ------------------------------------------------------- |
| `0`  | `85`  | $-2$   | $4$                                                                        | $K_p(e) + \text{Accumulated integral} = -5(e) + 4 = 14$ |
| `1`  | `85`  | $-2$   | $4 + 4 = 8$                                                                | $-5(e) + 8 = 18$                                        |
| `2`  | `85`  | $-2$   | $8 + 4 = 12$                                                               | $-5(e) + 12 = 22$                                       |
| `3`  | `85`  | $-2$   | $12 + 4 = 16$                                                              | $-5(e) + 16 = 26$                                       |
| `4`  | `85`  | $-2$   | $16 + 4 = 20$                                                              | $-5(e) + 20 = 30$                                       |
| `5`  | `84`  | $-1$   | $20 + 2 = 22$                                                              | $-5(e) + 22 = -5(83 - 84) + 22 = 27$                    |
| `6`  | `83`  | $0$    | $22 + 0 = 22$                                                              | $-5(e) + 22 = -5(83 - 83) + 22 = 22$                    |
| `7`  | `82`  | $1$    | $22 - 2 = 20$                                                              | $-5(e) + 20 = -5(83 - 82) + 20 = 15$                    |
| `8`  | `81`  | $2$    | $20 - 4 = 16$                                                              | $-5(e) + 16 = -5(83 - 81) + 16 = 6$                     |
| `9`  | `82`  | $1$    | $16 - 2 = 14$                                                              | $-5(e) + 16 = -5(83 - 82) + 14 = 9$                     |

## Understanding and Preventing Integral Wind-up

Let’s take a look at the integral term. If the input remains greater than or
less than the Setpoint for an extended period, the integral term will
continuously accumulate.

$$
K_i \cdot \sum_{\tau=0}^{t} e(\tau)\\cdot\Delta t = K_i \cdot \big[ e(t) + e(t-\Delta t) + e(t-2\cdot \Delta t) + \cdots + e(0) \big] \cdot \Delta t
$$

For example assuming Setpoint = 83, $K_p$ = -5, $K_i$ = -2, $K_d$ = 0,
$\Delta t$ = 1.

| Time           | Input | $e(t)$ | Accumulated integral ($K_i \cdot \sum_{\tau=0}^{t} e(\tau)\\cdot\Delta t$) | Output                                                  |
| -------------- | ----- | ------ | -------------------------------------------------------------------------- | ------------------------------------------------------- |
| `0`            | `85`  | $-2$   | $4$                                                                        | $K_p(e) + \text{Accumulated integral} = -5(e) + 4 = 14$ |
| `1`            | `85`  | $-2$   | $4 + 4 = 8$                                                                | $-5(e) + 8 = 18$                                        |
| `2`            | `85`  | $-2$   | $8 + 4 = 12$                                                               | $-5(e) + 12 = 22$                                       |
| `3`            | `85`  | $-2$   | $12 + 4 = 16$                                                              | $-5(e) + 16 = 26$                                       |
| `4`            | `85`  | $-2$   | $16 + 4 = 20$                                                              | $-5(e) + 20 = 30$                                       |
| `long time...` | `85`  | $-2$   | $∞$                                                                        | $-5(e) + ∞ = ∞$                                         |

As a result, the output tends toward infinity (∞). Once this happens, even if
the input begins to decrease, the output remains saturated and cannot be reduced
effectively.

In closed-loop operation, the input is expected to converge to the Setpoint, but
integral wind-up can hinder this convergence.

To prevent the integral wind-up phenomenon, you can configure the parameters
ILimitMax and ILimitMin in the D-Bus JSON file to define the upper and lower
boundary conditions. For example: `ILimitMax` = 100, `ILimitMin` = 0.

## Hysteresis Mechanism

To prevent continuous output oscillation caused by small input fluctuations, a
configuration parameter called "`checkHysterWithSetpt`": true is introduced.

With this setting enabled, (Setpoint - NegativeHysteresis) is treated as the
lower bound of the hysteresis band, and (Setpoint + PositiveHysteresis) is
treated as the upper bound. While the input remains within this band, the
controller maintains the previous output instead of recalculating it.

For example assuming Setpoint = 83, $K_p$ = -5, $K_i$ = -2, $K_d$ = 0,
$\Delta t$ = 1, NegativeHysteresis = 1, PositiveHysteresis = 1.

| Time | Input                   | $e(t)$ | Accumulated integral ($K_i \cdot \sum_{\tau=0}^{t} e(\tau)\\cdot\Delta t$) | Output                                                  |
| ---- | ----------------------- | ------ | -------------------------------------------------------------------------- | ------------------------------------------------------- |
| `0`  | `85`                    | $-2$   | $4$                                                                        | $K_p(e) + \text{Accumulated integral} = -5(e) + 4 = 14$ |
| `1`  | `85`                    | $-2$   | $4 + 4 = 8$                                                                | $-5(e) + 8 = 18$                                        |
| `2`  | `85`                    | $-2$   | $8 + 4 = 12$                                                               | $-5(e) + 12 = 22$                                       |
| `3`  | `85`                    | $-2$   | $12 + 4 = 16$                                                              | $-5(e) + 16 = 26$                                       |
| `4`  | `85`                    | $-2$   | $16 + 4 = 20$                                                              | $-5(e) + 20 = 30$                                       |
| `5`  | `84` In hysteresis band | $-1$   | $20$ (same as previous)                                                    | $30$ (same as previous)                                 |
| `6`  | `83` In hysteresis band | $0$    | $20$ (same as previous)                                                    | $30$ (same as previous)                                 |
| `7`  | `82` In hysteresis band | $1$    | $20$ (same as previous)                                                    | $30$ (same as previous)                                 |
| `8`  | `81`                    | $2$    | $20 - 4 = 16$                                                              | $-5(e) + 16 = -5(83 - 81) + 16 = 6$                     |
| `9`  | `82` In hysteresis band | $1$    | $16$ (same as previous)                                                    | $6$ (same as previous)                                  |

## Dynamic Setpoint Behavior

To reduce output oscillation caused by the input hovering around the hysteresis
band, the concept of dynamic setpoint, a configuration parameter called
"`CheckDynamicSetpoint`": true is introduced.

It effectively minimizes large fluctuations in the proportional (P) term.

$$
e(t) =
\begin{cases}
SP + PositiveHysteresis - input, & \text{input} > \text{SP} + \text{PositiveHysteresis (upper bound)},\\
SP - NegativeHysteresis - input, & \text{input} < \text{SP} - \text{NegativeHysteresis (lower bound)},\\
SP - input, & \text{SP} - \text{NegativeHysteresis} \le \text{input} \le \text{SP} + \text{PositiveHysteresis (hysteresis band)}.
\end{cases}
$$

The following table compares the behavior between Fixed Setpoint and Dynamic
Setpoint.

For example assuming Setpoint = 83, $K_p$ = -5, $K_i$ = -2, $K_d$ = 0,
$\Delta t$ = 1, NegativeHysteresis = 1, PositiveHysteresis = 1.

| Time | Input                   | $e(t)$ (fixed SP) | Pterm(fixed SP) | $e(t)$ (dynamic SP) | Pterm(dynamic SP) |
| ---- | ----------------------- | ----------------- | --------------- | ------------------- | ----------------- |
| `0`  | `85` In upper bound     | $-2$              | $-2K_p$         | $-1$                | $-K_p$            |
| `1`  | `85` In upper bound     | $-2$              | $-2K_p$         | $-1$                | $-K_p$            |
| `2`  | `85` In upper bound     | $-2$              | $-2K_p$         | $-1$                | $-K_p$            |
| `3`  | `85` In upper bound     | $-2$              | $-2K_p$         | $-1$                | $-K_p$            |
| `4`  | `85` In upper bound     | $-2$              | $-2K_p$         | $-1$                | $-K_p$            |
| `5`  | `84` In hysteresis band | $1$               | $K_p$           | $0$                 | $0$               |
| `6`  | `83` In hysteresis band | $0$               | $0$             | $0$                 | $0$               |
| `7`  | `82` In hysteresis band | $1$               | $K_p$           | $0$                 | $0$               |
| `8`  | `81` In lower bound     | $2$               | $2K_p$          | $1$                 | $K_p$             |
| `9`  | `82` In hysteresis band | $1$               | $K_p$           | $0$                 | $0$               |

We can observe from Time 4 to Time 8 (when the input enters and then leaves the
hysteresis band):

For the Fixed Setpoint, the proportional term changes from $-2K_p$ to $2K_p$,
resulting in a total variation of $4K_p$.

For the Dynamic Setpoint, the proportional term changes from $-K_p$ to $K_p$,
resulting in a total variation of only $2K_p$.

If Dynamic Setpoint is used together with Hysteresis with Setpoint, the detailed
output behavior is as follows:

| Time | Input                   | $e(t)$ | Accumulated integral ($K_i \cdot \sum_{\tau=0}^{t} e(\tau)\\cdot\Delta t$) | Output                                                 |
| ---- | ----------------------- | ------ | -------------------------------------------------------------------------- | ------------------------------------------------------ |
| `0`  | `85` In upper bound     | $-1$   | $2$                                                                        | $K_p(e) + \text{Accumulated integral} = -5(e) + 2 = 7$ |
| `1`  | `85` In upper bound     | $-1$   | $2 + 2 = 4$                                                                | $-5(e) + 4 = 9$                                        |
| `2`  | `85` In upper bound     | $-1$   | $4 + 2 = 6$                                                                | $-5(e) + 6 = 11$                                       |
| `3`  | `85` In upper bound     | $-1$   | $6 + 2 = 8$                                                                | $-5(e) + 8 = 13$                                       |
| `4`  | `85` In upper bound     | $-1$   | $8 + 2 = 10$                                                               | $-5(e) + 10 = 15$                                      |
| `5`  | `84` In hysteresis band | $0$    | $10$ (same as previous)                                                    | $15$ (same as previous)                                |
| `6`  | `83` In hysteresis band | $0$    | $10$ (same as previous)                                                    | $15$ (same as previous)                                |
| `7`  | `82` In hysteresis band | $0$    | $10$ (same as previous)                                                    | $15$ (same as previous)                                |
| `8`  | `81` In lower bound     | $1$    | $10 - 2 = 8$                                                               | $-5(e) + 8 = -5(82 - 81) + 8 = 3$                      |
| `9`  | `82` In hysteresis band | $0$    | $8$ (same as previous)                                                     | $3$ (same as previous)                                 |
