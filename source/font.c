#include "font.h"

#include "util.h"

#include <stdlib.h>

static const char* FONT_POINT_LOOKUP[] = {
    FP0_STR,
    FP1_STR,
    FP2_STR,
    FP3_STR,
    FP4_STR,
    FP5_STR,
    FP6_STR,
    FP7_STR,
    FP8_STR,
    FP9_STR,
};

const char* get_font_point_str(int val)
{
    val = abs(val) % 10;
    return FONT_POINT_LOOKUP[val];
}

char digit_char_to_font_point(char digit_char)
{
    return get_font_point_str(digit_char - '0')[0];
}
