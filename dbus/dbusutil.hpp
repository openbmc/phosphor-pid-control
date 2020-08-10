#pragma once

#include <exception>
#include <string>

namespace pid_control
{

struct VariantToDoubleVisitor
{
    template <typename T>
    std::enable_if_t<std::is_arithmetic<T>::value, double>
        operator()(const T& t) const
    {
        return static_cast<double>(t);
    }

    template <typename T>
    std::enable_if_t<!std::is_arithmetic<T>::value, double>
        operator()(const T& t) const
    {
        throw std::invalid_argument("Cannot translate type to double");
    }
};

std::string getSensorPath(const std::string& type, const std::string& id);
std::string getMatch(const std::string& type, const std::string& id);
void scaleSensorReading(const double min, const double max, double& value);
bool validType(const std::string& type);

} // namespace pid_control
