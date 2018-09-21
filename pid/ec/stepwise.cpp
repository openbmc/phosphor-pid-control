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
float stepwise(const ec::StepwiseInfo& info, float input)
{
    float value = info.output[0]; // if we are below the lowest
                                  // point, we set the lowest value

    for (size_t ii = 1; ii < ec::maxStepwisePoints; ii++)
    {

        if (std::isnan(info.reading[ii]))
        {
            break;
        }
        if (info.reading[ii] > input)
        {
            break;
        }
        value = info.output[ii];
    }

    return value;
}
} // namespace ec