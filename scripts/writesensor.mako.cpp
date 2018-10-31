## This file is a template.  The comment below is emitted
## into the rendered file; feel free to edit this file.

// !!! WARNING: This is GENERATED Code... Please do NOT edit !!!

#include <map>
#include "conf.hpp"

std::map<std::string, struct SensorConfig> sensorConfig = {
% for key in sensorDict.iterkeys():
   % if key:
   <%
           sensor = sensorDict[key]
           type = sensor["type"]
           readpath = sensor["readpath"]
           writepath = sensor.get("writepath", "")
           min = sensor.get("min", 0)
           max = sensor.get("max", 0)
           # Presently only thermal inputs have their timeout
           # checked, but we should default it as 2s, which is
           # the previously hard-coded value.
           # If it's a fan sensor though, let's set the default
           # to 0.
           if type == "fan":
               timeout = sensor.get("timeout", 0)
           else:
               timeout = sensor.get("timeout", 2)
   %>
    {"${key}",
        {"${type}","${readpath}","${writepath}", ${min}, ${max}, ${timeout}},
    },
   % endif
% endfor
};


