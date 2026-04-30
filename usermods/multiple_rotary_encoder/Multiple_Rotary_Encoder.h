#include <Arduino.h>
#include <driver/pcnt.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <rotaryEncoder.h>

#ifdef WOKWI_SIM
  #define Serial Serial0
  #define DEFAULT_SDA 21
  #define DEFAULT_SCL 22
  #include <wled.h>
#else
  #include <wled.h>
  #include <wled_bridge.h>
  #define DEFAULT_SDA 17
  #define DEFAULT_SCL 18
#endif



#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1


/**
 * Rotary Encoder Specifications
 */
#define PCNT_LIMIT_HIGH 21
#define PCNT_LIMIT_LOW -21
#define ROTARY_ENCODER_DETENTS 30


const int MAX_ENCODERS = 4;

extern int NUM_ENCODERS;
extern bool standalone;
extern bool displayON;
extern volatile unsigned long lastUpdate;



//Config Variables

static const char _name[] PROGMEM = "Multiple Rotary Encoder";
static const char _enabled[] PROGMEM = "Enabled";
static const char _pinCLK[] PROGMEM = "Pin CLK";
static const char _pinDT[] PROGMEM = "Pin DT";
static const char _pinSW[] PROGMEM = "Pin SW";
static const char _segID[] PROGMEM = "Segment ID";
static const char _longShortPressThreshold[] PROGMEM = "Long Press Threshold";
static const char _doublePressThreshold[] PROGMEM = "Double Press Threshold";
static const char _displayPinSDA[] PROGMEM = "Display PIN SDA";
static const char _displayPinSCL[] PROGMEM = "Display PIN SCL";





/**
 * Configures and Sets Up a PCNT unit
 */
inline void setup_PCNT_UNIT(pcnt_unit_t unit, int pin_clk, int pin_dt);



class multiple_rotary_encoder : public Usermod {

private:

    int displayPinSDA = DEFAULT_SDA;
    int displayPinSCL = DEFAULT_SCL;

    int longShortPressThreshold = 500;
    int doublePressThreshold = 200;
    bool enabled = false;

    Adafruit_SSD1306 OLED_Display{SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET};


    // Rotary Encoders Pin Declarations (actual ESP32-S3 Pinout Numbers)
    rotaryEncoder Encoders[MAX_ENCODERS]{
        {PCNT_UNIT_0, 5, 6, 7, 0},
        {PCNT_UNIT_1, -1, -1, -1, -1},
        {PCNT_UNIT_2, -1, -1, -1, -1},
        {PCNT_UNIT_3, -1, -1, -1, -1}
    };

    void init_PCNT_UNITS();

    /**
     * Hardware - Click Detection
     */

    /**
     * Hardware - Rotation Detection
     */


    void updateHardware();

    void global_EventHandler();




public:

    int BRIGHTNESS_ROTATION_DELAY = 40;
    int EFFECT_ROTATION_DELAY = 150;

    //const int longShortPressThreshold = 500;
    bool global_eventPending = false;

    void initOLED();

    void updateDisplay(rotaryEncoder& Encoder);

    void OnOffDisplay(rotaryEncoder& Encoder);

    inline void enable(bool e) { enabled = e; }

    inline bool isEnabled() { return enabled; }

    void setup() override ;

    void addToConfig(JsonObject& root) override ;

    bool readFromConfig(JsonObject& root) override ;

    void loop() override ;

};

extern multiple_rotary_encoder encoder_manager;

