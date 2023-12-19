#ifndef GESTAYBRIGHT_H
#define GESTAYBRIGHT_H

#include <stdlib.h>
#include <esp32-hal-ledc.h>
#include <esp32-hal-timer.h>


typedef enum
{
    WHITE = 0,
    MULTICOLOR = 1
} COLOR_MODE_typedef;

typedef enum
{
    STEADY_WHITE = 0,
    STEADY_MULTICOLOR = 1,
    SLO_GLO = 2,
    WHITE_SLO_GLO = 3,
    MULTI_SLO_GLO = 4,
    WHITE_FLASHING = 5,
    MULTI_FLASHING = 6,
    CHANGE_FLASHING = 7,
    DMX = 8,
    E131 = 9
} EFFECT_MODE_typedef;

class gestring
{
  private:
    int brightness;
    COLOR_MODE_typedef color;
    int i_white_pin;
    int i_color_pin;
    // setting PWM properties
    const int freq = 8000;
    const int resolution = 12;
    const int whiteChannel =0;
    const int colorChannel =1;
    EFFECT_MODE_typedef effect;
    //hw_timer_t *effect_timer;
    int dim_bright;
    int effect_scaler;
    static int const bright_map[];

  public:
    gestring(int white_pin, int color_pin);
    void begin(EFFECT_MODE_typedef eft);
    void setOutput(int bright, COLOR_MODE_typedef white_or_color);
    void  runEffects();
    int getBrightness();
    COLOR_MODE_typedef getColor();


};



#endif