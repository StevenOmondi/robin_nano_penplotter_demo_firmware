/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "../../../inc/MarlinConfigPre.h"

#if HAS_TFT_LVGL_UI

#include "draw_ready_print.h"
#include <lv_conf.h>
#include "tft_lvgl_configuration.h"
#include "draw_ui.h"

#include <lvgl.h>

#include "../../../module/temperature.h"
#include "../../../feature/penplotter/penplotter_settings.h"
#include "../../../inc/MarlinConfig.h"

#if ENABLED(TOUCH_SCREEN_CALIBRATION)
  #include "../../tft_io/touch_calibration.h"
  #include "draw_touch_calibration.h"
#endif

#include "mks_hardware.h"
#include <stdio.h>

#define ICON_POS_Y          260
#define TARGET_LABEL_MOD_Y -36
#define LABEL_MOD_Y         30

extern lv_group_t*  g;
static lv_obj_t *scr;
static lv_obj_t *buttonExt1, *labelExt1, *buttonFanstate, *labelFan;

#if HAS_MULTI_HOTEND
  static lv_obj_t *labelExt2;
  static lv_obj_t *buttonExt2;
#endif

#if HAS_HEATED_BED
  static lv_obj_t* labelBed;
  static lv_obj_t* buttonBedstate;
#endif

#if ENABLED(MKS_TEST)
  uint8_t current_disp_ui = 0;
#endif

enum { ID_T_WORDS = 1, ID_T_ART, ID_T_MORE, ID_T_CONTROLS, ID_T_CALIBRATE, ID_T_MOV, ID_T_HOME, ID_T_SETTINGS };

static void event_handler(lv_obj_t *obj, lv_event_t event) {
  if (event != LV_EVENT_RELEASED) return;
  if (obj->mks_obj_id == ID_T_SETTINGS) { lv_clear_ready_print(); lv_draw_plotter_settings(); return; }
  lv_clear_ready_print();
  switch (obj->mks_obj_id) {
    case ID_T_WORDS:    lv_draw_word_writer(); break;
    case ID_T_ART:      lv_draw_art_generator(); break;
    case ID_T_MORE:     lv_draw_more(); break;
    case ID_T_CONTROLS: lv_draw_operation(); break;
    case ID_T_CALIBRATE:lv_draw_pen_calibration(); break;
    case ID_T_MOV:      lv_draw_move_motor(); break;
    case ID_T_HOME:     lv_draw_home(); break;
  }
}

lv_obj_t *limit_info, *det_info;
lv_style_t limit_style, det_style;
void disp_Limit_ok() {
  limit_style.text.color.full = 0xFFFF;
  lv_obj_set_style(limit_info, &limit_style);
  lv_label_set_text(limit_info, "Limit:ok");
}
void disp_Limit_error() {
  limit_style.text.color.full = 0xF800;
  lv_obj_set_style(limit_info, &limit_style);
  lv_label_set_text(limit_info, "Limit:error");
}

void disp_det_ok() {
  det_style.text.color.full = 0xFFFF;
  lv_obj_set_style(det_info, &det_style);
  lv_label_set_text(det_info, "det:ok");
}

void disp_det_error() {
  det_style.text.color.full = 0xF800;
  lv_obj_set_style(det_info, &det_style);
  lv_label_set_text(det_info, "det:error");
}

lv_obj_t *e1, *e2, *e3, *bed;
void mks_disp_test() {
  char buf[30] = {0};
  #if HAS_HOTEND
    sprintf_P(buf, PSTR("e1:%d"), thermalManager.wholeDegHotend(0));
    lv_label_set_text(e1, buf);
  #endif
  #if HAS_MULTI_HOTEND
    sprintf_P(buf, PSTR("e2:%d"), thermalManager.wholeDegHotend(1));
    lv_label_set_text(e2, buf);
  #endif
  #if HAS_HEATED_BED
    sprintf_P(buf, PSTR("bed:%d"), thermalManager.wholeDegBed());
    lv_label_set_text(bed, buf);
  #endif
}

void lv_draw_ready_print() {
  char buf[30] = {0};
  lv_obj_t *buttonTool;

  disp_state_stack._disp_index = 0;
  ZERO(disp_state_stack._disp_state);
  scr = lv_screen_create(PRINT_READY_UI, "Plotter");

  if (mks_test_flag == 0x1E) {
    // Create image buttons
    buttonTool = lv_imgbtn_create(scr, "F:/bmp_tool.bin", event_handler, ID_T_SETTINGS);

    lv_obj_set_pos(buttonTool, 360, 180);

    lv_obj_t *label_tool = lv_label_create_empty(buttonTool);
    if (gCfgItems.multiple_language) {
      lv_label_set_text(label_tool, main_menu.tool);
      lv_obj_align(label_tool, buttonTool, LV_ALIGN_IN_BOTTOM_MID, 0, BUTTON_TEXT_Y_OFFSET);
    }

    #if HAS_HOTEND
      e1 = lv_label_create_empty(scr);
      lv_obj_set_pos(e1, 20, 20);
      sprintf_P(buf, PSTR("e1:  %d"), thermalManager.wholeDegHotend(0));
      lv_label_set_text(e1, buf);
    #endif
    #if HAS_MULTI_HOTEND
      e2 = lv_label_create_empty(scr);
      lv_obj_set_pos(e2, 20, 45);
      sprintf_P(buf, PSTR("e2:  %d"), thermalManager.wholeDegHotend(1));
      lv_label_set_text(e2, buf);
    #endif
    #if HAS_HEATED_BED
      bed = lv_label_create_empty(scr);
      lv_obj_set_pos(bed, 20, 95);
      sprintf_P(buf, PSTR("bed:  %d"), thermalManager.wholeDegBed());
      lv_label_set_text(bed, buf);
    #endif

    limit_info = lv_label_create_empty(scr);

    lv_style_copy(&limit_style, &lv_style_scr);
    limit_style.body.main_color.full = 0x0000;
    limit_style.body.grad_color.full = 0x0000;
    limit_style.text.color.full      = 0xFFFF;
    lv_obj_set_style(limit_info, &limit_style);

    lv_obj_set_pos(limit_info, 20, 120);
    lv_label_set_text(limit_info, " ");

    det_info = lv_label_create_empty(scr);

    lv_style_copy(&det_style, &lv_style_scr);
    det_style.body.main_color.full = 0x0000;
    det_style.body.grad_color.full = 0x0000;
    det_style.text.color.full      = 0xFFFF;
    lv_obj_set_style(det_info, &det_style);

    lv_obj_set_pos(det_info, 20, 145);
    lv_label_set_text(det_info, " ");
  }
  else {
    // Plotter hub: every action is one touch away.
    // Live status bar with pen state, position and job state
    penplotter_settings_sanitize();
    plotter_status_create(scr);
    plotter_status_start();

    // 2x4 icon grid; icons are 117x102 so rows must not use BTN_Y_PIXEL (140)
    lv_big_button_create(scr, "F:/bmp_plot_words.bin", "Words", INTERVAL_V, 84, event_handler, ID_T_WORDS);
    lv_big_button_create(scr, "F:/bmp_plot_art.bin", "Art", BTN_X_PIXEL + INTERVAL_V * 2, 84, event_handler, ID_T_ART);
    lv_big_button_create(scr, "F:/bmp_plot_demos.bin", "Demos", BTN_X_PIXEL * 2 + INTERVAL_V * 3, 84, event_handler, ID_T_MORE);
    lv_big_button_create(scr, "F:/bmp_plot_calibrate.bin", "Calibrate", BTN_X_PIXEL * 3 + INTERVAL_V * 4, 84, event_handler, ID_T_CALIBRATE);
    lv_big_button_create(scr, "F:/bmp_plot_controls.bin", "Controls", INTERVAL_V, 196, event_handler, ID_T_CONTROLS);
    lv_big_button_create(scr, "F:/bmp_plot_jog.bin", "Jog", BTN_X_PIXEL + INTERVAL_V * 2, 196, event_handler, ID_T_MOV);
    lv_big_button_create(scr, "F:/bmp_plot_home.bin", "Home", BTN_X_PIXEL * 2 + INTERVAL_V * 3, 196, event_handler, ID_T_HOME);
    lv_big_button_create(scr, "F:/bmp_plot_settings.bin", "Settings", BTN_X_PIXEL * 3 + INTERVAL_V * 4, 196, event_handler, ID_T_SETTINGS);
    lv_android_home_indicator(scr);
  }

  #if ENABLED(TOUCH_SCREEN_CALIBRATION)
    // If calibration is required, let's trigger it now, handles the case when there is default value in configuration files
    if (!touch_calibration.calibration_loaded()) {
      lv_clear_ready_print();
      lv_draw_touch_calibration_screen();
    }
  #endif
}

void lv_temp_refr() {
  #if HAS_HOTEND
    sprintf(public_buf_l, printing_menu.temp1, thermalManager.wholeDegHotend(0), thermalManager.degTargetHotend(0));
    lv_label_set_text(labelExt1, public_buf_l);
    lv_obj_align(labelExt1, buttonExt1, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
  #endif
  #if HAS_MULTI_HOTEND
    sprintf(public_buf_l, printing_menu.temp1, thermalManager.wholeDegHotend(1), thermalManager.degTargetHotend(1));
    lv_label_set_text(labelExt2, public_buf_l);
    lv_obj_align(labelExt2, buttonExt2, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
  #endif
  #if HAS_HEATED_BED
    sprintf(public_buf_l, printing_menu.bed_temp, thermalManager.wholeDegBed(), thermalManager.degTargetBed());
    lv_label_set_text(labelBed, public_buf_l);
    lv_obj_align(labelBed, buttonBedstate, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
  #endif
  #if HAS_FAN
    sprintf_P(public_buf_l, PSTR("%d%%"), (int)thermalManager.fanSpeedPercent(0));
    lv_label_set_text(labelFan, public_buf_l);
    lv_obj_align(labelFan, buttonFanstate, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
  #endif
}

void lv_clear_ready_print() {
  plotter_status_stop();
  #if HAS_ROTARY_ENCODER
    if (gCfgItems.encoder_enable) lv_group_remove_all_objs(g);
  #endif
  lv_obj_del(scr);
}

#endif // HAS_TFT_LVGL_UI
