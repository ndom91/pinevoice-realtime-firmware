/*******************************************************************************
 * Size: 12 px
 * Bpp: 1
 * Opts: 
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef RM_12
#define RM_12 1
#endif

#if RM_12

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xfc, 0x80,

    /* U+0022 "\"" */
    0xb6, 0x80,

    /* U+0023 "#" */
    0x14, 0x51, 0xf9, 0x42, 0x9f, 0x8a, 0x24, 0x50,

    /* U+0024 "$" */
    0x23, 0x92, 0x94, 0x38, 0xe1, 0x8c, 0x5c, 0x40,

    /* U+0025 "%" */
    0x60, 0x94, 0x94, 0x68, 0x18, 0x16, 0x29, 0x29,
    0x6,

    /* U+0026 "&" */
    0x30, 0x91, 0x23, 0xc7, 0x1e, 0xa7, 0x44, 0x7c,

    /* U+0027 "'" */
    0xe0,

    /* U+0028 "(" */
    0x29, 0x49, 0x24, 0x91, 0x22,

    /* U+0029 ")" */
    0x89, 0x12, 0x49, 0x25, 0x28,

    /* U+002A "*" */
    0x21, 0x3e, 0xc5, 0x0,

    /* U+002B "+" */
    0x20, 0x82, 0x3f, 0x20, 0x82, 0x0,

    /* U+002C "," */
    0x54,

    /* U+002D "-" */
    0xe0,

    /* U+002E "." */
    0x80,

    /* U+002F "/" */
    0x11, 0x12, 0x22, 0x44, 0x48,

    /* U+0030 "0" */
    0x74, 0x63, 0x18, 0xc6, 0x31, 0x70,

    /* U+0031 "1" */
    0x3c, 0x92, 0x49, 0x20,

    /* U+0032 "2" */
    0x72, 0x28, 0x82, 0x10, 0xc6, 0x10, 0xfc,

    /* U+0033 "3" */
    0x76, 0x42, 0x13, 0x4, 0x39, 0x70,

    /* U+0034 "4" */
    0x8, 0x63, 0x8a, 0x6b, 0x2f, 0xc2, 0x8,

    /* U+0035 "5" */
    0x7e, 0x31, 0xe0, 0x86, 0x39, 0x70,

    /* U+0036 "6" */
    0x32, 0x21, 0xe8, 0xc6, 0x39, 0x70,

    /* U+0037 "7" */
    0xfc, 0x30, 0x86, 0x10, 0x43, 0x8, 0x60,

    /* U+0038 "8" */
    0x74, 0x63, 0x17, 0x46, 0x31, 0x70,

    /* U+003A ":" */
    0x82,

    /* U+003B ";" */
    0x87, 0x0,

    /* U+003C "<" */
    0x9, 0xf9, 0xc3, 0x84,

    /* U+003D "=" */
    0xf8, 0x1, 0xf0,

    /* U+003E ">" */
    0x87, 0xe, 0x7e, 0x40,

    /* U+003F "?" */
    0x72, 0x42, 0x11, 0x10, 0x80, 0x20,

    /* U+0040 "@" */
    0x1e, 0x8, 0x64, 0xb, 0x39, 0x9a, 0x64, 0x99,
    0x26, 0x5b, 0x9b, 0x90, 0x6, 0x0, 0xf8,

    /* U+0041 "A" */
    0x18, 0x18, 0x3c, 0x3c, 0x24, 0x66, 0x7e, 0x42,
    0xc3,

    /* U+0042 "B" */
    0xfa, 0x18, 0x61, 0xfa, 0x18, 0x61, 0xf8,

    /* U+0043 "C" */
    0x39, 0x38, 0x60, 0x82, 0x8, 0x53, 0x78,

    /* U+0044 "D" */
    0xf2, 0x28, 0x61, 0x86, 0x18, 0x62, 0xf0,

    /* U+0045 "E" */
    0xfc, 0x21, 0xf, 0xc2, 0x10, 0xf8,

    /* U+0046 "F" */
    0xfc, 0x21, 0xf, 0x42, 0x10, 0x80,

    /* U+0048 "H" */
    0x83, 0x6, 0xc, 0x1f, 0xf0, 0x60, 0xc1, 0x82,

    /* U+0049 "I" */
    0xff, 0x80,

    /* U+004A "J" */
    0x8, 0x42, 0x10, 0x84, 0x31, 0x70,

    /* U+004B "K" */
    0x8e, 0x6b, 0x28, 0xe3, 0xc9, 0xa2, 0x8c,

    /* U+004C "L" */
    0x84, 0x21, 0x8, 0x42, 0x10, 0xf8,

    /* U+004D "M" */
    0xc1, 0xe0, 0xf8, 0xf4, 0x5a, 0x6d, 0xb6, 0x53,
    0x39, 0x88, 0x80,

    /* U+004E "N" */
    0x83, 0x87, 0x8d, 0x9b, 0xb3, 0x63, 0xc3, 0x82,

    /* U+004F "O" */
    0x38, 0x8a, 0xc, 0x18, 0x30, 0x60, 0xa2, 0x38,

    /* U+0050 "P" */
    0xfa, 0x18, 0x61, 0xfa, 0x8, 0x20, 0x80,

    /* U+0051 "Q" */
    0x38, 0x8a, 0xc, 0x18, 0x30, 0x60, 0xa3, 0x3c,
    0xc,

    /* U+0052 "R" */
    0xfa, 0x18, 0x61, 0xfa, 0x68, 0xe3, 0x84,

    /* U+0053 "S" */
    0x39, 0x14, 0x58, 0x38, 0x38, 0x71, 0x78,

    /* U+0054 "T" */
    0xfe, 0x20, 0x40, 0x81, 0x2, 0x4, 0x8, 0x10,

    /* U+0055 "U" */
    0x86, 0x18, 0x61, 0x86, 0x18, 0x63, 0x78,

    /* U+0056 "V" */
    0xc3, 0x42, 0x46, 0x64, 0x24, 0x2c, 0x38, 0x18,
    0x18,

    /* U+0057 "W" */
    0xc4, 0x53, 0x14, 0xed, 0x2b, 0x6a, 0x9e, 0xa3,
    0x28, 0xc6, 0x31, 0x0,

    /* U+0058 "X" */
    0x46, 0xc8, 0xb1, 0xc1, 0x87, 0xb, 0x32, 0x46,

    /* U+0059 "Y" */
    0xc6, 0x89, 0xb1, 0x43, 0x82, 0x4, 0x8, 0x10,

    /* U+005A "Z" */
    0xfc, 0x31, 0x84, 0x30, 0x86, 0x30, 0xfc,

    /* U+005B "[" */
    0xea, 0xaa, 0xaa, 0xc0,

    /* U+005C "\\" */
    0xc2, 0x10, 0xc2, 0x10, 0xc2, 0x18, 0x40,

    /* U+005D "]" */
    0xd5, 0x55, 0x55, 0xc0,

    /* U+005E "^" */
    0x21, 0x14, 0xad, 0x80,

    /* U+005F "_" */
    0xf8,

    /* U+0060 "`" */
    0x4c,

    /* U+0061 "a" */
    0x74, 0x42, 0xf8, 0xc5, 0xe0,

    /* U+0062 "b" */
    0x84, 0x21, 0xe9, 0xc6, 0x31, 0x9f, 0x80,

    /* U+0063 "c" */
    0x7b, 0x28, 0x20, 0x82, 0x27, 0x0,

    /* U+0064 "d" */
    0x8, 0x42, 0xfc, 0xc6, 0x31, 0xcb, 0xc0,

    /* U+0065 "e" */
    0x76, 0x63, 0xf8, 0x65, 0xc0,

    /* U+0066 "f" */
    0x34, 0x4f, 0x44, 0x44, 0x44,

    /* U+0067 "g" */
    0x7e, 0x63, 0x18, 0xe5, 0xe1, 0x8b, 0x80,

    /* U+0068 "h" */
    0x84, 0x21, 0xe8, 0xc6, 0x31, 0x8c, 0x40,

    /* U+0069 "i" */
    0xbf, 0x80,

    /* U+006A "j" */
    0x45, 0x55, 0x57,

    /* U+006B "k" */
    0x84, 0x21, 0x3b, 0x73, 0x96, 0x94, 0xc0,

    /* U+006C "l" */
    0xff, 0xc0,

    /* U+006D "m" */
    0xff, 0x44, 0x62, 0x31, 0x18, 0x8c, 0x46, 0x22,

    /* U+006E "n" */
    0xf4, 0x63, 0x18, 0xc6, 0x20,

    /* U+006F "o" */
    0x7b, 0x38, 0x61, 0x87, 0x37, 0x80,

    /* U+0070 "p" */
    0xf4, 0xe3, 0x18, 0xcf, 0xd0, 0x84, 0x0,

    /* U+0071 "q" */
    0x7e, 0x63, 0x18, 0xe5, 0xe1, 0x8, 0x40,

    /* U+0072 "r" */
    0xf2, 0x49, 0x20,

    /* U+0073 "s" */
    0x74, 0x70, 0xe0, 0xc5, 0xc0,

    /* U+0074 "t" */
    0x44, 0xf4, 0x44, 0x44, 0x70,

    /* U+0075 "u" */
    0x8c, 0x63, 0x18, 0xc5, 0xe0,

    /* U+0076 "v" */
    0xcd, 0x24, 0x9e, 0x30, 0xc3, 0x0,

    /* U+0077 "w" */
    0xc9, 0xac, 0x95, 0x4a, 0xa7, 0x71, 0xb0, 0x88,

    /* U+0078 "x" */
    0x4d, 0xe3, 0xc, 0x31, 0xec, 0xc0,

    /* U+0079 "y" */
    0x45, 0x36, 0x8a, 0x38, 0xc3, 0x4, 0x31, 0x80,

    /* U+007A "z" */
    0xf8, 0xcc, 0x46, 0x63, 0xe0,

    /* U+007B "{" */
    0x2d, 0x24, 0xa2, 0x49, 0x30,

    /* U+007C "|" */
    0xff, 0xe0,

    /* U+007D "}" */
    0x8c, 0x44, 0x44, 0x34, 0x44, 0x4c, 0x80,

    /* U+007E "~" */
    0x65, 0x28, 0x70
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 48, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 51, .box_w = 1, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 3, .adv_w = 62, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 5, .adv_w = 117, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 13, .adv_w = 109, .box_w = 5, .box_h = 12, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 21, .adv_w = 141, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 30, .adv_w = 123, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 38, .adv_w = 32, .box_w = 1, .box_h = 3, .ofs_x = 1, .ofs_y = 7},
    {.bitmap_index = 39, .adv_w = 67, .box_w = 3, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 44, .adv_w = 68, .box_w = 3, .box_h = 13, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 49, .adv_w = 85, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 53, .adv_w = 107, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 59, .adv_w = 42, .box_w = 2, .box_h = 4, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 60, .adv_w = 63, .box_w = 3, .box_h = 1, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 61, .adv_w = 54, .box_w = 1, .box_h = 1, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 62, .adv_w = 76, .box_w = 4, .box_h = 10, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 67, .adv_w = 109, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 73, .adv_w = 109, .box_w = 3, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 77, .adv_w = 109, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 84, .adv_w = 109, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 90, .adv_w = 109, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 97, .adv_w = 109, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 103, .adv_w = 109, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 109, .adv_w = 109, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 116, .adv_w = 109, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 122, .adv_w = 51, .box_w = 1, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 123, .adv_w = 46, .box_w = 1, .box_h = 9, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 125, .adv_w = 98, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 129, .adv_w = 107, .box_w = 5, .box_h = 4, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 132, .adv_w = 100, .box_w = 5, .box_h = 6, .ofs_x = 1, .ofs_y = 1},
    {.bitmap_index = 136, .adv_w = 93, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 142, .adv_w = 172, .box_w = 10, .box_h = 12, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 157, .adv_w = 128, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 166, .adv_w = 121, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 173, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 180, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 187, .adv_w = 109, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 193, .adv_w = 105, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 199, .adv_w = 136, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 207, .adv_w = 54, .box_w = 1, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 209, .adv_w = 107, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 215, .adv_w = 121, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 222, .adv_w = 104, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 228, .adv_w = 168, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 239, .adv_w = 136, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 247, .adv_w = 133, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 255, .adv_w = 123, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 262, .adv_w = 133, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 271, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 278, .adv_w = 116, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 285, .adv_w = 117, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 293, .adv_w = 125, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 300, .adv_w = 124, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 309, .adv_w = 169, .box_w = 10, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 321, .adv_w = 122, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 329, .adv_w = 117, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 337, .adv_w = 116, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 344, .adv_w = 53, .box_w = 2, .box_h = 13, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 348, .adv_w = 80, .box_w = 5, .box_h = 10, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 355, .adv_w = 53, .box_w = 2, .box_h = 13, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 359, .adv_w = 82, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 5},
    {.bitmap_index = 363, .adv_w = 87, .box_w = 5, .box_h = 1, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 364, .adv_w = 62, .box_w = 3, .box_h = 2, .ofs_x = 0, .ofs_y = 8},
    {.bitmap_index = 365, .adv_w = 104, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 370, .adv_w = 108, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 377, .adv_w = 101, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 383, .adv_w = 108, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 390, .adv_w = 103, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 395, .adv_w = 68, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 400, .adv_w = 109, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 407, .adv_w = 107, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 414, .adv_w = 49, .box_w = 1, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 416, .adv_w = 48, .box_w = 2, .box_h = 12, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 419, .adv_w = 100, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 426, .adv_w = 49, .box_w = 1, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 428, .adv_w = 167, .box_w = 9, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 436, .adv_w = 107, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 441, .adv_w = 109, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 447, .adv_w = 108, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 454, .adv_w = 109, .box_w = 5, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 461, .adv_w = 68, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 464, .adv_w = 99, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 469, .adv_w = 64, .box_w = 4, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 474, .adv_w = 107, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 479, .adv_w = 95, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 485, .adv_w = 143, .box_w = 9, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 493, .adv_w = 97, .box_w = 6, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 499, .adv_w = 93, .box_w = 6, .box_h = 10, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 507, .adv_w = 97, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 512, .adv_w = 64, .box_w = 3, .box_h = 13, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 517, .adv_w = 48, .box_w = 1, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 519, .adv_w = 64, .box_w = 4, .box_h = 13, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 526, .adv_w = 128, .box_w = 7, .box_h = 3, .ofs_x = 1, .ofs_y = 3}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 25, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 58, .range_length = 13, .glyph_id_start = 26,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 72, .range_length = 55, .glyph_id_start = 39,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};

/*-----------------
 *    KERNING
 *----------------*/


/*Pair left and right glyphs for kerning*/
static const uint8_t kern_pair_glyph_ids[] =
{
    1, 51,
    3, 3,
    3, 8,
    3, 33,
    3, 64,
    3, 66,
    3, 67,
    3, 68,
    3, 70,
    3, 76,
    3, 77,
    3, 78,
    3, 79,
    3, 80,
    3, 82,
    3, 86,
    8, 3,
    8, 8,
    8, 33,
    8, 64,
    8, 66,
    8, 67,
    8, 68,
    8, 70,
    8, 76,
    8, 77,
    8, 78,
    8, 79,
    8, 80,
    8, 82,
    8, 86,
    9, 53,
    9, 54,
    9, 56,
    13, 3,
    13, 8,
    15, 3,
    15, 8,
    16, 16,
    33, 3,
    33, 8,
    33, 31,
    33, 35,
    33, 46,
    33, 48,
    33, 51,
    33, 52,
    33, 53,
    33, 54,
    33, 56,
    33, 76,
    33, 77,
    33, 78,
    33, 79,
    33, 83,
    33, 84,
    33, 85,
    33, 86,
    33, 88,
    33, 89,
    34, 51,
    34, 53,
    34, 56,
    35, 10,
    35, 51,
    35, 60,
    35, 92,
    36, 13,
    36, 15,
    36, 33,
    36, 51,
    36, 53,
    36, 55,
    36, 56,
    36, 57,
    37, 51,
    37, 66,
    37, 67,
    37, 68,
    37, 69,
    37, 70,
    37, 78,
    37, 80,
    37, 84,
    37, 85,
    37, 86,
    37, 88,
    38, 13,
    38, 15,
    38, 33,
    38, 41,
    38, 51,
    38, 64,
    38, 66,
    38, 67,
    38, 68,
    38, 70,
    38, 78,
    38, 80,
    38, 81,
    38, 84,
    38, 85,
    38, 88,
    39, 33,
    39, 51,
    39, 55,
    39, 56,
    40, 33,
    40, 51,
    40, 55,
    40, 56,
    41, 33,
    42, 14,
    42, 35,
    42, 46,
    42, 48,
    42, 66,
    42, 67,
    42, 68,
    42, 70,
    42, 78,
    42, 80,
    42, 84,
    42, 85,
    42, 86,
    42, 88,
    43, 3,
    43, 8,
    43, 33,
    43, 35,
    43, 46,
    43, 48,
    43, 51,
    43, 52,
    43, 53,
    43, 54,
    43, 56,
    43, 84,
    43, 85,
    43, 86,
    43, 88,
    44, 33,
    44, 51,
    44, 55,
    44, 56,
    45, 33,
    45, 51,
    45, 55,
    45, 56,
    46, 13,
    46, 15,
    46, 33,
    46, 51,
    46, 53,
    46, 55,
    46, 56,
    46, 57,
    47, 13,
    47, 15,
    47, 33,
    47, 41,
    47, 55,
    47, 57,
    47, 64,
    47, 66,
    47, 67,
    47, 68,
    47, 70,
    47, 78,
    47, 80,
    47, 83,
    47, 85,
    47, 88,
    48, 51,
    48, 53,
    48, 54,
    48, 56,
    49, 51,
    49, 53,
    49, 56,
    51, 1,
    51, 13,
    51, 14,
    51, 15,
    51, 33,
    51, 35,
    51, 41,
    51, 46,
    51, 48,
    51, 50,
    51, 51,
    51, 53,
    51, 54,
    51, 56,
    51, 64,
    51, 66,
    51, 67,
    51, 68,
    51, 70,
    51, 76,
    51, 77,
    51, 78,
    51, 79,
    51, 80,
    51, 81,
    51, 82,
    51, 84,
    51, 85,
    51, 86,
    51, 87,
    51, 88,
    51, 89,
    52, 33,
    53, 10,
    53, 13,
    53, 14,
    53, 15,
    53, 33,
    53, 35,
    53, 46,
    53, 48,
    53, 60,
    53, 64,
    53, 66,
    53, 67,
    53, 68,
    53, 70,
    53, 78,
    53, 80,
    53, 81,
    53, 84,
    53, 85,
    53, 88,
    53, 92,
    54, 10,
    54, 13,
    54, 14,
    54, 15,
    54, 33,
    54, 51,
    54, 60,
    54, 64,
    54, 66,
    54, 67,
    54, 68,
    54, 70,
    54, 78,
    54, 80,
    54, 81,
    54, 84,
    54, 92,
    55, 14,
    55, 35,
    55, 46,
    55, 48,
    55, 53,
    55, 66,
    55, 67,
    55, 68,
    55, 70,
    55, 78,
    55, 80,
    55, 84,
    55, 85,
    55, 88,
    56, 7,
    56, 10,
    56, 11,
    56, 13,
    56, 14,
    56, 15,
    56, 33,
    56, 35,
    56, 41,
    56, 46,
    56, 48,
    56, 50,
    56, 51,
    56, 52,
    56, 53,
    56, 54,
    56, 55,
    56, 56,
    56, 60,
    56, 64,
    56, 66,
    56, 67,
    56, 68,
    56, 69,
    56, 70,
    56, 76,
    56, 77,
    56, 78,
    56, 79,
    56, 80,
    56, 81,
    56, 82,
    56, 83,
    56, 84,
    56, 85,
    56, 87,
    56, 88,
    56, 89,
    56, 92,
    57, 33,
    57, 35,
    57, 46,
    57, 48,
    57, 66,
    57, 67,
    57, 68,
    57, 70,
    57, 78,
    57, 80,
    57, 84,
    57, 85,
    57, 86,
    57, 88,
    58, 41,
    58, 52,
    64, 3,
    64, 8,
    64, 85,
    64, 88,
    65, 3,
    65, 8,
    65, 85,
    65, 87,
    65, 88,
    65, 89,
    66, 3,
    66, 8,
    68, 3,
    68, 8,
    68, 85,
    68, 88,
    69, 3,
    69, 8,
    69, 10,
    69, 60,
    69, 66,
    69, 67,
    69, 68,
    69, 70,
    69, 80,
    69, 92,
    71, 3,
    71, 8,
    74, 66,
    74, 67,
    74, 68,
    74, 70,
    74, 80,
    76, 3,
    76, 8,
    77, 3,
    77, 8,
    78, 3,
    78, 8,
    78, 85,
    78, 87,
    78, 88,
    78, 89,
    79, 3,
    79, 8,
    79, 85,
    79, 87,
    79, 88,
    79, 89,
    81, 3,
    81, 8,
    81, 13,
    81, 15,
    81, 64,
    81, 66,
    81, 67,
    81, 68,
    81, 69,
    81, 70,
    81, 78,
    81, 80,
    81, 83,
    81, 85,
    81, 86,
    81, 88,
    83, 78,
    85, 3,
    85, 8,
    85, 13,
    85, 15,
    85, 64,
    85, 66,
    85, 67,
    85, 68,
    85, 69,
    85, 70,
    85, 78,
    85, 80,
    86, 13,
    86, 15,
    87, 66,
    87, 67,
    87, 68,
    87, 70,
    87, 78,
    87, 80,
    88, 3,
    88, 8,
    88, 13,
    88, 15,
    88, 64,
    88, 66,
    88, 67,
    88, 68,
    88, 69,
    88, 70,
    88, 78,
    88, 80,
    89, 66,
    89, 67,
    89, 68,
    89, 70,
    89, 78,
    89, 80,
    90, 41,
    90, 52
};

/* Kerning between the respective left and right glyphs
 * 4.4 format which needs to scaled with `kern_scale`*/
static const int8_t kern_pair_values[] =
{
    -6, -3, -3, -11, -5, -6, -6, -6,
    -6, -2, -2, -9, -2, -6, -9, 1,
    -3, -3, -11, -5, -6, -6, -6, -6,
    -2, -2, -9, -2, -6, -9, 1, 2,
    4, 2, -27, -27, -27, -27, -23, -11,
    -11, -8, -2, -2, -2, -11, -2, -7,
    -4, -14, -4, -4, -1, -4, -2, -1,
    -5, -3, -5, 1, -3, -2, -5, -2,
    -3, -1, -2, -11, -11, -2, -8, -2,
    -2, -4, -2, 2, -2, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, -26,
    -26, -18, -19, 2, -3, -2, -2, -2,
    -2, -2, -2, -2, -2, -2, -2, 2,
    -3, 2, -3, 2, -3, 2, -3, -2,
    -15, -3, -3, -3, -2, -2, -2, -2,
    -3, -2, -2, -4, -6, -4, -27, -27,
    2, -6, -6, -6, -19, -2, -19, -9,
    -26, -1, -12, -5, -12, 2, -3, 2,
    -3, 2, -3, 2, -3, -11, -11, -2,
    -8, -2, -2, -4, -2, -38, -38, -17,
    -17, -5, -3, -1, -1, -1, -1, -1,
    -1, -1, 1, 1, 1, -3, -3, -2,
    -3, -5, -2, -4, -6, -24, -25, -24,
    -11, -3, -20, -3, -3, -1, 2, 2,
    1, 2, -16, -8, -8, -8, -8, -8,
    -8, -19, -8, -8, -6, -7, -6, -8,
    -4, -7, -8, -6, -2, 2, -20, -15,
    -20, -7, -1, -1, -1, 2, -4, -4,
    -4, -4, -4, -4, -4, -3, -3, -1,
    -1, 2, 1, -13, -6, -13, -4, 1,
    1, -3, -3, -3, -3, -3, -3, -3,
    -2, -2, 1, -15, -2, -2, -2, 1,
    -2, -2, -2, -2, -2, -2, -2, -3,
    -3, -3, 2, -5, -22, -14, -22, -14,
    -3, -9, -3, -3, -1, 2, -9, 2,
    2, 1, 2, 2, -6, -6, -6, -6,
    -2, -6, -4, -4, -6, -4, -6, -4,
    -5, -2, -4, -2, -2, -2, -3, 2,
    1, -2, -2, -2, -2, -2, -2, -2,
    -2, -2, -2, -3, -3, -3, -2, -2,
    -2, -2, -1, -1, -3, -3, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    2, 2, 2, 2, -2, -2, -2, -2,
    -2, 2, -7, -7, -2, -2, -2, -2,
    -2, -7, -7, -7, -7, -8, -8, -1,
    -2, -1, -1, -3, -3, -1, -1, -1,
    -1, 2, 2, -16, -16, -3, -2, -2,
    -2, 2, -2, -3, -2, 5, 2, 2,
    2, -3, 1, 1, -16, -16, -1, -1,
    -1, -1, 1, -1, -1, -1, -12, -12,
    -2, -2, -2, -2, -4, -2, 1, 1,
    -16, -16, -1, -1, -1, -1, 1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -2, -2
};

/*Collect the kern pair's data in one place*/
static const lv_font_fmt_txt_kern_pair_t kern_pairs =
{
    .glyph_ids = kern_pair_glyph_ids,
    .values = kern_pair_values,
    .pair_cnt = 426,
    .glyph_ids_size = 0
};

/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LV_VERSION_CHECK(8, 0, 0)
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = &kern_pairs,
    .kern_scale = 16,
    .cmap_num = 3,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LV_VERSION_CHECK(8, 0, 0)
    .cache = &cache
#endif
};


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LV_VERSION_CHECK(8, 0, 0)
const lv_font_t rm_12 = {
#else
lv_font_t rm_12 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 14,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc           /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
};



#endif /*#if RM_12*/

