#pragma once

#include <exception>
#include <string>

class SensorBuildException : public std::exception
{
  public:
    SensorBuildException(const std::string& message) : message(message)
    {
    }

    virtual const char* what() const noexcept override
    {
        return message.c_str();
    }

  private:
    std::string message;
};
