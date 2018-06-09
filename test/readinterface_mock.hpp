#pragma once

#include "interfaces.hpp"

class ReadInterfaceMock : public ReadInterface
{
    public:
        virtual ~ReadInterfaceMock() = default;

        MOCK_METHOD0(read, ReadReturn());
};
