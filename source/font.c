#include "font.h"

#include "tonc.h"

static const char* s_font_point_lookup[] = {
    XSTR(FP0),
    XSTR(FP1),
    XSTR(FP2),
    XSTR(FP3),
    XSTR(FP4),
    XSTR(FP5),
    XSTR(FP6),
    XSTR(FP7),
    XSTR(FP8),
    XSTR(FP9),
};

const char* get_font_point_str(int val)
{
    val = val & 9;
    return s_font_point_lookup[val];
}
