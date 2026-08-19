# 增量式 PID 改造规格说明书

## 1. 目标与范围

### 1.1 目标

将 `phosphor-pid-control` 模块中的 PID 控制器从**位置式（positional）**实现改造为支持**增量式（incremental/velocity）**计算方式，通过配置开关实现两种模式的兼容共存。

### 1.2 范围

| 包含 | 不包含 |
|------|--------|
| `pid/ec/pid.cpp` 核心 PID 算法 | Stepwise 控制器 |
| `pid/ec/pid.hpp` 数据结构 | Fan 控制器逻辑 |
| `pid/pidcontroller.cpp` 迟滞适配 | Thermal 控制器逻辑 |
| `pid/util.cpp` 初始化/调试函数 | D-Bus 通信层 |
| `pid/buildjson.cpp` JSON 配置解析 | IPMI 接口 |
| `conf.hpp` 配置结构体 | 传感器读取/写入 |
| `pid/ec/logging.hpp/cpp` 日志适配 | 构建系统（除测试注册外） |

### 1.3 向后兼容

- **默认模式为位置式**（`incrementalMode = false`），所有现有配置无需修改即可正常运行
- 现有测试全部保持 PASS
- JSON 配置中 `incrementalMode` 为可选字段，缺失时默认为 `false`

---

## 2. 数学定义

### 2.1 位置式 PID（现有实现）

```
error[k] = setpoint[k] - input[k]

proportionalTerm = Kp * error[k]
integralTerm    = integral[k-1] + Ki * error[k] * ts
derivativeTerm  = Kd * (error[k] - error[k-1]) / ts
feedFwdTerm     = (setpoint[k] + feedFwdOffset) * feedFwdGain

output[k] = proportionalTerm + integralTerm + derivativeTerm + feedFwdTerm
```

### 2.2 增量式 PID（新增实现）

```
error[k] = setpoint[k] - input[k]

Δproportional = Kp * (error[k] - error[k-1])
Δintegral     = Ki * error[k] * ts
Δderivative   = -Kd * (input[k] - 2*input[k-1] + input[k-2]) / ts
ΔfeedFwd      = FF[k] - FF[k-1]
  其中 FF[k] = (setpoint[k] + feedFwdOffset) * feedFwdGain

Δoutput = Δproportional + Δintegral + Δderivative + ΔfeedFwd
output[k] = output[k-1] + Δoutput
```

**微分项说明**：微分项基于 `input`（温度）而非 `error` 计算，以避免 setpoint 变化时的"微分冲击"（derivative kick）。当 setpoint 不变时，`Δinput = -Δerror`，因此基于 input 的微分与基于 error 的微分等价（符号相反，由 `-Kd` 补偿）。

### 2.3 首次调用处理

当 `initialized == false` 时：

```
error[k-1] = 0.0
output[k-1] = 0.0
FF[k-1] = 0.0
```

**微分项回退**：首次调用时没有有效的 `input[k-1]` 和 `input[k-2]` 历史，input-based 微分公式无法使用。此时回退为 error-based 微分：

```
Δderivative = Kd * error[k] / ts
```

这与位置式首次调用的 `Kd * (error[k] - 0) / ts` 完全一致，保证 TC07 等价性在第二次迭代仍然成立。

**状态初始化**：首次调用后，`lastLastInput` 初始化为 `setpoint`（而非 0.0）。这是因为位置式的 `lastError=0` 隐含 `lastInput = setpoint`（因为 `error = setpoint - input`）。将 `lastLastInput` 设为 `setpoint` 使 input-based 微分在第二次调用时与 error-based 微分保持一致。

首次调用后设置 `initialized = true`。

### 2.4 输出限幅（Output Clamp）

```
if (output[k] > outLim.max) {
    output[k] = outLim.max;
    // Back calculation: 调整 integral 防止积分饱和
    integral = outLim.max - (proportionalTerm + derivativeTerm + feedFwdTerm);
    integral = clamp(integral, integralLimit.min, integralLimit.max);
}
if (output[k] < outLim.min) {
    output[k] = outLim.min;
    // Back calculation
    integral = outLim.min - (proportionalTerm + derivativeTerm + feedFwdTerm);
    integral = clamp(integral, integralLimit.min, integralLimit.max);
}
```

### 2.5 积分限幅（Integral Clamp）

```
integral = clamp(integral, integralLimit.min, integralLimit.max);
```

### 2.6 Slew Rate 限幅

增量式下 slew rate 限制 output 相对于 lastOutput 的变化量：

```
if (slewNeg != 0.0) {
    minOut = lastOutput + slewNeg * ts;
    if (output < minOut) {
        output = minOut;
    }
}
if (slewPos != 0.0) {
    maxOut = lastOutput + slewPos * ts;
    if (output > maxOut) {
        output = maxOut;
    }
}
```

**Slew rate 反算**：当 slew rate 激活时，需要执行积分反算以匹配位置式行为：

```
if (slewNeg != 0.0 || slewPos != 0.0) {
    integral = output - proportionalTerm;
    integral = clamp(integral, integralLimit.min, integralLimit.max);
}
```

其中 `proportionalTerm = Kp * error[k]`。这与位置式在 slew rate 激活时的 `integralTerm = output - proportionalTerm` 逻辑一致。

### 2.7 增量式与位置式等价性

在无输出限幅、无积分限幅、无 slew rate 限制的理想条件下，增量式与位置式的输出序列完全一致。证明：

```
位置式: output[k] = P[k] + I[k] + D[k] + FF[k]
增量式: output[k] = output[k-1] + ΔP + ΔI + ΔD + ΔFF

其中:
  output[k-1] = P[k-1] + I[k-1] + D[k-1] + FF[k-1]
  ΔP = P[k] - P[k-1]
  ΔI = I[k] - I[k-1]  (因为 Δintegral = Ki*e[k]*ts = I[k] - I[k-1])
  ΔD = D[k] - D[k-1]
  ΔFF = FF[k] - FF[k-1]

因此: output[k] = P[k] + I[k] + D[k] + FF[k]  ✓
```

---

## 3. 数据结构变更

### 3.1 `pid/ec/pid.hpp` — `pid_info_t`

新增字段：

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `lastInput` | `double` | `0.0` | 存储 `input[k-1]`，用于增量式 D 项计算 |
| `lastLastInput` | `double` | `0.0` | 存储 `input[k-2]`，用于增量式 D 项计算 |
| `lastSetpoint` | `double` | `0.0` | 存储上次 setpoint，用于增量式 FF 项计算 |
| `incrementalMode` | `bool` | `false` | 配置开关：`true`=增量式，`false`=位置式 |

完整结构体：

```cpp
struct pid_info_t
{
    bool initialized = false;
    bool checkHysterWithSetpt = false;

    double ts = 0.0;
    double integral = 0.0;
    double lastOutput = 0.0;
    double lastError = 0.0;
    double lastInput = 0.0;           // 新增：input[k-1]
    double lastLastInput = 0.0;       // 新增：input[k-2]
    double lastSetpoint = 0.0;        // 新增：上次 setpoint
    bool incrementalMode = false;     // 新增：增量式开关

    double proportionalCoeff = 0.0;
    double integralCoeff = 0.0;
    double derivativeCoeff = 0.0;
    double feedFwdOffset = 0.0;
    double feedFwdGain = 0.0;

    limits_t integralLimit;
    limits_t outLim;
    double slewNeg = 0.0;
    double slewPos = 0.0;
    double positiveHysteresis = 0.0;
    double negativeHysteresis = 0.0;
};
```

### 3.2 `pid/ec/pid.hpp` — `pidinfo`（配置用精简结构体）

新增字段：

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `incrementalMode` | `bool` | `false` | 配置开关 |

### 3.3 `conf.hpp` — `ControllerInfo`

无需修改。`ControllerInfo` 包含 `ec::pidinfo pidInfo`，后者新增字段自动生效。

---

## 4. 接口契约

### 4.1 `ec::pid()` — 入口函数

**签名不变**：
```cpp
double pid(pid_info_t* pidinfoptr, double input, double setpoint,
           const std::string* nameptr = nullptr);
```

**行为**：
- 若 `pidinfoptr->incrementalMode == false`：调用现有位置式算法（行为不变）
- 若 `pidinfoptr->incrementalMode == true`：调用新增增量式算法

### 4.2 `ec::pid_incremental()` — 新增增量式函数

**签名**：
```cpp
double pid_incremental(pid_info_t* pidinfoptr, double input, double setpoint);
```

**输入**：
- `pidinfoptr`：PID 状态结构体指针（包含所有系数和状态）
- `input`：当前传感器输入值
- `setpoint`：当前设定点

**输出**：
- 返回计算后的输出值 `output[k]`

**副作用**：
- 更新 `pidinfoptr->lastError` = `error[k]`
- 更新 `pidinfoptr->lastInput` = `input[k]`
- 更新 `pidinfoptr->lastLastInput` = `input[k-1]`
- 更新 `pidinfoptr->lastOutput` = `output[k]`
- 更新 `pidinfoptr->lastSetpoint` = `setpoint[k]`
- 更新 `pidinfoptr->integral`（用于 back calculation 和日志）
- 设置 `pidinfoptr->initialized = true`

### 4.3 `PidCoreContext` — 日志上下文

增量式下 `PidCoreContext` 各字段含义：

| 字段 | 增量式含义 |
|------|-----------|
| `proportionalTerm` | `Δproportional`（比例增量） |
| `integralTerm` | `Δintegral`（积分增量） |
| `derivativeTerm` | `Δderivative`（微分增量） |
| `feedFwdTerm` | `ΔfeedFwd`（前馈增量） |
| `output` | `output[k]`（最终输出，与位置式一致） |

---

## 5. 配置格式

### 5.1 JSON 配置扩展

在 PID 对象的 `pid` 子对象中新增可选字段 `incrementalMode`：

```json
{
    "zones": [{
        "id": 1,
        "pids": [{
            "name": "fan1",
            "type": "fan",
            "inputs": ["fan1_tach"],
            "setpoint": 10000,
            "pid": {
                "samplePeriod": 0.1,
                "proportionalCoeff": 0.5,
                "integralCoeff": 0.1,
                "feedFwdOffsetCoeff": 0.0,
                "feedFwdGainCoeff": 0.0,
                "integralLimit_min": -100,
                "integralLimit_max": 100,
                "outLim_min": 0,
                "outLim_max": 255,
                "slewNeg": -10,
                "slewPos": 10,
                "incrementalMode": true
            }
        }]
    }]
}
```

### 5.2 解析规则

- `incrementalMode` 为可选字段
- 缺失时默认为 `false`（位置式）
- 类型为 `bool`
- 仅对非 `stepwise` 类型的 PID 生效

---

## 6. 测试用例规格

### 6.1 测试文件

新增 `test/pid_ec_unittest.cpp`，使用 Google Test 框架。

### 6.2 测试用例

#### TC01: 增量式首次调用输出等于增量值

- **Given**: `incrementalMode=true`, `initialized=false`, `Kp=1.0`, `Ki=0`, `Kd=0`, `feedFwdGain=0`
- **When**: 首次调用 `pid()`, `input=0`, `setpoint=10`
- **Then**: `output = Kp * (10 - 0) = 10.0`（因为 `lastError=0`, `lastOutput=0`）

#### TC02: 增量式稳态输出不变

- **Given**: `incrementalMode=true`, 已运行至稳态 `input=10`, `setpoint=10`, `lastError=0`, `lastInput=10`, `lastLastInput=10`, `lastOutput=50`
- **When**: 再次调用 `pid()`, `input=10`, `setpoint=10`
- **Then**: `output = 50.0`（Δoutput = 0）

#### TC03: 增量式纯比例响应

- **Given**: `incrementalMode=true`, `Kp=2.0`, `Ki=0`, `Kd=0`, `feedFwdGain=0`, `lastError=0`, `lastOutput=0`
- **When**: 调用 `pid()`, `input=0`, `setpoint=5`
- **Then**: `output = 2.0 * (5 - 0) = 10.0`

#### TC04: 增量式纯积分响应

- **Given**: `incrementalMode=true`, `Kp=0`, `Ki=1.0`, `Kd=0`, `feedFwdGain=0`, `ts=0.1`, `lastOutput=0`
- **When**: 调用 `pid()`, `input=0`, `setpoint=10`
- **Then**: `output = 1.0 * 10 * 0.1 = 1.0`

#### TC05: 增量式纯微分响应（基于 input）

- **Given**: `incrementalMode=true`, `Kp=0`, `Ki=0`, `Kd=1.0`, `feedFwdGain=0`, `ts=0.1`, `lastInput=2`, `lastLastInput=0`, `lastOutput=0`
- **When**: 调用 `pid()`, `input=10`, `setpoint=10`（error=0）
- **Then**: `Δderivative = -1.0 * (10 - 2*2 + 0) / 0.1 = -60.0`, `output = 0 + (-60) = -60.0`

#### TC06: 增量式纯前馈响应

- **Given**: `incrementalMode=true`, `Kp=0`, `Ki=0`, `Kd=0`, `feedFwdGain=1.0`, `feedFwdOffset=0`, `lastSetpoint=0`, `lastOutput=0`
- **When**: 调用 `pid()`, `input=0`, `setpoint=10`
- **Then**: `output = (10+0)*1.0 - (0+0)*1.0 = 10.0`

#### TC07: 增量式与位置式等价—恒定 setpoint（无限制条件）

- **Given**: 两组相同的 `pid_info_t`（一组 `incrementalMode=true`，一组 `false`），相同系数，无 outLim/slew
- **When**: 使用相同的输入序列调用 10 次，**setpoint 保持恒定**
- **Then**: 每次输出值完全一致（浮点误差 < 1e-9）
- **注意**: 由于增量式微分项基于 input 而位置式基于 error，仅在 setpoint 不变时两者等价

#### TC08: 增量式输出限幅行为

- **Given**: `incrementalMode=true`，设置 `outLim.min=0, outLim.max=50`
- **When**: 使用输入序列调用 10 次，**setpoint 保持恒定**
- **Then**: 每次输出值均在 `[0, 50]` 范围内
- **注意**: 当输出被限幅钳位后，位置式从零重新计算（P+I+D+FF），而增量式从钳位后的 `lastOutput` 累加，两者产生永久性偏差。等价性仅在无限幅条件下成立（见 §2.7）

#### TC09: 增量式 slew rate 限幅行为

- **Given**: `incrementalMode=true`，设置 `slewNeg=-5, slewPos=5`
- **When**: 使用输入序列调用 10 次，**setpoint 保持恒定**
- **Then**: 每步输出变化量均在 `[-0.5, 0.5]`（即 `[slewNeg*ts, slewPos*ts]`）范围内
- **注意**: 当 slew rate 激活后，位置式和增量式的积分反算路径不同，两者输出会发散。等价性仅在无 slew rate 条件下成立（见 §2.7）

#### TC10: 增量式输出上限钳位

- **Given**: `incrementalMode=true`, `outLim.max=100`, `lastOutput=95`
- **When**: 调用产生 Δoutput=20（理论输出=115）
- **Then**: `output = 100.0`（被钳位）

#### TC11: 增量式输出下限钳位

- **Given**: `incrementalMode=true`, `outLim.min=0`, `lastOutput=5`
- **When**: 调用产生 Δoutput=-20（理论输出=-15）
- **Then**: `output = 0.0`（被钳位）

#### TC12: 增量式积分限幅

- **Given**: `incrementalMode=true`, `integralLimit.max=10`
- **When**: 积分项超过 10
- **Then**: 积分项被钳位在 10

#### TC13: 增量式正 slew rate 限幅

- **Given**: `incrementalMode=true`, `slewPos=2.0`, `ts=0.1`, `lastOutput=0`
- **When**: 调用产生 Δoutput=5.0
- **Then**: `Δoutput` 被限制为 `2.0 * 0.1 = 0.2`

#### TC14: 增量式负 slew rate 限幅

- **Given**: `incrementalMode=true`, `slewNeg=-2.0`, `ts=0.1`, `lastOutput=0`
- **When**: 调用产生 Δoutput=-5.0
- **Then**: `Δoutput` 被限制为 `-2.0 * 0.1 = -0.2`

#### TC15: 入口函数分发到位置式

- **Given**: `incrementalMode=false`
- **When**: 调用 `ec::pid()`
- **Then**: 输出与直接调用位置式算法一致

#### TC16: 入口函数分发到增量式

- **Given**: `incrementalMode=true`
- **When**: 调用 `ec::pid()`
- **Then**: 输出与直接调用增量式算法一致

#### TC17: 默认模式为位置式

- **Given**: 新创建的 `pid_info_t`（不设置 `incrementalMode`）
- **When**: 调用 `ec::pid()`
- **Then**: 使用位置式算法（向后兼容）

#### TC18: 模式切换—位置式到增量式

- **Given**: 位置式运行 3 轮后，设置 `incrementalMode=true`
- **When**: 继续运行
- **Then**: 状态正确传递，输出连续

#### TC19: 模式切换—增量式到位置式

- **Given**: 增量式运行 3 轮后，设置 `incrementalMode=false`
- **When**: 继续运行
- **Then**: 状态正确传递，输出连续

---

## 7. 文件变更清单

| 文件 | 操作 | 说明 |
|------|------|------|
| `docs/incremental_pid_spec.md` | **新增** | 本规格说明书 |
| `pid/ec/pid.hpp` | 修改 | `pid_info_t` 增加 4 个字段；`pidinfo` 增加 1 个字段 |
| `pid/ec/pid.cpp` | 修改 | 新增 `pid_incremental()` 函数；修改 `pid()` 入口分发 |
| `pid/util.cpp` | 修改 | `initializePIDStruct()` 和 `dumpPIDStruct()` 适配新字段 |
| `pid/buildjson.cpp` | 修改 | JSON 解析支持 `incrementalMode` |
| `pid/ec/logging.hpp` | 修改 | `PidCoreContext` 增加增量式相关字段（可选） |
| `pid/ec/logging.cpp` | 修改 | 日志输出适配增量式字段（可选） |
| `pid/pidcontroller.cpp` | 修改 | `calPIDOutput()` 增量式迟滞适配 |
| `test/pid_ec_unittest.cpp` | **新增** | 核心 PID 增量式单元测试 |
| `test/meson.build` | 修改 | 注册 `pid_ec_unittest` 测试目标 |
| `test/pid_thermalcontroller_unittest.cpp` | 修改 | 增加增量式迟滞测试变体 |

---

## 8. 实施步骤

```
步骤0: 编写 docs/incremental_pid_spec.md（本文件）✓

步骤1: 修改数据结构（编译通过，不影响行为）
       1a. pid/ec/pid.hpp — pid_info_t 增加 lastInput, lastLastInput, lastSetpoint, incrementalMode
       1b. conf.hpp — pidinfo 增加 incrementalMode（通过包含 pid.hpp 自动生效）
       1c. pid/util.cpp — initializePIDStruct/dumpPIDStruct 适配新字段

步骤2: 编写 test/pid_ec_unittest.cpp（编译通过，全部 FAIL）

步骤3: 实现增量式算法（测试变绿）
       3a. pid/ec/pid.cpp — 新增 ec::pid_incremental()
       3b. pid/ec/pid.cpp — 修改 ec::pid() 入口分发逻辑

步骤4: 适配 calPIDOutput 迟滞逻辑
       4a. pid/pidcontroller.cpp — 增量式下的迟滞行为
       4b. 更新 pid_thermalcontroller_unittest.cpp 中的迟滞测试

步骤5: 配置解析适配
       5a. pid/buildjson.cpp — JSON 解析支持 incrementalMode

步骤6: 日志适配（可选）
       6a. pid/ec/logging.hpp / logging.cpp

步骤7: 注册测试 & 全量回归
       7a. test/meson.build — 增加 pid_ec_unittest
       7b. 运行全量测试验证
