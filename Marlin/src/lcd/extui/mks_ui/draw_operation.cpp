/**
 * Pen plotter live drawing controls.
 */

#include "../../../inc/MarlinConfigPre.h"

#if HAS_TFT_LVGL_UI

#include "draw_ui.h"
#include <lv_conf.h>

#include "../../../feature/penplotter/penplotter_settings.h"
#include "../../../gcode/queue.h"
#include "../../../inc/MarlinConfig.h"
#include "../../../lcd/marlinui.h"
#include "../../../module/motion.h"

extern lv_group_t *g;
static lv_obj_t *scr;
static lv_obj_t *statusLabel;
static lv_task_t *updateStatusTask;

enum {
  ID_O_PAUSE = 1,
  ID_O_RESUME,
  ID_O_STOP,
  ID_O_SPEED,
  ID_O_PEN_UP,
  ID_O_PEN_DOWN,
  ID_O_JOG,
  ID_O_RETURN
};

static lv_obj_t *control_button(const char *text, const lv_coord_t x, const lv_coord_t y, const int id, lv_event_cb_t cb) {
  lv_obj_t *btn = lv_btn_create(scr, x, y, 106, 50, cb, id, &style_para_value);
  lv_obj_t *label = lv_label_create_empty(btn);
  lv_label_set_text(label, text);
  lv_obj_align(label, btn, LV_ALIGN_CENTER, 0, 0);
  if (TERN0(HAS_ROTARY_ENCODER, gCfgItems.encoder_enable))
    lv_group_add_obj(g, btn);
  return btn;
}

static void disp_operation_status() {
  if (!statusLabel) return;
  const char *state = penplotter_is_stopped() ? "Stopped" : (penplotter_is_paused() ? "Paused" : (penplotter_is_busy() ? "Drawing" : "Ready"));
  char status[48];
  snprintf_P(status, sizeof(status), PSTR("Speed %d%%  Progress %u%%  %s"), feedrate_percentage, penplotter_progress_percent(), state);
  lv_label_set_text(statusLabel, status);
}

static void refresh_operation(lv_task_t *) {
  disp_operation_status();
}

static void event_handler(lv_obj_t *obj, lv_event_t event) {
  if (event != LV_EVENT_RELEASED) return;

  switch (obj->mks_obj_id) {
    case ID_O_PAUSE:
      penplotter_request_pause(true);
      ui.set_status(F("Plotter paused"));
      break;
    case ID_O_RESUME:
      penplotter_request_pause(false);
      ui.set_status(F("Plotter resumed"));
      break;
    case ID_O_STOP:
      penplotter_request_stop();
      queue.clear();
      ui.set_status(F("Plotter stopped"));
      break;
    case ID_O_SPEED:
      lv_clear_operation();
      lv_draw_change_speed();
      return;
    case ID_O_PEN_UP:
      queue.enqueue_one(F("M751 U"));
      break;
    case ID_O_PEN_DOWN:
      queue.enqueue_one(F("M751 D"));
      break;
    case ID_O_JOG:
      lv_clear_operation();
      lv_draw_move_motor();
      return;
    case ID_O_RETURN:
      goto_previous_ui();
      return;
  }
  disp_operation_status();
}

void lv_draw_operation() {
  scr = lv_screen_create(OPERATE_UI, "Controls");

  lv_obj_t *panel = lv_obj_create(scr, nullptr);
  lv_obj_set_style(panel, &style_android_panel);
  lv_obj_set_pos(panel, 12, 44);
  lv_obj_set_size(panel, 456, 58);
  statusLabel = lv_label_create(panel, 12, 18, "");
  lv_obj_set_style(statusLabel, &tft_style_label_rel);

  static const lv_coord_t x[4] = { 12, 126, 240, 354 };
  control_button("Pause", x[0], 122, ID_O_PAUSE, event_handler);
  control_button("Resume", x[1], 122, ID_O_RESUME, event_handler);
  control_button("Stop", x[2], 122, ID_O_STOP, event_handler);
  control_button("Speed", x[3], 122, ID_O_SPEED, event_handler);

  control_button("Lift", x[0], 192, ID_O_PEN_UP, event_handler);
  control_button("Draw", x[1], 192, ID_O_PEN_DOWN, event_handler);
  control_button("Jog", x[2], 192, ID_O_JOG, event_handler);
  control_button(common_menu.text_back, x[3], 192, ID_O_RETURN, event_handler);

  disp_operation_status();
  updateStatusTask = lv_task_create(refresh_operation, 300, LV_TASK_PRIO_LOWEST, 0);
  lv_android_home_indicator(scr);
}

void lv_clear_operation() {
  #if HAS_ROTARY_ENCODER
    if (gCfgItems.encoder_enable) lv_group_remove_all_objs(g);
  #endif
  if (updateStatusTask) {
    lv_task_del(updateStatusTask);
    updateStatusTask = nullptr;
  }
  statusLabel = nullptr;
  lv_obj_del(scr);
}

#endif // HAS_TFT_LVGL_UI
