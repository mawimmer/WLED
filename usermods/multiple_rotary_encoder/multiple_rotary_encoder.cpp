#include <Arduino.h>
#include <driver/pcnt.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <rotaryEncoder.h>
#include <multiple_rotary_encoder.h>
#include <wled_bridge.h>


#if defined(WOKWI_SIM)
    #define Serial Serial0
    #include <wled_mock.h>
#elif defined(WLED_DEV)
    #error "This Project can not be built in this env."
    #include "../../WLED/wled00/wled.h"
#else
    #include <wled.h>
#endif

int NUM_ENCODERS = 1;
bool standalone = false;
bool displayON = false;
volatile unsigned long lastUpdate = 0;




/**
 * Configures and Sets Up a PCNT unit
 */
inline void setup_PCNT_UNIT(pcnt_unit_t unit, int pin_clk, int pin_dt) {
    pcnt_config_t pcnt_config = {
        .pulse_gpio_num = pin_clk,
        .ctrl_gpio_num = pin_dt,
        .lctrl_mode = PCNT_MODE_REVERSE,
        .hctrl_mode = PCNT_MODE_KEEP,
        .pos_mode = PCNT_COUNT_INC,
        .neg_mode = PCNT_COUNT_DEC,
        .counter_h_lim = PCNT_LIMIT_HIGH,
        .counter_l_lim = PCNT_LIMIT_LOW,
        .unit = unit,
        .channel = PCNT_CHANNEL_0,
    };

    pcnt_unit_config(&pcnt_config);
    pcnt_set_filter_value(unit, 1000);
    pcnt_filter_enable(unit);
    pcnt_counter_pause(unit);
    pcnt_counter_clear(unit);
    pcnt_counter_resume(unit);
}

    Adafruit_SSD1306 OLED_Display{SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET};

    void multiple_rotary_encoder::initOLED() {

        Wire.begin(displayPinSDA, displayPinSCL);

        if (!OLED_Display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
            Serial.println(F("SSD1306 Fehler: Display nicht gefunden!"));
        };

        OLED_Display.clearDisplay();
        OLED_Display.setTextSize(1);
        OLED_Display.setTextColor(SSD1306_WHITE);
        OLED_Display.setCursor(0, 10);
        OLED_Display.println(F("Brightness:"));
        OLED_Display.setCursor(90, 10);
        OLED_Display.println(F("0"));
        OLED_Display.setCursor(0, 20);
        OLED_Display.println(F("Effect:"));
        OLED_Display.setCursor(90, 20);
        OLED_Display.println(F("0"));
        OLED_Display.setCursor(0, 30);
        OLED_Display.println(F("Config:"));
        OLED_Display.setCursor(90, 30);
        OLED_Display.println(F("0"));
        OLED_Display.display();
        Serial.println("Display initialized!");
    }


    void multiple_rotary_encoder::init_PCNT_UNITS() {
        for (int i = 0; i < NUM_ENCODERS; i++) {
            rotaryEncoder& Encoder = Encoders[i];

            gpio_set_direction(Encoder.pin_clk, GPIO_MODE_INPUT);
            gpio_set_pull_mode(Encoder.pin_clk, GPIO_PULLUP_ONLY);
            gpio_set_direction(Encoder.pin_dt, GPIO_MODE_INPUT);
            gpio_set_pull_mode(Encoder.pin_dt, GPIO_PULLUP_ONLY);
            gpio_set_direction(Encoder.pin_sw, GPIO_MODE_INPUT);
            gpio_set_pull_mode(Encoder.pin_sw, GPIO_PULLUP_ONLY);

            gpio_set_intr_type(Encoder.pin_sw, GPIO_INTR_ANYEDGE);
            gpio_isr_handler_add(Encoder.pin_sw, buttonISR, &Encoder);

            setup_PCNT_UNIT(Encoder.unit, Encoder.pin_clk, Encoder.pin_dt);

            Serial.printf("Encoder # %d has been initialized! \r\n", i);
        }
    }


    void multiple_rotary_encoder::updateHardware() {

        for (int i = 0; i < NUM_ENCODERS; i++) {

            // Referencing Encoder to Encoder[i] Pointer in the for Scope
            rotaryEncoder& Encoder = Encoders[i];
            int32_t timeNOW = xTaskGetTickCount() * portTICK_PERIOD_MS;
            
            Encoder.updateButtonState(timeNOW);

            Encoder.updatePCNT_Unit(i);
        }
    }

    void multiple_rotary_encoder::global_EventHandler() {
        Serial.println("global_EventHandler");
        global_eventPending = false;

        for (int i = 0; i < NUM_ENCODERS; i++) {
            rotaryEncoder& Encoder = Encoders[i];

            if (Encoder.eventButton != rotaryEncoder::ButtonEventType::NONE) {
                Encoder.ButtonEventHandler();
            }

            if (Encoder.eventRotation) {
                Encoder.RotationEventHandler();
            }
        }
    }





    void multiple_rotary_encoder::updateDisplay(rotaryEncoder& Encoder) {

        if( millis() - lastUpdate >= 50 ) {
            lastUpdate = millis();

            Serial.println("updateDisplay");

            OLED_Display.fillRect(90, 10, 50, 10, SSD1306_BLACK);
            OLED_Display.setCursor(90, 10);
            OLED_Display.println(Encoder.brightness);
            OLED_Display.fillRect(90, 20, 50, 10, SSD1306_BLACK);
            OLED_Display.setCursor(90, 20);
            OLED_Display.println(Encoder.effect);
            OLED_Display.fillRect(90, 30, 50, 10, SSD1306_BLACK);
            OLED_Display.setCursor(90, 30);
            int index = static_cast<int>(Encoder.selectedSegConfig);
            OLED_Display.println(index);
            char lineBuffer[17];
            extractModeName(Encoder.effect, JSON_mode_names, lineBuffer, 16);
            OLED_Display.fillRect(0, 40, 100, 10, SSD1306_BLACK);
            OLED_Display.setCursor(0, 40);
            OLED_Display.println(lineBuffer);
            OLED_Display.display();
        };

    }



    void multiple_rotary_encoder::OnOffDisplay(rotaryEncoder& Encoder) {
        if(displayON){
            Serial.println("Display OFF");
            displayON = false;
            OLED_Display.ssd1306_command(SSD1306_DISPLAYOFF);
            return;
        } else {
            Serial.println("Display ON");
            displayON = true;
            OLED_Display.ssd1306_command(SSD1306_DISPLAYON);
            return;
        }

    };

    void multiple_rotary_encoder::setup() {

    for (int i = 0; i < NUM_ENCODERS; i++) {
        Encoders[i].rotationDelay = BRIGHTNESS_ROTATION_DELAY;
    }
        gpio_install_isr_service(0);
        initOLED();
        init_PCNT_UNITS();
    }

    void multiple_rotary_encoder::addToConfig(JsonObject& root) {
        JsonObject top = root.createNestedObject(FPSTR(_name));
        top[FPSTR(_enabled)] = enabled;
        top[FPSTR(_longShortPressThreshold)]  = (int)longShortPressThreshold;
        top[FPSTR(_doublePressThreshold)]  = (int)doublePressThreshold;
        top[FPSTR(_displayPinSDA)]  = (int)displayPinSDA;
        top[FPSTR(_displayPinSCL)]  = (int)displayPinSCL;

        top["Anzahl Encoder"] = NUM_ENCODERS;

        

        for (int i = 0; i < NUM_ENCODERS; i++) {
            rotaryEncoder& Encoder = Encoders[i];
            String encoderName = "Encoder " + String(i);
            JsonObject encoderObj = top.createNestedObject(encoderName);

            encoderObj[FPSTR(_pinCLK)] = (int8_t)Encoder.pin_clk;
            encoderObj[FPSTR(_pinDT)]  = (int8_t)Encoder.pin_dt;
            encoderObj[FPSTR(_pinSW)]  = (int8_t)Encoder.pin_sw;
            
            encoderObj[FPSTR(_segID)]  = (int8_t)Encoder.segmentID;
        }
    }

    bool multiple_rotary_encoder::readFromConfig(JsonObject& root) {
        JsonObject top = root[FPSTR(_name)];
        if (top.isNull()) return false;

        getJsonValue(top[FPSTR(_enabled)], enabled);
        getJsonValue(top[FPSTR(_longShortPressThreshold)], longShortPressThreshold);
        getJsonValue(top[FPSTR(_doublePressThreshold)], doublePressThreshold);
        getJsonValue(top[FPSTR(_displayPinSDA)], displayPinSDA);
        getJsonValue(top[FPSTR(_displayPinSCL)], displayPinSCL);

        int8_t newCount = NUM_ENCODERS;
        getJsonValue(top["Anzahl Encoder"], newCount);
        NUM_ENCODERS = max(1, min((int)newCount, MAX_ENCODERS));

        for (int i = 0; i < NUM_ENCODERS; i++) {
            rotaryEncoder& Encoder = Encoders[i];
            String encoderName = "Encoder " + String(i);
            JsonObject encoderObj = top[encoderName];

            if (!encoderObj.isNull()) {
                int8_t pCLK = Encoder.pin_clk;
                int8_t pDT  = Encoder.pin_dt;
                int8_t pSW  = Encoder.pin_sw;
                int8_t sID  = Encoder.segmentID;

                getJsonValue(encoderObj[FPSTR(_pinCLK)], pCLK);
                getJsonValue(encoderObj[FPSTR(_pinDT)], pDT);
                getJsonValue(encoderObj[FPSTR(_pinSW)], pSW);
                getJsonValue(encoderObj[FPSTR(_segID)], sID);

                Encoder.pin_clk = static_cast<gpio_num_t>(pCLK);
                Encoder.pin_dt  = static_cast<gpio_num_t>(pDT);
                Encoder.pin_sw  = static_cast<gpio_num_t>(pSW);
                Encoder.segmentID  = sID;
            }
        }
        return true;
    }

    void multiple_rotary_encoder::loop() {

        if(!enabled){
            return;
        }

        updateHardware();

        if (global_eventPending) {
            global_EventHandler();
        }
    }

    multiple_rotary_encoder encoder_manager;

    REGISTER_USERMOD(encoder_manager);
