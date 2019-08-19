/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include "stepwise.hpp"

#include <cmath>
#include <cstddef>
#include <limits>

namespace ec
{
double stepwise(const ec::StepwiseInfo& info, double input)
{
    double value = info.output[0]; // if we are below the lowest
                                   // point, we set the lowest value
    if (input > info.reading[0])
    {
        for (size_t i = 1; i < info.reading.size(); ++i)
        {
            if (info.reading[i] > input)
            {
                break;
            }
            value = info.output[i];
        }
    }

    return value;
}

double linear(const ec::StepwiseInfo& info, double input)
{
    double value = info.output[0];

    // if input is higher than the max reading point
    // return the max value of output
    if (input >= info.reading.back())
    {
        value = info.output.back();
    }
    // if input is smaller than the minimum reading point
    // return the minimum value of output
    else if (input > info.reading[0])
    {
        for (size_t i = 1; i < info.reading.size(); ++i)
        {
            // DO Interpolation
            if (info.reading[i] > input)
            {
                double inputLow = info.reading[i - 1];
                double inputHigh = info.reading[i];
                double outputLow = info.output[i - 1];
                double outputHigh = info.output[i];
                value = outputLow +
                        ((outputHigh - outputLow) / (inputHigh - inputLow)) *
                            (input - inputLow);
                break;
            }
        }
    }

    return value;
}
} // namespace ec
