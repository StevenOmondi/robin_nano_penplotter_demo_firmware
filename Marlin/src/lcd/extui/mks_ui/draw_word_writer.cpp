/**
 * Touch text writer for the pen plotter.
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

static char plotter_text[49] = "HELLO PLOTTER";
static uint8_t plotter_font_index = 0;
static uint8_t plotter_size_index = 1;
static uint8_t plotter_align_index = 1;

static const char * const font_names[] = { "Vector", "Block", "Outline", "Italic" };
static const char * const align_names[] = { "Left", "Center", "Right" };
static constexpr uint8_t size_values[] = { 12, 18, 24, 32, 40 };

enum {
  ID_WORD_EDIT = 1,
  ID_WORD_FONT,
  ID_WORD_SIZE,
  ID_WORD_ALIGN,
  ID_WORD_FRAME,
  ID_WORD_DRY,
  ID_WORD_WRITE,
  ID_WORD_PARA,
  ID_WORD_RETURN
};

void plotter_text_set(const char *text) {
  uint8_t out = 0, lines = 1;
  if (text) {
    for (uint8_t i = 0; text[i] && out < sizeof(plotter_text) - 1; ++i) {
      char c = text[i];
      if (c == '\r' || c == '\n' || c == '|') {
        if (lines < 4 && out && plotter_text[out - 1] != '\n') {
          plotter_text[out++] = '\n';
          ++lines;
        }
        continue;
      }
      if (c == '"' || c == '\\') c = '\'';
      if (WITHIN(c, 'a', 'z')) c -= 32;
      if (WITHIN(c, ' ', 'Z'))
        plotter_text[out++] = c;
    }
  }
  while (out && plotter_text[out - 1] == '\n') --out;
  plotter_text[out] = '\0';
  if (!out) strcpy_P(plotter_text, PSTR("HELLO PLOTTER"));
}

const char *plotter_text_get() {
  return plotter_text;
}

static lv_obj_t *text_button(const char *text, const lv_coord_t x, const lv_coord_t y, const int id, lv_event_cb_t cb, const lv_coord_t w = 106, lv_style_t *style = &style_para_value) {
  lv_obj_t *btn = lv_btn_create(scr, x, y, w, 50, cb, id, style);
  lv_obj_t *label = lv_label_create_empty(btn);
  lv_label_set_text(label, text);
  lv_obj_align(label, btn, LV_ALIGN_CENTER, 0, 0);
  if (TERN0(HAS_ROTARY_ENCODER, gCfgItems.encoder_enable))
    lv_group_add_obj(g, btn);
  return btn;
}

static void encoded_text(char *out, const size_t out_size) {
  uint8_t j = 0;
  for (uint8_t i = 0; plotter_text[i] && j < out_size - 1; ++i) {
    char c = plotter_text[i];
    if (c == '\n') c = '|';
    if (c == '"' || c == '\\') c = '\'';
    out[j++] = c;
  }
  out[j] = '\0';
}

static void queue_text_write(const bool dry_run, const bool frame_only) {
  char text_arg[sizeof(plotter_text)], cmd[MAX_CMD_SIZE];
  encoded_text(text_arg, sizeof(text_arg));
  snprintf_P(cmd, sizeof(cmd), PSTR("M752 F%u S%u A%u%s%s \"%s\""),
    plotter_font_index,
    size_values[plotter_size_index],
    plotter_align_index,
    dry_run ? " P1" : "",
    frame_only ? " B1" : "",
    text_arg
  );
  if (!queue.enqueue_one(cmd)) {
    ui.set_status(F("Queue busy"));
    return;
  }
  ui.set_status(F("Text queued; homing first"));
}

static void redraw_word_writer() {
  lv_clear_word_writer();
  lv_draw_word_writer();
}

static uint8_t text_line_count() {
  uint8_t lines = 1;
  for (const char *p = plotter_text; *p; ++p) if (*p == '\n') ++lines;
  return lines;
}

static uint8_t longest_line_chars() {
  uint8_t longest = 0, current = 0;
  for (const char *p = plotter_text; ; ++p) {
    if (*p && *p != '\n') ++current;
    else {
      longest = _MAX(longest, current);
      current = 0;
      if (!*p) break;
    }
  }
  return longest;
}

static void queue_paragraph_write() {
  char cmd[MAX_CMD_SIZE];
  snprintf_P(cmd, sizeof(cmd), PSTR("M752 F%u S%u A%u N\"PLOT.TXT\""),
    plotter_font_index,
    size_values[plotter_size_index],
    plotter_align_index
  );
  if (!queue.enqueue_one(cmd)) {
    ui.set_status(F("Queue busy"));
    return;
  }
  ui.set_status(F("Paragraph queued; homing first"));
}

static void event_handler(lv_obj_t *obj, lv_event_t event) {
  if (event != LV_EVENT_RELEASED) return;

  switch (obj->mks_obj_id) {
    case ID_WORD_EDIT:
      keyboard_value = plotterTextInput;
      lv_clear_word_writer();
      lv_draw_keyboard();
      break;

    case ID_WORD_FONT:
      plotter_font_index = (plotter_font_index + 1) % COUNT(font_names);
      redraw_word_writer();
      break;

    case ID_WORD_SIZE:
      plotter_size_index = (plotter_size_index + 1) % COUNT(size_values);
      redraw_word_writer();
      break;

    case ID_WORD_ALIGN:
      plotter_align_index = (plotter_align_index + 1) % COUNT(align_names);
      redraw_word_writer();
      break;

    case ID_WORD_FRAME:
      queue_text_write(true, true);
      break;

    case ID_WORD_DRY:
      queue_text_write(true, false);
      break;

    case ID_WORD_WRITE:
      queue_text_write(false, false);
      break;

    case ID_WORD_PARA:
      queue_paragraph_write();
      break;

    case ID_WORD_RETURN:
      goto_previous_ui();
      break;
  }
}

void lv_draw_word_writer() {
  scr = lv_screen_create(WORD_WRITER_UI, "Words");

  lv_obj_t *panel = lv_obj_create(scr, nullptr);
  lv_obj_set_style(panel, &style_android_panel);
  lv_obj_set_pos(panel, 12, 42);
  lv_obj_set_size(panel, 456, 54);

  lv_obj_t *penIcon = lv_img_create(panel, nullptr);
  lv_img_set_src(penIcon, "F:/bmp_plot_pen.bin");
  lv_obj_set_pos(penIcon, 18, 9);

  char line[96], preview[49];
  strncpy(preview, plotter_text, sizeof(preview) - 1);
  preview[sizeof(preview) - 1] = '\0';
  for (char *p = preview; *p; ++p) if (*p == '\n') *p = '/';

  snprintf_P(line, sizeof(line), PSTR("%.44s"), preview);
  lv_obj_t *textLabel = lv_label_create(panel, 62, 6, line);
  lv_obj_set_style(textLabel, &tft_style_label_rel);

  const uint8_t lines = text_line_count();
  const uint8_t chars = longest_line_chars();
  const uint16_t est_w = _MIN(uint16_t(180), uint16_t(chars * size_values[plotter_size_index] * 6 / 7));
  const uint16_t est_h = _MIN(uint16_t(180), uint16_t(lines * size_values[plotter_size_index] * 120 / 100));
  snprintf_P(line, sizeof(line), PSTR("%s %umm  %s  Est %ux%umm"),
    font_names[plotter_font_index], size_values[plotter_size_index], align_names[plotter_align_index], est_w, est_h);
  lv_obj_t *metaLabel = lv_label_create(panel, 62, 30, line);
  lv_obj_set_style(metaLabel, &style_android_muted);

  static const lv_coord_t x[4] = { 12, 126, 240, 354 };
  text_button("Edit", x[0], 116, ID_WORD_EDIT, event_handler);
  snprintf_P(line, sizeof(line), PSTR("Font\n%s"), font_names[plotter_font_index]);
  text_button(line, x[1], 116, ID_WORD_FONT, event_handler);
  snprintf_P(line, sizeof(line), PSTR("Size\n%u"), size_values[plotter_size_index]);
  text_button(line, x[2], 116, ID_WORD_SIZE, event_handler);
  snprintf_P(line, sizeof(line), PSTR("Align\n%s"), align_names[plotter_align_index]);
  text_button(line, x[3], 116, ID_WORD_ALIGN, event_handler);

  static const lv_coord_t x5[5] = { 12, 100, 188, 276, 364 };
  text_button("Frame", x5[0], 184, ID_WORD_FRAME, event_handler, 86);
  text_button("Dry Run", x5[1], 184, ID_WORD_DRY, event_handler, 86);
  text_button("Write", x5[2], 184, ID_WORD_WRITE, event_handler, 86, &style_android_accent);
  text_button("Para SD", x5[3], 184, ID_WORD_PARA, event_handler, 86);
  text_button(common_menu.text_back, x5[4], 184, ID_WORD_RETURN, event_handler, 86);

  lv_android_home_indicator(scr);
}

void lv_clear_word_writer() {
  #if HAS_ROTARY_ENCODER
    if (gCfgItems.encoder_enable) lv_group_remove_all_objs(g);
  #endif
  lv_obj_del(scr);
}

#endif // HAS_TFT_LVGL_UI
