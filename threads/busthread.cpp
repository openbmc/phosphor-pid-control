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

#include "busthread.hpp"

#include <string>

void BusThread(struct ThreadParams& params)
{
    if (params.name.length() > 0)
    {
        params.bus.request_name(params.name.c_str());
    }

    while (true)
    {
        params.bus.process_discard();
        /* block indefinitely for updates. */
        params.bus.wait();
    }

    return;
}
