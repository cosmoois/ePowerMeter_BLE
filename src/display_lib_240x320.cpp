#include <LovyanGFX.hpp>
#include "display_lib.h"
#include "obd2_lib.h"
#include <stdint.h>
#undef U8G2_FONT_SECTION
#define U8G2_FONT_SECTION(name) 
// u8g2のフォントを利用。cファイルをhに変更して取得
// https://github.com/olikraus/u8g2/tree/master/tools/font/build/single_font_files
#include "u8g2_font_logisoso46_tn.h"
#include "u8g2_font_FreeSans.h"
static const lgfx::U8g2font u8g2font1( u8g2_font_logisoso46_tn );
static const lgfx::U8g2font u8g2font92(u8g2_font_FreeSansBoldOblique92pt7b);
static const lgfx::U8g2font u8g2font40(u8g2_font_FreeSansBoldOblique40pt7b);

#define TFT_WIDTH  320
#define TFT_HEIGHT 240

#define DISP_GND -1
#define DISP_VCC 4
#define DISP_SCK 5
#define DISP_SDA 6
#define DISP_RES 7
#define DISP_DC  8
#define DISP_CS  10
LGFX_ESP32C3_ST7789_SPI display(TFT_HEIGHT, TFT_WIDTH, DISP_SCK, DISP_SDA, DISP_RES, DISP_DC, DISP_CS, -1, DISP_VCC, DISP_GND);
LGFX_Sprite canvas(&display);
LGFX_Sprite disp_log(&display);

int fontsize = 60;
int y1_base = fontsize * 1 - 2; // 1行目ベースライン
int y2_base = fontsize * 2 - 2; // 2行目ベースライン
int y3_base = fontsize * 3 - 2; // 3行目ベースライン
int y4_base = fontsize * 4 - 2; // 4行目ベースライン
int backlight = 3;
bool br_chg_ena = false;
int br_chg_cnt = 0;

void DrawCounter(int line, float val, int format, int rx);
void DrawParameter(String PreWord, float value, String SufWord, int warn, int dpt = -1);
void setDispBrightness();

void display_init() {
  display.init();
  display.setColorDepth(8);
  display.setRotation(3);

  canvas.setColorDepth(8);
  canvas.createSprite(TFT_WIDTH, TFT_HEIGHT);
  canvas.setColor(TFT_WHITE);
  canvas.setTextColor(TFT_WHITE);
  // canvas.drawRect(0, 0, TFT_WIDTH, TFT_HEIGHT, TFT_WHITE);  // 描画領域確認用
  
  disp_log.setColorDepth(8);
  disp_log.createSprite(TFT_WIDTH, TFT_HEIGHT);
  disp_log.setTextWrap(true);
  disp_log.setTextScroll(true);
  disp_log.setTextColor(TFT_WHITE);
  disp_log.setBaseColor(TFT_NAVY);
  disp_log.fillScreen(TFT_NAVY);

  disp_log.setFont(&fonts::Font4);
  disp_log.setCursor(0, 0);
  disp_log.println("e-Power Meter");
  disp_log.setFont(&fonts::Font2);
  disp_log.pushSprite(0, 0);

  setDispBrightness();
}

void display_inpane_draw() {
  int x_base = 260;

  canvas.clear();
  canvas.setTextSize(1);
  canvas.setTextWrap(false);
  canvas.setTextColor(TFT_WHITE);
  if (br_chg_ena == true) {
    br_chg_ena = false;
    if (backlight == 1) {
      backlight = 4;
    } else {
      backlight--;
    }
    setDispBrightness();
    br_chg_cnt = 30;
  }

  canvas.setFont(&fonts::FreeSansBoldOblique12pt7b);
  canvas.setTextDatum(textdatum_t::baseline_left);
  canvas.drawString("engine", 0, y1_base);
  canvas.drawString("motor", 0, y2_base);

  DrawCounter(0, obd2_eng_rpm, 0, x_base);
  if (abs(obd2_vcm_rpm) > 3183) { // 最大トルク範囲を超えた
    canvas.setTextColor(TFT_SKYBLUE);
  }
  if (obd2_vcm_rpm < 0) {
    canvas.setTextColor(TFT_ORANGE);
    DrawCounter(1, -obd2_vcm_rpm, 0, x_base);
  } else {
    DrawCounter(1, obd2_vcm_rpm, 0, x_base);
  }
  canvas.setTextColor(TFT_WHITE);

  canvas.setFont(&fonts::FreeSansBoldOblique12pt7b);
  canvas.setTextDatum(textdatum_t::baseline_left);
  canvas.drawString("rpm", x_base, y1_base);
  canvas.drawString("rpm", x_base, y2_base);

  // canvas.setCursor(0, y3_base);
  // DrawParameter("ped", obd2_pedal_d, "%", -99, 10);
  // canvas.setCursor(162, y3_base);
  // DrawParameter("流量", obd2_throttle, "%", -99, 10);
  canvas.setCursor(0, y3_base);
  DrawParameter("電圧", obd2_bms_volt, "V", -99);
  canvas.setCursor(162, y3_base);
  DrawParameter("電流", obd2_bms_curr, "A", -99, 10);
  canvas.setCursor(0, y4_base);
  DrawParameter("SOC", obd2_bms_soc, "%", 30, 10);
  canvas.setCursor(190, y4_base);
  DrawParameter("水温", obd2_eng_temp, "℃", -99);

  if (br_chg_cnt > 0) {
    canvas.setTextDatum(textdatum_t::top_right);
    canvas.setFont(&fonts::Font4);
    canvas.drawString(String(backlight), TFT_WIDTH, 0);
    br_chg_cnt--;
  }

  // if (lost_packet_cnt > 0) {
  //   canvas.setTextDatum(textdatum_t::top_right);
  //   canvas.setFont(&fonts::Font2);
  //   char cntbuf[9];
  //   sprintf(cntbuf, "%3d", lost_packet_cnt);
  //   canvas.drawString(String(cntbuf), TFT_WIDTH, 0);
  // }

  // canvas.drawRect(0, 0, TFT_WIDTH, TFT_HEIGHT, TFT_WHITE);  // レイアウト確認用
  // canvas.drawFastHLine(0, y1_base, TFT_WIDTH, TFT_WHITE); // レイアウト確認用
  // canvas.drawFastHLine(0, y2_base, TFT_WIDTH, TFT_WHITE); // レイアウト確認用
  // canvas.drawFastHLine(0, y3_base, TFT_WIDTH, TFT_WHITE); // レイアウト確認用
  // canvas.drawFastHLine(0, y4_base, TFT_WIDTH, TFT_WHITE); // レイアウト確認用
  canvas.pushSprite(0, 0);
}

void display_brightness_shift() {
  br_chg_ena = true;
}

void setDispBrightness() {
  switch (backlight)
  {
  case 4:
    if (DISP_GND != -1) gpio_set_drive_capability((gpio_num_t)DISP_GND, GPIO_DRIVE_CAP_3);
    if (DISP_VCC != -1) gpio_set_drive_capability((gpio_num_t)DISP_VCC, GPIO_DRIVE_CAP_3);
    break;
  case 3:
    if (DISP_GND != -1) gpio_set_drive_capability((gpio_num_t)DISP_GND, GPIO_DRIVE_CAP_1);
    if (DISP_VCC != -1) gpio_set_drive_capability((gpio_num_t)DISP_VCC, GPIO_DRIVE_CAP_1);
    break;
  case 2:
    if (DISP_GND != -1) gpio_set_drive_capability((gpio_num_t)DISP_GND, GPIO_DRIVE_CAP_2);
    if (DISP_VCC != -1) gpio_set_drive_capability((gpio_num_t)DISP_VCC, GPIO_DRIVE_CAP_2);
    break;
  case 1:
    if (DISP_GND != -1) gpio_set_drive_capability((gpio_num_t)DISP_GND, GPIO_DRIVE_CAP_0);
    if (DISP_VCC != -1) gpio_set_drive_capability((gpio_num_t)DISP_VCC, GPIO_DRIVE_CAP_0);
    break;
  }
}

/**
 * @brief カウンタ表示
 * 
 * @param line 描画行
 * @param val 描画値
 * @param format 少数点位置
 * @param rx 右寄せ位置
 */
void DrawCounter(int line, float val, int format, int rx)
{
  String Str_set;
  char str[32];
  int num_size;   // 数字フォント基準幅（フォント毎に固定：調整不可）
  int dot_size;   // 小数点フォント幅（フォント毎に固定：調整不可）
  int x_adj;      // x軸右寄せ調整値（フォント毎に固定：調整不可）

  int line_space; // 行間隔
  int y_offset;   // Y軸オフセット

  if (format == 0) {
    Str_set = String((int)val);
  } else {
    Str_set = String(val, format);
  }
  Str_set.toCharArray(str, sizeof(str));

  canvas.setTextDatum(textdatum_t::top_left);

  canvas.setFont(&u8g2font40); num_size = 40;  dot_size = 20;  x_adj = 12;
  line_space = 60;
  y_offset = 4;

  canvas.drawString(str, rx - (strlen(str) * num_size) - (format ? 0 : dot_size) + x_adj, (line * line_space) + y_offset);
}

void DrawParameter(String PreWord, float value, String SufWord, int warn, int dpt)
{
  String str_val;
  int int_val;

  canvas.setTextColor(TFT_WHITE);
  canvas.setFont(&fonts::lgfxJapanGothic_20);
  canvas.print(PreWord);

  if (value < 0) {
    canvas.setTextColor(TFT_ORANGE);
    value = -value;
  } else {
    canvas.setTextColor(TFT_WHITE);
  }
  int_val = (int)value;

  if (value == -99) {
    str_val = "--";
  } else {
    char str[10] = "";
    if (value >= 100) {
      sprintf(str, "%3d", int_val);
    } else if (value >= 10) {
      sprintf(str, "%2d", int_val);
    } else {
      sprintf(str, " %1d", int_val);
    }
    if (dpt == -1) {
      str_val = String(str);
    } else {
      str_val = String(str) + "." + String((int)((value - int_val) * dpt));
    }

    if (value < warn) {
      canvas.setTextColor(TFT_RED, TFT_WHITE);
    }
  }
  canvas.setFont(&u8g2font1);
  canvas.print(str_val);

  canvas.setTextColor(TFT_WHITE);
  canvas.setFont(&fonts::lgfxJapanGothic_20);
  canvas.print(SufWord);
}
