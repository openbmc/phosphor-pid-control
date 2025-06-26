#pragma once

#include <exception>
#include <string>

class SensorBuildException : public std::exception
{
  public:
    explicit SensorBuildException(const std::string& message) : message(message)
    {}

    const char* what() const noexcept override
    {
        return message.c_str();
    }

  private:
    std::string message;
};

class ControllerBuildException : public std::exception
{
  public:
    explicit ControllerBuildException(const std::string& message) :
        message(message)
    {}

    const char* what() const noexcept override
    {
        return message.c_str();
    }

  private:
    std::string message;
};

class ConfigurationException : public std::exception
{
  public:
    explicit ConfigurationException(const std::string& message) :
        message(message)
    {}

    const char* what() const noexcept override
    {
        return message.c_str();
    }

  private:
    std::string message;
};
