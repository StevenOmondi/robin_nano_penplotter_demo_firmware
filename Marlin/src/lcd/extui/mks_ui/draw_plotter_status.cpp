/**
 * Shared live status bar for pen plotter screens.
 */

#include "../../../inc/MarlinConfigPre.h"

#if HAS_TFT_LVGL_UI

#include "draw_plotter_status.h"
#include "draw_ui.h"
#include <lv_conf.h>

#include "../../../feature/penplotter/penplotter_settings.h"
#include "../../../module/motion.h"

static lv_obj_t *status_pen_chip = nullptr;
static lv_obj_t *status_pos_label = nullptr;
static lv_obj_t *status_job_chip = nullptr;
static lv_task_t *status_task = nullptr;

void plotter_status_create(lv_obj_t *scr) {
  lv_obj_t *bar = lv_obj_create(scr, nullptr);
  lv_obj_set_style(bar, &style_plotter_status);
  lv_obj_set_pos(bar, 12, 42);
  lv_obj_set_size(bar, 456, 32);

  status_pen_chip = lv_obj_create(bar, nullptr);
  lv_obj_set_style(status_pen_chip, &style_android_chip);
  lv_obj_set_pos(status_pen_chip, 10, 4);
  lv_obj_set_size(status_pen_chip, 92, 24);
  lv_obj_t *pen_label = lv_label_create_empty(status_pen_chip);
  lv_obj_set_style(pen_label, &style_android_chip);
  lv_label_set_text(pen_label, "--");
  lv_obj_align(pen_label, status_pen_chip, LV_ALIGN_CENTER, 0, 0);

  status_pos_label = lv_label_create(bar, 110, 6, "X ---  Y ---");
  lv_obj_set_style(status_pos_label, &tft_style_label_rel);

  status_job_chip = lv_obj_create(bar, nullptr);
  lv_obj_set_style(status_job_chip, &style_android_chip);
  lv_obj_set_pos(status_job_chip, 334, 4);
  lv_obj_set_size(status_job_chip, 112, 24);
  lv_obj_t *job_label = lv_label_create_empty(status_job_chip);
  lv_obj_set_style(job_label, &style_android_chip);
  lv_label_set_text(job_label, "IDLE");
  lv_obj_align(job_label, status_job_chip, LV_ALIGN_CENTER, 0, 0);

  plotter_status_refresh(nullptr);
}

void plotter_status_start() {
  if (!status_task)
    status_task = lv_task_create(plotter_status_refresh, 300, LV_TASK_PRIO_LOWEST, 0);
}

void plotter_status_stop() {
  if (status_task) {
    lv_task_del(status_task);
    status_task = nullptr;
  }
}

void plotter_status_refresh(lv_task_t *) {
  if (!status_pen_chip) return;

  const float z = current_position.z;

  // Pen state chip
  lv_style_t *pen_style;
  const char *pen_text;
  if (z >= penplotter_settings.pen_up_z - 0.15f) {
    pen_style = &style_plotter_chip_ok;
    pen_text = "PEN UP";
  }
  else if (z <= penplotter_settings.pen_down_z + 0.15f) {
    pen_style = &style_plotter_chip_err;
    pen_text = "PEN DOWN";
  }
  else {
    pen_style = &style_plotter_chip_warn;
    pen_text = "MOVING";
  }
  lv_obj_set_style(status_pen_chip, pen_style);
  lv_obj_t *pen_label = lv_obj_get_child(status_pen_chip, nullptr);
  lv_obj_set_style(pen_label, pen_style);
  lv_label_set_text(pen_label, pen_text);

  // Position label
  char pos[32];
  if (axis_was_homed(X_AXIS) && axis_was_homed(Y_AXIS)) {
    char xb[12], yb[12];
    dtostrf(current_position.x, 1, 1, xb);
    dtostrf(current_position.y, 1, 1, yb);
    snprintf_P(pos, sizeof(pos), PSTR("X %s  Y %s"), xb, yb);
  }
  else {
    strcpy_P(pos, PSTR("X ---  Y ---"));
  }
  lv_label_set_text(status_pos_label, pos);

  // Job state chip
  lv_style_t *job_style;
  char job[20];
  if (penplotter_is_busy()) {
    if (penplotter_is_paused()) {
      job_style = &style_plotter_chip_warn;
      strcpy_P(job, PSTR("PAUSED"));
    }
    else if (penplotter_is_stopped()) {
      job_style = &style_plotter_chip_err;
      strcpy_P(job, PSTR("STOPPED"));
    }
    else {
      job_style = &style_android_accent;
      snprintf_P(job, sizeof(job), PSTR("BUSY %u%%"), penplotter_progress_percent());
    }
  }
  else {
    job_style = &style_android_chip;
    strcpy_P(job, PSTR("IDLE"));
  }
  lv_obj_set_style(status_job_chip, job_style);
  lv_obj_t *job_label = lv_obj_get_child(status_job_chip, nullptr);
  lv_obj_set_style(job_label, job_style);
  lv_label_set_text(job_label, job);
}

#endif // HAS_TFT_LVGL_UI
