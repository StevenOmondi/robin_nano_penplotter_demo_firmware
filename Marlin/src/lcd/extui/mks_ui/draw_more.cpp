/**
 * Plotter demo gallery.
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
  { "Hello",   "M752 D1",             "M752 D1 P1" },
  { "Words",   "M752 D3",             "M752 D3 P1" },
  { "Square",  "M753 A0 S120 D3",     "M753 A0 S120 D3 P1" },
  { "Circle",  "M753 A1 S130 D4",     "M753 A1 S130 D4 P1" },
  { "Star",    "M753 A3 S145 D5",     "M753 A3 S145 D5 P1" },
  { "Spiral",  "M753 A2 S155 D5",     "M753 A2 S155 D5 P1" },
  { "Mandala", "M753 A7 S165 D5",     "M753 A7 S165 D5 P1" },
  { "House",   "M750 D4",             "M751 R" }
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
  ID_DEMO_DRY,
  ID_DEMO_RETURN
};

static lv_obj_t *demo_button(const char *text, const lv_coord_t x, const lv_coord_t y, const int id, lv_event_cb_t cb) {
  lv_obj_t *btn = lv_btn_create(scr, x, y, 106, 50, cb, id, &style_para_value);
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

  if (WITHIN(obj->mks_obj_id, ID_DEMO_0, ID_DEMO_7)) {
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

  lv_obj_t *panel = lv_obj_create(scr, nullptr);
  lv_obj_set_style(panel, &style_android_panel);
  lv_obj_set_pos(panel, 12, 44);
  lv_obj_set_size(panel, 456, 34);

  char line[64];
  snprintf_P(line, sizeof(line), PSTR("Demo gallery   %s"), demo_dry_run ? "Dry run" : "Draw");
  lv_obj_t *label = lv_label_create(panel, 12, 8, line);
  lv_obj_set_style(label, &tft_style_label_rel);

  static const lv_coord_t x[4] = { 12, 126, 240, 354 };
  for (uint8_t i = 0; i < 4; ++i)
    demo_button(demo_items[i].label, x[i], 92, ID_DEMO_0 + i, event_handler);
  for (uint8_t i = 0; i < 4; ++i)
    demo_button(demo_items[i + 4].label, x[i], 156, ID_DEMO_4 + i, event_handler);

  snprintf_P(line, sizeof(line), PSTR("Mode\n%s"), demo_dry_run ? "Dry" : "Draw");
  demo_button(line, x[0], 220, ID_DEMO_DRY, event_handler);
  demo_button(common_menu.text_back, x[3], 220, ID_DEMO_RETURN, event_handler);

  lv_android_home_indicator(scr);
}

void lv_clear_more() {
  #if BUTTONS_EXIST(EN1, EN2, ENC)
    if (gCfgItems.encoder_enable) lv_group_remove_all_objs(g);
  #endif
  lv_obj_del(scr);
}

#endif // HAS_TFT_LVGL_UI
