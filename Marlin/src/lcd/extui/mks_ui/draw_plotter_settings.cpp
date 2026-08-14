/**
 * Pen plotter settings screen: pen Z heights, plotter speed and favorite demo.
 * Values are edited through the number keypad and saved to EEPROM.
 */

#include "../../../inc/MarlinConfigPre.h"

#if HAS_TFT_LVGL_UI

#include "draw_ui.h"
#include <lv_conf.h>

#include "../../../feature/penplotter/penplotter_settings.h"
#include "../../../inc/MarlinConfig.h"
#include "../../../lcd/marlinui.h"
#include "../../../module/motion.h"
#include "../../../module/settings.h"

extern lv_group_t *g;
static lv_obj_t *scr;

enum {
  ID_PS_PENUP = 1,
  ID_PS_PENDOWN,
  ID_PS_SPEED,
  ID_PS_DEMO,
  ID_PS_SAVE,
  ID_PS_RESET,
  ID_PS_RETURN
};

static void redraw_plotter_settings() {
  lv_clear_plotter_settings();
  lv_draw_plotter_settings();
}

static void event_handler(lv_obj_t *obj, lv_event_t event) {
  if (event != LV_EVENT_RELEASED) return;

  switch (obj->mks_obj_id) {
    case ID_PS_PENUP:
      value = PlotterPenUpZ;
      lv_clear_plotter_settings();
      lv_draw_number_key();
      break;
    case ID_PS_PENDOWN:
      value = PlotterPenDownZ;
      lv_clear_plotter_settings();
      lv_draw_number_key();
      break;
    case ID_PS_SPEED:
      value = PlotterSpeedPct;
      lv_clear_plotter_settings();
      lv_draw_number_key();
      break;
    case ID_PS_DEMO:
      value = PlotterFavoriteDemo;
      lv_clear_plotter_settings();
      lv_draw_number_key();
      break;
    case ID_PS_SAVE:
      TERN_(EEPROM_SETTINGS, (void)settings.save());
      ui.set_status(F("Plotter settings saved"));
      break;
    case ID_PS_RESET:
      penplotter_settings_reset();
      penplotter_settings_sanitize();
      feedrate_percentage = 100;
      redraw_plotter_settings();
      break;
    case ID_PS_RETURN:
      goto_previous_ui();
      break;
  }
}

static lv_obj_t *setting_row(const char *name, const char *value, const lv_coord_t y, const int id) {
  lv_obj_t *btn = lv_btn_create(scr, 12, y, 456, 42, event_handler, id, &style_para_value);
  lv_obj_t *name_label = lv_label_create_empty(btn);
  lv_label_set_text(name_label, name);
  lv_obj_set_style(name_label, &tft_style_label_rel);
  lv_obj_align(name_label, btn, LV_ALIGN_IN_LEFT_MID, 24, 0);
  lv_obj_t *value_label = lv_label_create_empty(btn);
  lv_label_set_text(value_label, value);
  lv_obj_set_style(value_label, &style_sel_text);
  lv_obj_align(value_label, btn, LV_ALIGN_IN_RIGHT_MID, -24, 0);
  if (TERN0(HAS_ROTARY_ENCODER, gCfgItems.encoder_enable))
    lv_group_add_obj(g, btn);
  return btn;
}

void lv_draw_plotter_settings() {
  penplotter_settings_sanitize();
  scr = lv_screen_create(PLOTTER_SETTINGS_UI, "Plotter Settings");

  plotter_status_create(scr);
  plotter_status_start();

  char line[64], buf[16];
  dtostrf(penplotter_settings.pen_up_z, 1, 2, buf);
  snprintf_P(line, sizeof(line), PSTR("%s mm"), buf);
  setting_row("Pen Up Z", line, 84, ID_PS_PENUP);
  dtostrf(penplotter_settings.pen_down_z, 1, 2, buf);
  snprintf_P(line, sizeof(line), PSTR("%s mm"), buf);
  setting_row("Pen Down Z", line, 128, ID_PS_PENDOWN);
  snprintf_P(line, sizeof(line), PSTR("%d %%"), feedrate_percentage);
  setting_row("Plotter Speed", line, 172, ID_PS_SPEED);
  snprintf_P(line, sizeof(line), PSTR("%u"), penplotter_settings.favorite_demo);
  setting_row("Favorite Demo", line, 216, ID_PS_DEMO);

  lv_obj_t *save = lv_btn_create(scr, 12, 262, 144, 42, event_handler, ID_PS_SAVE, &style_android_accent);
  lv_obj_t *save_label = lv_label_create_empty(save);
  lv_label_set_text(save_label, "Save");
  lv_obj_align(save_label, save, LV_ALIGN_CENTER, 0, 0);
  lv_obj_t *reset = lv_btn_create(scr, 168, 262, 144, 42, event_handler, ID_PS_RESET, &style_para_value);
  lv_obj_t *reset_label = lv_label_create_empty(reset);
  lv_label_set_text(reset_label, "Reset");
  lv_obj_align(reset_label, reset, LV_ALIGN_CENTER, 0, 0);
  lv_obj_t *back = lv_btn_create(scr, 324, 262, 144, 42, event_handler, ID_PS_RETURN, &style_para_value);
  lv_obj_t *back_label = lv_label_create_empty(back);
  lv_label_set_text(back_label, common_menu.text_back);
  lv_obj_align(back_label, back, LV_ALIGN_CENTER, 0, 0);

  lv_android_home_indicator(scr);
}

void lv_clear_plotter_settings() {
  plotter_status_stop();
  #if HAS_ROTARY_ENCODER
    if (gCfgItems.encoder_enable) lv_group_remove_all_objs(g);
  #endif
  lv_obj_del(scr);
}

#endif // HAS_TFT_LVGL_UI
