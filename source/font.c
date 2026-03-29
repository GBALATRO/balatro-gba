#include "font.h"

#include "util.h"

#include <stdlib.h>

static const char* s_font_point_lookup[] = {
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
    return s_font_point_lookup[val];
}

char digit_char_to_font_point(char digit_char)
{
    return get_font_point_str(digit_char - '0')[0];
}
