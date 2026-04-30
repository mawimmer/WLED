#include <rotaryEncoder.h>
#include <multiple_rotary_encoder.h>
#include <wled_bridge.h>


#ifdef WOKWI_SIM
    #define Serial Serial0
    #include <wled.h>
#else
    #include <wled_bridge.h>
    #include <wled.h>
#endif

/**
 * Hardware Interrupt Function
 */
void IRAM_ATTR buttonISR(void* arg) {

    rotaryEncoder& Encoder = *static_cast<rotaryEncoder*>(arg);
    
    uint32_t now = xTaskGetTickCountFromISR() * portTICK_PERIOD_MS;
    
    if (now - Encoder.lastEdge < 50) {
        return;  // Ignore bounces
    }
    
    Encoder.lastEdge = now;
    
    if (!Encoder.buttonIsPressed) {
        // Button PRESSED
        Encoder.timeOfLastClick = now;  // Only set on initial press
        Encoder.buttonIsPressed = true;
        Encoder.buttonWasPressed = false;
        Encoder.buttonPressHandled = false;
        return;
    }
    
    if (Encoder.buttonIsPressed) {
        // Button RELEASED
        Encoder.buttonIsPressed = false;
        Encoder.buttonWasPressed = true;
        return;
    }
}

void rotaryEncoder::RotationEventHandler() {
    Serial.println("RotationEventHandler");
    eventRotation = false;

    switch (MODI) {
        case Rotary_Encoder_MODI::BRIGHTNESS_MODI:
            Brightness_Push();
            encoder_manager.updateDisplay(*this);
            break;

        case Rotary_Encoder_MODI::EFFECT_MODI:
            Effect_Push();
            encoder_manager.updateDisplay(*this);
            break;

        case Rotary_Encoder_MODI::TOGGLED_OFF:
            break;

        case Rotary_Encoder_MODI::EFFECT_CONFIG_MODI:
            //some config adjustment
            //setSegmentEffectConfig()
            Select_Effect_Config();
            encoder_manager.updateDisplay(*this);
            break;
        
        case Rotary_Encoder_MODI::EFFECT_CONFIG_SELECTED_MODI:
            Effect_Config_Push();
            break;

    }
    deltaValue = 0;

}

void rotaryEncoder::ButtonEventHandler() {
    Serial.println("ButtonEventHandler");
    if (eventButton == ButtonEventType::SHORT_PRESS) {
        switch (MODI) {
            case Rotary_Encoder_MODI::TOGGLED_OFF: {
                encoder_manager.OnOffDisplay(*this);
                Serial.println("was TOGGLED OFF - now BRIGHTNESS");

                Segment& seg = strip.getSegment(segmentID);
                seg.setOption(SEG_OPTION_ON, true);
                stateUpdated(CALL_MODE_BUTTON);
                updateInterfaces(CALL_MODE_BUTTON);

                MODI = Rotary_Encoder_MODI::BRIGHTNESS_MODI;
                rotationDelay = BRIGHTNESS_ROTATION_DELAY;
                pcnt_counter_resume(unit);
                break;

            }
            case Rotary_Encoder_MODI::BRIGHTNESS_MODI:
                Serial.println("was BRIGHTNESS - now EFFECT");
                MODI = Rotary_Encoder_MODI::EFFECT_MODI;
                rotationDelay = EFFECT_ROTATION_DELAY;
                break;

            case Rotary_Encoder_MODI::EFFECT_MODI:
                Serial.println("was EFFECT- now BRIGHTNESS");
                MODI = Rotary_Encoder_MODI::BRIGHTNESS_MODI;
                rotationDelay = BRIGHTNESS_ROTATION_DELAY;
                break;
            
            case Rotary_Encoder_MODI::EFFECT_CONFIG_MODI:
                Serial.println("Config was selected");
                MODI = Rotary_Encoder_MODI::EFFECT_CONFIG_SELECTED_MODI;
                break;
            
            case Rotary_Encoder_MODI::EFFECT_CONFIG_SELECTED_MODI:
                Serial.println("Leaving selected config back to config selection");
                MODI = Rotary_Encoder_MODI::EFFECT_CONFIG_MODI;
                break;

        }

    } else if (eventButton == ButtonEventType::LONG_PRESS && MODI != Rotary_Encoder_MODI::TOGGLED_OFF) {
        Serial.println("LONG PRESS - Toggling OFF");
        Segment& seg = strip.getSegment(segmentID);
        seg.setOption(SEG_OPTION_ON, false);
        stateUpdated(CALL_MODE_BUTTON);
        updateInterfaces(CALL_MODE_BUTTON);
        MODI = Rotary_Encoder_MODI::TOGGLED_OFF;
        encoder_manager.OnOffDisplay(*this);
        pcnt_counter_pause(unit);

    } else if (eventButton == ButtonEventType::DOUBLE_PRESS) {
        switch (MODI) {
            case Rotary_Encoder_MODI::EFFECT_MODI:
                MODI = Rotary_Encoder_MODI::EFFECT_CONFIG_MODI;
                break;

            case Rotary_Encoder_MODI::EFFECT_CONFIG_MODI:
                MODI = Rotary_Encoder_MODI::EFFECT_MODI;
                break;

        }
    }
    eventButton = ButtonEventType::NONE;

}

void rotaryEncoder::Brightness_Push() {
    Serial.println("Brightness_Push");
    float norm = (float)brightness / 255.0f;
    float factor = 0.1f + 1.5f * norm * norm;
    int32_t step = 1 + (int32_t)(factor * 20);
    int32_t change = deltaValue * step;
    int32_t newDuty = brightness + change;

    if (newDuty < 1) newDuty = 1;
    if (newDuty > 255) newDuty = 255;

    brightness = newDuty;

    //strip.setBrightness(brightness, false);

    WLED_Bridge::setSegmentBrightness(segmentID, brightness);

}

void rotaryEncoder::Effect_Config_Push() {
    Serial.println("Effect_Config_Push");

    int32_t newEffect = effect + (deltaValue / pulsesPerDetent);

    if (newEffect < 1) newEffect = 1;
    if (newEffect > 255) newEffect = 255;

    //effect = newEffect;

    //strip.setBrightness(brightness, false);

    int8_t configIndex = static_cast<int>(selectedSegConfig);

    WLED_Bridge::setSegmentEffectConfig(segmentID, configIndex, effect);
    
}



void rotaryEncoder::Select_Effect_Config() {
    Serial.println("Select_Effect_Config");

    int8_t configIndex = static_cast<int>(selectedSegConfig);

    Serial.println(configIndex);

    configIndex += (deltaValue / pulsesPerDetent);

    if(configIndex > 5) {
        configIndex = 5;
    } else if(configIndex < 0) {
        configIndex = 0;
    }
    Serial.println(configIndex);
    selectedSegConfig = static_cast<selectedSegmentConfig>(configIndex);
}


void rotaryEncoder::Effect_Push() {
    Serial.println("Effect_Push");

    int32_t newEffectValue = effect + (deltaValue / pulsesPerDetent);

    if (newEffectValue < 1) newEffectValue = 1;
    if (newEffectValue > 255) newEffectValue = 255;

    effect = newEffectValue;

    //strip.setBrightness(brightness, false);
    char effectName[33];
    extractModeName(effect,JSON_mode_names , effectName, 32);
    Serial.println(effectName);
    WLED_Bridge::setSegmentEffectConfig(segmentID, effect, effect);
    
}

void rotaryEncoder::updatePCNT_Unit(int i) {
    int16_t ValueNOW;
    pcnt_get_counter_value(unit, &ValueNOW);

    if (ValueNOW) {
        pcnt_counter_clear(unit);
        deltaValue += ValueNOW;
        rotationPending = true;
        timeOfLastRotation = xTaskGetTickCount() * portTICK_PERIOD_MS;
        Serial.printf("Encoder %d has a NEW Value: %d .Time of last Rotation: %d \r\n", i, ValueNOW, timeOfLastRotation);
    }

    if (rotationPending && (xTaskGetTickCount() * portTICK_PERIOD_MS) - timeOfLastRotation >= rotationDelay) {
        rotationPending = false;
        eventRotation = true;
        stateChanged = true;
        encoder_manager.global_eventPending = true;
        Serial.printf("Encoder %d has a CONFIRMED Delta: %d \r\n", i, deltaValue);
    }
}

void rotaryEncoder::updateButtonState(int32_t timeNOW) {
// If a falling Edge is detected from an Interrupt, the .buttonPressHandled Flag is set "false"
    if ( !buttonPressHandled ) {



        int32_t timeDifference = timeNOW - timeOfLastClick;

        //Serial.printf("timeNow: %d timeOfLastClick %d timeDifference: %d timeNow", timeNOW ,timeOfLastClick, timeDifference );
        //Serial.printf("DEBUG: pressed=%d, wasPressed=%d, timeDiff=%ld, threshold=%d\n", 
        //buttonIsPressed, buttonWasPressed, timeDifference, longShortPressThreshold);

        // Long Press Detection when Button is Held
        if( buttonIsPressed) {
            

            // If Pressed Longer or Equal to (const int longShortPressThreshold) -> Long Press
            if( timeDifference >= longShortPressThreshold ) {
                Serial.println("LONG PRESS HOLD");
                eventButton = ButtonEventType::LONG_PRESS;
                stateChanged = true;
                encoder_manager.global_eventPending = true;

                // Reset .buttonPressHandled to "true", so no more execution until next button press
                buttonPressHandled = true;

            };
        };

        // Long and Short Press Detection when Button is Released
        if( buttonWasPressed) {

            // If Pressed Shorter than (const int longShortPressThreshold) -> Short Press
            if( timeDifference < longShortPressThreshold ) {

                if(waitingForDoubleClick && (timeNOW - timeOfFirstClick <= doublePressThreshold)) {
                    // YEAH! DoubleClick!
                    eventButton = ButtonEventType::DOUBLE_PRESS;
                    waitingForDoubleClick = false;
                    buttonPressHandled = true;
                    Serial.println("double press!");
                    encoder_manager.global_eventPending = true;
                    return;
                } 
                if(!waitingForDoubleClick) {
                    Serial.println("waiting for doubleclick");
                    waitingForDoubleClick = true;
                    timeOfFirstClick = timeNOW;
                    buttonPressHandled = true;
                }

            // If Pressed Longer or Equal to (const int longShortPressThreshold) -> Long Press
            // Actual Edge Case - When CPU takes longer than 500ms to check the Button Press
            } else {
                Serial.println("LONG PRESS RELEASE");
                eventButton = ButtonEventType::LONG_PRESS;
                encoder_manager.global_eventPending = true;
                stateChanged = true;

                //      Reset .buttonPressHandled to "true", so no more execution until next button press
                buttonPressHandled = true;
            };

        };
    };
    if(waitingForDoubleClick && (timeNOW - timeOfFirstClick > doublePressThreshold)) {
        Serial.println("SHORT PRESS");
        eventButton = ButtonEventType::SHORT_PRESS;
        encoder_manager.global_eventPending = true;
        stateChanged = true;

        // Reset .buttonPressHandled to "true", so no more execution until next button press
        buttonPressHandled = true;
        waitingForDoubleClick = false; 
    }

}




