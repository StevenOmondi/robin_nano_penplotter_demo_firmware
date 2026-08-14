/**
 * Z-axis pen calibration screen for the pen plotter.
 */

#include "../../../inc/MarlinConfigPre.h"

#if HAS_TFT_LVGL_UI

#include "draw_ui.h"
#include <lv_conf.h>

#include "../../../feature/penplotter/penplotter_settings.h"
#include "../../../gcode/queue.h"
#include "../../../inc/MarlinConfig.h"
#include "../../../lcd/marlinui.h"
#include "../../../module/settings.h"

extern lv_group_t *g;
static lv_obj_t *scr;

enum {
  ID_CAL_UP_PLUS = 1,
  ID_CAL_UP_MINUS,
  ID_CAL_DOWN_PLUS,
  ID_CAL_DOWN_MINUS,
  ID_CAL_TEST,
  ID_CAL_PAPER,
  ID_CAL_SAVE,
  ID_CAL_RETURN
};

static lv_obj_t *cal_button(const char *text, const lv_coord_t x, const lv_coord_t y, const int id, lv_event_cb_t cb, lv_style_t *style = &style_para_value) {
  lv_obj_t *btn = lv_btn_create(scr, x, y, 106, 50, cb, id, style);
  lv_obj_t *label = lv_label_create_empty(btn);
  lv_label_set_text(label, text);
  lv_obj_align(label, btn, LV_ALIGN_CENTER, 0, 0);
  if (TERN0(HAS_ROTARY_ENCODER, gCfgItems.encoder_enable))
    lv_group_add_obj(g, btn);
  return btn;
}

static void adjust_pen_up(const float delta) {
  penplotter_settings.pen_up_z += delta;
  penplotter_settings_sanitize();
}

static void adjust_pen_down(const float delta) {
  penplotter_settings.pen_down_z += delta;
  penplotter_settings_sanitize();
}

static void redraw_calibration() {
  lv_clear_pen_calibration();
  lv_draw_pen_calibration();
}

static void event_handler(lv_obj_t *obj, lv_event_t event) {
  if (event != LV_EVENT_RELEASED) return;

  switch (obj->mks_obj_id) {
    case ID_CAL_UP_PLUS:
      adjust_pen_up(0.10f);
      redraw_calibration();
      break;
    case ID_CAL_UP_MINUS:
      adjust_pen_up(-0.10f);
      redraw_calibration();
      break;
    case ID_CAL_DOWN_PLUS:
      adjust_pen_down(0.05f);
      redraw_calibration();
      break;
    case ID_CAL_DOWN_MINUS:
      adjust_pen_down(-0.05f);
      redraw_calibration();
      break;
    case ID_CAL_TEST:
      queue.enqueue_one(F("M751 C"));
      ui.set_status(F("Tap queued; homing first"));
      break;
    case ID_CAL_PAPER:
      penplotter_settings.pen_down_z = 0.05f;
      penplotter_settings_sanitize();
      redraw_calibration();
      break;
    case ID_CAL_SAVE:
      TERN_(EEPROM_SETTINGS, (void)settings.save());
      ui.set_status(F("Pen calibration saved"));
      break;
    case ID_CAL_RETURN:
      goto_previous_ui();
      break;
  }
}

void lv_draw_pen_calibration() {
  penplotter_settings_sanitize();
  scr = lv_screen_create(PLOTTER_CALIB_UI, "Calibrate");

  lv_obj_t *panel = lv_obj_create(scr, nullptr);
  lv_obj_set_style(panel, &style_android_panel);
  lv_obj_set_pos(panel, 12, 44);
  lv_obj_set_size(panel, 456, 70);

  lv_obj_t *calIcon = lv_img_create(panel, nullptr);
  lv_img_set_src(calIcon, "F:/bmp_plot_pen.bin");
  lv_obj_set_pos(calIcon, 18, 15);

  char line[96], up[12], down[12];
  dtostrf(penplotter_settings.pen_up_z, 1, 2, up);
  dtostrf(penplotter_settings.pen_down_z, 1, 2, down);
  snprintf_P(line, sizeof(line), PSTR("PenUp %s  PenDown %s"), up, down);
  lv_obj_t *label = lv_label_create(panel, 62, 10, line);
  lv_obj_set_style(label, &tft_style_label_rel);
  lv_obj_t *hint = lv_label_create(panel, 62, 40, "Tap test homes X/Y, moves to center, lowers Z, then lifts.");
  lv_obj_set_style(hint, &style_android_muted);

  static const lv_coord_t x[4] = { 12, 126, 240, 354 };
  cal_button("Up\n+0.10", x[0], 130, ID_CAL_UP_PLUS, event_handler);
  cal_button("Up\n-0.10", x[1], 130, ID_CAL_UP_MINUS, event_handler);
  cal_button("Down\n+0.05", x[2], 130, ID_CAL_DOWN_PLUS, event_handler);
  cal_button("Down\n-0.05", x[3], 130, ID_CAL_DOWN_MINUS, event_handler);

  cal_button("Test\nTap", x[0], 200, ID_CAL_TEST, event_handler, &style_android_accent);
  cal_button("Paper\n0.05", x[1], 200, ID_CAL_PAPER, event_handler);
  cal_button("Save", x[2], 200, ID_CAL_SAVE, event_handler);
  cal_button(common_menu.text_back, x[3], 200, ID_CAL_RETURN, event_handler);

  lv_android_home_indicator(scr);
}

void lv_clear_pen_calibration() {
  #if HAS_ROTARY_ENCODER
    if (gCfgItems.encoder_enable) lv_group_remove_all_objs(g);
  #endif
  lv_obj_del(scr);
}

#endif // HAS_TFT_LVGL_UI
