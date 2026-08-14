/**
 * Plotter demo gallery: curated one-touch demos.
 */

#include "../../../inc/MarlinConfigPre.h"

#if HAS_TFT_LVGL_UI

#include "lv_conf.h"
#include "draw_ui.h"

#include "../../../gcode/queue.h"
#include "../../../inc/MarlinConfig.h"
#include "../../../lcd/marlinui.h"

extern lv_group_t *g;
static lv_obj_t *scr;
static bool demo_dry_run = true;

struct DemoItem {
  const char *label;
  const char *live_cmd;
  const char *dry_cmd;
};

static const DemoItem demo_items[] = {
  { "Words",    "M752 D3",                     "M752 D3 P1" },
  { "Paragraph","M752 F0 S18 A1 N\"PLOT.TXT\"", "M752 F0 S18 A1 N\"PLOT.TXT\" P1" },
  { "House",    "M750 D4",                     "M751 R" },
  { "Square",   "M753 A0 S120 D3",             "M753 A0 S120 D3 P1" },
  { "Circle",   "M753 A1 S130 D4",             "M753 A1 S130 D4 P1" },
  { "Spiral",   "M753 A2 S155 D5",             "M753 A2 S155 D5 P1" },
  { "Star",     "M753 A3 S145 D5",             "M753 A3 S145 D5 P1" },
  { "Grid",     "M753 A4 S150 D4",             "M753 A4 S150 D4 P1" },
  { "Hatch",    "M753 A5 S150 D4",             "M753 A5 S150 D4 P1" },
  { "Waves",    "M753 A6 S150 D5",             "M753 A6 S150 D5 P1" },
  { "Mandala",  "M753 A7 S165 D5",             "M753 A7 S165 D5 P1" },
  { "Spiro",    "M753 A8 S140 D6",             "M753 A8 S140 D6 P1" }
};

enum {
  ID_DEMO_0 = 1,
  ID_DEMO_1,
  ID_DEMO_2,
  ID_DEMO_3,
  ID_DEMO_4,
  ID_DEMO_5,
  ID_DEMO_6,
  ID_DEMO_7,
  ID_DEMO_8,
  ID_DEMO_9,
  ID_DEMO_10,
  ID_DEMO_11,
  ID_DEMO_DRY,
  ID_DEMO_RETURN
};

static lv_obj_t *demo_button(const char *text, const lv_coord_t x, const lv_coord_t y, const int id, lv_event_cb_t cb, lv_style_t *style = &style_para_value) {
  lv_obj_t *btn = lv_btn_create(scr, x, y, 106, 50, cb, id, style);
  lv_obj_t *label = lv_label_create_empty(btn);
  lv_label_set_text(label, text);
  lv_obj_align(label, btn, LV_ALIGN_CENTER, 0, 0);
  if (TERN0(HAS_ROTARY_ENCODER, gCfgItems.encoder_enable))
    lv_group_add_obj(g, btn);
  return btn;
}

static void queue_demo_command(const char *cmd) {
  if (!queue.enqueue_one(cmd)) {
    ui.set_status(F("Queue busy"));
    return;
  }
  ui.set_status(F("Demo queued; homing first"));
}

static void redraw_more() {
  lv_clear_more();
  lv_draw_more();
}

static void event_handler(lv_obj_t *obj, lv_event_t event) {
  if (event != LV_EVENT_RELEASED) return;

  if (WITHIN(obj->mks_obj_id, ID_DEMO_0, ID_DEMO_11)) {
    const uint8_t item = obj->mks_obj_id - ID_DEMO_0;
    const DemoItem &demo = demo_items[item];
    queue_demo_command(demo_dry_run ? demo.dry_cmd : demo.live_cmd);
    return;
  }

  switch (obj->mks_obj_id) {
    case ID_DEMO_DRY:
      demo_dry_run = !demo_dry_run;
      redraw_more();
      break;
    case ID_DEMO_RETURN:
      goto_previous_ui();
      break;
  }
}

void lv_draw_more() {
  scr = lv_screen_create(MORE_UI, "Demos");

  plotter_status_create(scr);
  plotter_status_start();

  static const lv_coord_t x[4] = { 12, 126, 240, 354 };
  for (uint8_t row = 0; row < 3; ++row)
    for (uint8_t col = 0; col < 4; ++col)
      demo_button(demo_items[row * 4 + col].label, x[col], 84 + row * 58, ID_DEMO_0 + row * 4 + col, event_handler);

  char line[64];
  snprintf_P(line, sizeof(line), PSTR("Mode\n%s"), demo_dry_run ? "Dry" : "Draw");
  demo_button(line, 12, 258, ID_DEMO_DRY, event_handler, demo_dry_run ? &style_para_value : &style_android_accent);
  demo_button(common_menu.text_back, 240, 258, ID_DEMO_RETURN, event_handler);

  lv_android_home_indicator(scr);
}

void lv_clear_more() {
  plotter_status_stop();
  #if BUTTONS_EXIST(EN1, EN2, ENC)
    if (gCfgItems.encoder_enable) lv_group_remove_all_objs(g);
  #endif
  lv_obj_del(scr);
}

#endif // HAS_TFT_LVGL_UI
