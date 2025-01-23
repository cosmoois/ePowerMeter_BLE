#pragma once

#include <LovyanGFX.hpp>
#include "LGFX_ESP32C3_ST7789_SPI.hpp"

extern LGFX_ESP32C3_ST7789_SPI display;
extern LGFX_Sprite canvas;
extern LGFX_Sprite disp_log;

void display_init();
void display_inpane_draw();
void display_brightness_shift();
