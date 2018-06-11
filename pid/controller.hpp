#pragma once

#include <memory>
#include <vector>

#include "fan.hpp"
#include "ec/pid.hpp"

class PIDZone;

/*
 * Base class for PID controllers.  Each PID that implements this needs to
 * provide an input_proc, setpt_proc, and output_proc.
 */
class PIDController
{
    public:
        PIDController(const std::string& id, PIDZone* owner)
            : _owner(owner),
              _id(id)
        { }

        virtual ~PIDController() { }

        virtual float input_proc(void) = 0;
        virtual float setpt_proc(void) = 0;
        virtual void output_proc(float value) = 0;

        void pid_process(void);

        std::string get_id(void)
        {
            return _id;
        }
        float get_setpoint(void)
        {
            return _setpoint;
        }
        void set_setpoint(float setpoint)
        {
            _setpoint = setpoint;
        }

        ec::pid_info_t* get_pid_info(void)
        {
            return &_pid_info;
        }

    protected:
        PIDZone* _owner;

    private:
        // parameters
        ec::pid_info_t  _pid_info;
        float           _setpoint;
        std::string     _id;
};

