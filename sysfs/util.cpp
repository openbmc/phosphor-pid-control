// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright 2017 Google Inc

#include "util.hpp"

#include <filesystem>
#include <string>

namespace pid_control
{

/*
 * Replace "**" in the provided path string with an appropriate concrete path
 * component by scanning entries under the base path and returning the first
 * candidate whose full path exists.
 */

namespace fs = std::filesystem;

std::string FixupPath(std::string original)
{
    std::string::size_type n;

    /* TODO: Consider the merits of using regex for this. */
    n = original.find("**");

    if (n != std::string::npos)
    {
        std::string base = original.substr(0, n);
        std::string suffix = original.substr(n + 2);

        /* Try each subdirectory; return the first whose full path exists. */
        for (const auto& entry : fs::directory_iterator(base))
        {
            std::string candidate = entry.path().string() + suffix;
            if (fs::exists(candidate))
            {
                return candidate;
            }
        }

        /* No valid candidate found; return original and let the caller fail. */
        return original;
    }

    /* It'll throw an exception when we use it if it's still bad. */
    return original;
}

} // namespace pid_control
