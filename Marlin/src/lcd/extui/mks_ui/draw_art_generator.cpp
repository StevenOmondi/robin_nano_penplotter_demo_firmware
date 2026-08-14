/**
 * Generated art screen for the pen plotter.
 */

#include "../../../inc/MarlinConfigPre.h"

#if HAS_TFT_LVGL_UI

#include "draw_ui.h"
#include <lv_conf.h>

#include "../../../gcode/queue.h"
#include "../../../inc/MarlinConfig.h"
#include "../../../lcd/marlinui.h"

extern lv_group_t *g;
static lv_obj_t *scr;

static uint8_t art_shape_index = 0;
static uint8_t art_size_index = 2;
static uint8_t art_density_index = 1;
static bool art_dry_run = true;

static const char * const shape_names[] = { "Square", "Circle", "Spiral", "Star", "Grid", "Hatch", "Waves", "Mandala", "Spiro" };
static constexpr uint8_t size_values[] = { 50, 90, 120, 160, 180 };
static constexpr uint8_t density_values[] = { 1, 3, 5, 7, 10 };

enum {
  ID_ART_SHAPE = 1,
  ID_ART_SIZE,
  ID_ART_DENSITY,
  ID_ART_DRY,
  ID_ART_DRAW,
  ID_ART_RETURN
};

static lv_obj_t *art_button(const char *text, const lv_coord_t x, const lv_coord_t y, const int id, lv_event_cb_t cb, lv_style_t *style = &style_para_value) {
  lv_obj_t *btn = lv_btn_create(scr, x, y, 106, 50, cb, id, style);
  lv_obj_t *label = lv_label_create_empty(btn);
  lv_label_set_text(label, text);
  lv_obj_align(label, btn, LV_ALIGN_CENTER, 0, 0);
  if (TERN0(HAS_ROTARY_ENCODER, gCfgItems.encoder_enable))
    lv_group_add_obj(g, btn);
  return btn;
}

static void queue_art_draw() {
  char cmd[MAX_CMD_SIZE];
  snprintf_P(cmd, sizeof(cmd), PSTR("M753 A%u S%u D%u%s"),
    art_shape_index,
    size_values[art_size_index],
    density_values[art_density_index],
    art_dry_run ? " P1" : ""
  );
  if (!queue.enqueue_one(cmd)) {
    ui.set_status(F("Queue busy"));
    return;
  }
  ui.set_status(F("Art queued; homing first"));
}

static void redraw_art() {
  lv_clear_art_generator();
  lv_draw_art_generator();
}

static void event_handler(lv_obj_t *obj, lv_event_t event) {
  if (event != LV_EVENT_RELEASED) return;

  switch (obj->mks_obj_id) {
    case ID_ART_SHAPE:
      art_shape_index = (art_shape_index + 1) % COUNT(shape_names);
      redraw_art();
      break;
    case ID_ART_SIZE:
      art_size_index = (art_size_index + 1) % COUNT(size_values);
      redraw_art();
      break;
    case ID_ART_DENSITY:
      art_density_index = (art_density_index + 1) % COUNT(density_values);
      redraw_art();
      break;
    case ID_ART_DRY:
      art_dry_run = !art_dry_run;
      redraw_art();
      break;
    case ID_ART_DRAW:
      queue_art_draw();
      break;
    case ID_ART_RETURN:
      goto_previous_ui();
      break;
  }
}

void lv_draw_art_generator() {
  scr = lv_screen_create(PLOTTER_ART_UI, "Art");

  plotter_status_create(scr);
  plotter_status_start();

  lv_obj_t *panel = lv_obj_create(scr, nullptr);
  lv_obj_set_style(panel, &style_android_panel);
  lv_obj_set_pos(panel, 12, 84);
  lv_obj_set_size(panel, 456, 66);

  lv_obj_t *brushIcon = lv_img_create(panel, nullptr);
  lv_img_set_src(brushIcon, "F:/bmp_plot_brush.bin");
  lv_obj_set_pos(brushIcon, 18, 13);

  char line[96];
  snprintf_P(line, sizeof(line), PSTR("%s  %umm  Density %u"),
    shape_names[art_shape_index], size_values[art_size_index], density_values[art_density_index]);
  lv_obj_t *label = lv_label_create(panel, 62, 10, line);
  lv_obj_set_style(label, &tft_style_label_rel);
  snprintf_P(line, sizeof(line), PSTR("%s   Centered 200x200"), art_dry_run ? "Dry run" : "Draw");
  lv_obj_t *meta = lv_label_create(panel, 62, 38, line);
  lv_obj_set_style(meta, &style_android_muted);

  static const lv_coord_t x[4] = { 12, 126, 240, 354 };
  snprintf_P(line, sizeof(line), PSTR("Shape\n%s"), shape_names[art_shape_index]);
  art_button(line, x[0], 160, ID_ART_SHAPE, event_handler);
  snprintf_P(line, sizeof(line), PSTR("Size\n%umm"), size_values[art_size_index]);
  art_button(line, x[1], 160, ID_ART_SIZE, event_handler);
  snprintf_P(line, sizeof(line), PSTR("Density\n%u"), density_values[art_density_index]);
  art_button(line, x[2], 160, ID_ART_DENSITY, event_handler);
  snprintf_P(line, sizeof(line), PSTR("Dry\n%s"), art_dry_run ? "On" : "Off");
  art_button(line, x[3], 160, ID_ART_DRY, event_handler);

  art_button("Draw", x[1], 222, ID_ART_DRAW, event_handler, &style_android_accent);
  art_button(common_menu.text_back, x[3], 222, ID_ART_RETURN, event_handler);

  lv_android_home_indicator(scr);
}

void lv_clear_art_generator() {
  plotter_status_stop();
  #if HAS_ROTARY_ENCODER
    if (gCfgItems.encoder_enable) lv_group_remove_all_objs(g);
  #endif
  lv_obj_del(scr);
}

#endif // HAS_TFT_LVGL_UI
