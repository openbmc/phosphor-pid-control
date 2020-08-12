#pragma once

#include <exception>
#include <string>

class SensorBuildException : public std::exception
{
  public:
    explicit SensorBuildException(const std::string& message) :
        _message(message)
    {}

    virtual const char* what() const noexcept override
    {
        return _message.c_str();
    }

  private:
    std::string _message;
};

class ControllerBuildException : public std::exception
{
  public:
    explicit ControllerBuildException(const std::string& message) :
        _message(message)
    {}

    virtual const char* what() const noexcept override
    {
        return _message.c_str();
    }

  private:
    std::string _message;
};

class ConfigurationException : public std::exception
{
  public:
    explicit ConfigurationException(const std::string& message) :
        _message(message)
    {}

    virtual const char* what() const noexcept override
    {
        return _message.c_str();
    }

  private:
    std::string _message;
};
