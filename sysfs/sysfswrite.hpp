#pragma once

#include <string>

#include "interfaces.hpp"
#include "sysfs/util.hpp"


/*
 * A WriteInterface that is expecting a path that's sysfs, but really could be
 * any filesystem path.
 */
class SysFsWritePercent : public WriteInterface
{
    public:
        SysFsWritePercent(std::string& writepath, int64_t min, int64_t max)
            : WriteInterface(min, max),
              _writepath(FixupPath(writepath))
        { }

        void write(double value) override;

    private:
        std::string _writepath;
};

class SysFsWrite : public WriteInterface
{
    public:
        SysFsWrite(std::string& writepath, int64_t min, int64_t max)
            : WriteInterface(min, max),
              _writepath(FixupPath(writepath))
        { }

        void write(double value) override;

    private:
        std::string _writepath;
};
