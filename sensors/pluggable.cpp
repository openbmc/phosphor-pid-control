/**
 * Copyright 2017 Google Inc.
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

#include "pluggable.hpp"

#include "interfaces.hpp"

#include <cstdint>
#include <string>

namespace pid_control
{

ReadReturn PluggableSensor::read(void)
{
    return _reader->read();
}

void PluggableSensor::write(double value)
{
    _writer->write(value);
}

void PluggableSensor::write(double value, bool force, int64_t* written)
{
    _writer->write(value, force, written);
}

bool PluggableSensor::getFailed(void)
{
    return _reader->getFailed();
}

std::string PluggableSensor::getFailReason(void)
{
    return _reader->getFailReason();
}

} // namespace pid_control
