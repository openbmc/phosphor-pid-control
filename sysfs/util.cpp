// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright 2017 Google Inc

#include "util.hpp"

#include <filesystem>
#include <string>

namespace pid_control
{

/*
 * Replace "**" in the provided path string with an appropriate concrete path
 * component (the first directory entry found, on the assumption that there
 * will only be a single candidate).
 */

namespace fs = std::filesystem;

std::string FixupPath(std::string original)
{
    std::string::size_type n;

    /* TODO: Consider the merits of using regex for this. */
    n = original.find("**");

    if (n != std::string::npos)
    {
        /* This path has some missing pieces and we support it. */
        std::string base = original.substr(0, n);
        std::string fldr;
        std::string f = original.substr(n + 2, original.size() - (n + 2));

        /* Equivalent to glob and grab 0th entry. */
        for (const auto& folder : fs::directory_iterator(base))
        {
            fldr = folder.path();
            break;
        }

        if (!fldr.length())
        {
            return original;
        }

        return fldr + f;
    }

    /* It'll throw an exception when we use it if it's still bad. */
    return original;
}

} // namespace pid_control
