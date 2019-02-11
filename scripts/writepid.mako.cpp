## This file is a template.  The comment below is emitted
## into the rendered file; feel free to edit this file.

// !!! WARNING: This is GENERATED Code... Please do NOT edit !!!

#include <map>
#include "conf.hpp"

std::map<int64_t, PIDConf> zoneConfig = {
% for zone in PIDDict.iterkeys():
    % if zone:
    {${zone},
        {
            % for key, details in PIDDict[zone].iteritems():
            {"${key}",
                {"${details['type']}",
                 {
                   % for item in details['inputs'].split():
                   "${item}",
                   % endfor
                 },
                 <%
                      # If the PID type is a fan, setpoint field is unused,
                      # so just use a default of 0.  If the PID is not a type
                      # of fan, require the setpoint field.
                      if 'fan' == details['type']:
                          setpoint = 0
                      else:
                          setpoint = details['setpoint']

                      neg_hysteresis = 0
                      pos_hysteresis = 0
                      if 'neg_hysteresis' in details['pid']:
                          neg_hysteresis = details['pid']['neg_hysteresis']
                      if 'pos_hysteresis' in details['pid']:
                          pos_hysteresis = details['pid']['pos_hysteresis']
                  %>
                 ${setpoint},
                 {${details['pid']['sampleperiod']},
                  ${details['pid']['proportionalCoeff']},
                  ${details['pid']['integralCoeff']},
                  ${details['pid']['feedFwdOffOffsetCoeff']},
                  ${details['pid']['feedFwdGainCoeff']},
                  {${details['pid']['integralLimit']['min']}, ${details['pid']['integralLimit']['max']}},
                  {${details['pid']['outLimit']['min']}, ${details['pid']['outLimit']['max']}},
                  ${details['pid']['slewNeg']},
                  ${details['pid']['slewPos']},
                  ${neg_hysteresis},
                  ${pos_hysteresis}},
                },
            },
            % endfor
        },
    },
   % endif
% endfor
};
