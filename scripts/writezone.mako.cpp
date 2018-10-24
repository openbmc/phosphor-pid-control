## This file is a template.  The comment below is emitted
## into the rendered file; feel free to edit this file.

// !!! WARNING: This is GENERATED Code... Please do NOT edit !!!

#include <map>
#include "conf.hpp"

std::map<int64_t, struct zone> zoneDetailsConfig = {
% for zone in ZoneDict.iterkeys():
   % if zone:
   <%
           zConf = ZoneDict[zone]
           min = zConf["minthermalrpm"]
           percent = zConf["failsafepercent"]
   %>
    {${zone},
        {${min}, ${percent}},
    },
   % endif
% endfor
};
