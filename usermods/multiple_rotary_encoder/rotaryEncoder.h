#pragma once
#include <Arduino.h>
#include <driver/pcnt.h>

#ifdef WOKWI_SIM
    #define Serial Serial0
#endif

void IRAM_ATTR buttonISR(void* arg);

class rotaryEncoder {

    public:

        /**
     * Hardware Info
     */
    pcnt_unit_t unit;
    gpio_num_t pin_clk;
    gpio_num_t pin_dt;
    gpio_num_t pin_sw;
    int brightness = 0;
    int effect = 0;
    //int effectValue = 0;
    int8_t segmentID;

    uint8_t selectedConfig = 0;

    int pulsesPerDetent = 2;

    int rotationDelay = 0;

    friend void IRAM_ATTR buttonISR(void* arg);
        /**
     * Click Modi
     */
    enum class Rotary_Encoder_MODI {
        BRIGHTNESS_MODI,
        EFFECT_MODI,
        EFFECT_CONFIG_MODI,
        EFFECT_CONFIG_SELECTED_MODI,
        TOGGLED_OFF
    };


    enum class ButtonEventType {
        NONE,
        SHORT_PRESS,
        DOUBLE_PRESS,
        LONG_PRESS,
    };

    enum class selectedSegmentConfig {
        SPEED,
        INTENSITY,
        OPACITY,
        CUSTOM1,
        CUSTOM2,
        CUSTOM3
    };

        /**
     * Click and Rotation Execution States
     */
    ButtonEventType eventButton = ButtonEventType::NONE;

    selectedSegmentConfig selectedSegConfig = selectedSegmentConfig::SPEED;

    bool eventRotation = false;

    rotaryEncoder() {}
    rotaryEncoder(pcnt_unit_t u, int clk, int dt, int sw, int8_t seg) : 
        unit(u), pin_clk(static_cast<gpio_num_t>(clk)), pin_dt(static_cast<gpio_num_t>(dt)), pin_sw(static_cast<gpio_num_t>(sw)), segmentID(seg){};

    private:


        bool stateChanged = false;

        int longShortPressThreshold = 500;
        int doublePressThreshold = 400;
        int BRIGHTNESS_ROTATION_DELAY = 40;
        int EFFECT_ROTATION_DELAY = 150;

        /**
         * Rotation Variables
         */
        int16_t deltaValue = 0;
        unsigned long timeOfLastRotation = 0;
        bool rotationPending = false;

        /**
         * Click Variables
         */
        volatile uint32_t timeOfLastClick = 0;
        volatile uint32_t lastEdge = 0;
        volatile bool buttonIsPressed = false;
        volatile bool buttonWasPressed = false;
        volatile bool buttonPressHandled = true;
        bool waitingForDoubleClick = false;
        u_int32_t timeOfFirstClick = 0;




        Rotary_Encoder_MODI MODI = Rotary_Encoder_MODI::TOGGLED_OFF;




        public:

        bool hasChanged() { return stateChanged; }
        
        void clearChangeFlag() { stateChanged = false; }

        void RotationEventHandler();

        void ButtonEventHandler();

        void Brightness_Push();

        void Effect_Push();

        void Effect_Config_Push();

        void Select_Effect_Config();

        void updatePCNT_Unit(int i);

        void updateButtonState(int32_t timeNOW);
        


};
