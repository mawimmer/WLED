#pragma once
#include <Arduino.h>

#ifdef WOKWI_SIM
    #define Serial Serial0
    #include <wled.h>
#else
    #include <wled.h>
#endif

namespace WLED_Bridge {

    inline void toggleSegment(int8_t segmentID) {
        Segment& seg = strip.getSegment(segmentID);

        if(seg.getOption(SEG_OPTION_ON)) {
            seg.setOption(SEG_OPTION_ON, false);
            stateUpdated(CALL_MODE_BUTTON);
            //updateInterfaces(CALL_MODE_BUTTON);
        } else {
            seg.setOption(SEG_OPTION_ON, true);
            stateUpdated(CALL_MODE_BUTTON);
            //updateInterfaces(CALL_MODE_BUTTON);

        }
    }

    inline void setSegmentBrightness(int8_t segmentID, int brightness) {
        Segment& seg = strip.getSegment(segmentID);
        seg.opacity = brightness;
        seg.setOpacity(brightness);
        stateUpdated(CALL_MODE_BUTTON);
        //updateInterfaces(CALL_MODE_BUTTON);

    }

    inline void setSegmentEffect(int8_t segmentID, int8_t effectID) {
        Segment& seg = strip.getSegment(segmentID);
        seg.setMode(effectID);
        //stateUpdated(CALL_MODE_BUTTON);
        //updateInterfaces(CALL_MODE_BUTTON);
        colorUpdated(CALL_MODE_BUTTON);

    }

    inline void setSegmentEffectConfig(int8_t segmentID, int8_t configIndex, int8_t deltaValue) {
        Segment& seg = strip.getSegment(segmentID);
     
        switch(configIndex) {
            case 0: seg.speed += deltaValue; break;
            case 1: seg.intensity += deltaValue; break;
            case 2: seg.opacity += deltaValue; break;
            case 3: seg.custom1 += deltaValue; break;
            case 4: seg.custom2 += deltaValue; break;
            case 5: seg.custom3 += deltaValue; break;
            default: return;
        }
        

        stateUpdated(CALL_MODE_BUTTON);
        //updateInterfaces(CALL_MODE_BUTTON);
    }
};



