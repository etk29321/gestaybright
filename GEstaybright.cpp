#include "GEstaybright.h"

/*
 COLOR_MODE_typedef:
    WHITE = 0
    MULTICOLOR = 1
*/

int const gestring::bright_map[] = {1,
2,
3,
4,
5,
6,
7,
8,
9,
10,
11,
12,
13,
14,
15,
16,
18,
20,
22,
25,
27,
30,
33,
36,
39,
42,
45,
49,
52,
56,
60,
64,
68,
72,
77,
81,
86,
90,
95,
100,
105,
111,
116,
121,
127,
133,
139,
145,
151,
157,
163,
170,
176,
183,
190,
197,
204,
211,
219,
226,
234,
242,
250,
258,
266,
274,
282,
291,
299,
308,
317,
326,
335,
344,
354,
363,
373,
383,
393,
403,
413,
423,
433,
444,
455,
465,
476,
487,
498,
510,
521,
533,
544,
556,
568,
580,
592,
604,
617,
629,
642,
655,
668,
681,
694,
707,
721,
734,
748,
762,
776,
790,
804,
818,
833,
847,
862,
877,
892,
907,
922,
937,
952,
968,
984,
1000,
1015,
1032,
1048,
1064,
1080,
1097,
1114,
1131,
1148,
1165,
1182,
1199,
1217,
1234,
1252,
1270,
1288,
1306,
1324,
1342,
1361,
1379,
1398,
1417,
1436,
1455,
1474,
1493,
1513,
1532,
1552,
1572,
1592,
1612,
1632,
1653,
1673,
1694,
1714,
1735,
1756,
1777,
1799,
1820,
1841,
1863,
1885,
1907,
1929,
1951,
1973,
1995,
2018,
2040,
2063,
2086,
2109,
2132,
2155,
2179,
2202,
2226,
2250,
2273,
2297,
2322,
2346,
2370,
2395,
2419,
2444,
2469,
2494,
2519,
2544,
2570,
2595,
2621,
2647,
2673,
2699,
2725,
2751,
2777,
2804,
2831,
2857,
2884,
2911,
2938,
2966,
2993,
3021,
3048,
3076,
3104,
3132,
3160,
3188,
3217,
3245,
3274,
3303,
3332,
3361,
3390,
3419,
3449,
3478,
3508,
3538,
3568,
3598,
3628,
3658,
3689,
3719,
3750,
3781,
3811,
3843,
3874,
3905,
3936,
3968,
4000,
4032,
4063,
4096};

gestring::gestring(int white_pin, int color_pin) {
  i_white_pin=white_pin;
  i_color_pin=color_pin;
}

void gestring::begin(EFFECT_MODE_typedef eft){
  ledcSetup(whiteChannel, freq, resolution);
  ledcSetup(colorChannel, freq, resolution);
  
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(i_white_pin, whiteChannel);
  ledcAttachPin(i_color_pin, colorChannel);

  dim_bright=1;
  brightness=0;
  effect=eft;
  color=WHITE;
  effect_scaler=0;
}
void gestring::setOutput(int bright, COLOR_MODE_typedef white_or_color){
  if(bright>255) bright=255;
  if(bright<0) bright=0;
  brightness=bright;
  //bright=(bright*bright)/15.87524414; // map brightness via exponential mapping
  bright=bright_map[bright];
  if (white_or_color==0) {
    ledcWrite(whiteChannel, bright);
    ledcWrite(colorChannel, 0);
  } else {
    ledcWrite(colorChannel, bright);
    ledcWrite(whiteChannel, 0);
  }
}


void gestring::runEffects(){
  //SLO-GLO
  switch(effect){
    case STEADY_WHITE:
      setOutput(255, WHITE);
    break;
    case STEADY_MULTICOLOR:
      setOutput(255, MULTICOLOR);
    break;
    case SLO_GLO:
      if(dim_bright==0) {
        brightness--;
        if (brightness<=0) {
          dim_bright=1;
          color=(COLOR_MODE_typedef)(!(int)color);
        }
      } else {
        brightness++;
        if(brightness>=255) {
          dim_bright=0;
        }
      }
      setOutput(brightness, color);
      break;
    case WHITE_SLO_GLO:
      color=WHITE;
      if(dim_bright==0) {
        brightness--;
        if (brightness<=0) {
          dim_bright=1;
        }
      } else {
        brightness++;
        if(brightness>=255) {
          dim_bright=0;
        }
      }
      setOutput(brightness, color);

    break;
    case MULTI_SLO_GLO:
      color=MULTICOLOR;
      if(dim_bright==0) {
        brightness--;
        if (brightness<=0) {
          dim_bright=1;
        }
      } else {
        brightness++;
        if(brightness>=255) {
          dim_bright=0;
        }
      }
      setOutput(brightness, color);

    break;
    case WHITE_FLASHING:
    effect_scaler++;
    if (effect_scaler<50)
      setOutput(0, WHITE);
    if (effect_scaler>=50)
      setOutput(255, WHITE);
    if (effect_scaler>=200)
      effect_scaler=0;
    break;
    case MULTI_FLASHING:
      effect_scaler++;
      if (effect_scaler<50)
        setOutput(0, MULTICOLOR);
      if (effect_scaler>=50)
        setOutput(255, MULTICOLOR);
      if (effect_scaler>=200)
        effect_scaler=0;

    break;
    case CHANGE_FLASHING:
      effect_scaler++;
      if (effect_scaler>=100) {
        color=(COLOR_MODE_typedef)(!(int)color);
        setOutput(255, color);
        effect_scaler=0;
      }

    break;
  }

}

int gestring::getBrightness(){
  return brightness;
}

COLOR_MODE_typedef gestring::getColor(){
  return color;
}

