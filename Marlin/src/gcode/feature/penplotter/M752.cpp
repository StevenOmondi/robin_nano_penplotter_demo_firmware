/**
 * M752 - Centered vector text for the Neptune 2S / Robin Nano pen plotter.
 *
 * Usage:
 *   M752 F0 S18 A1 W1 L120 R0 M0 "HELLO|PLOTTER"
 *   M752 N"PLOT.TXT"
 *   M752 D1
 *
 * F0 Vector, F1 Block, F2 Outline, F3 Italic
 * A0 left, A1 center, A2 right. Use | for line breaks.
 * N"file.txt" reads a paragraph from an SD card text file.
 * Long lines wrap automatically to fit the bed. Paragraphs up to 32 lines.
 * P1 dry-runs with the pen lifted. B1 frames the fitted text box.
 */

#include "../../../inc/MarlinConfig.h"

#if ENABLED(PENPLOTTER_EMBEDDED_DEMOS)

#include "../../gcode.h"
#include "../../../feature/penplotter/penplotter_settings.h"
#include "../../../sd/cardreader.h"
#include "../../../lcd/marlinui.h"
#include "../../../module/motion.h"
#include "../../../module/planner.h"
#include "../../../module/stepper.h"

namespace {
  static constexpr float DRAW_MARGIN = 10.0f;
  static constexpr float MAX_DRAW_W = X_BED_SIZE - DRAW_MARGIN * 2.0f;
  static constexpr float MAX_DRAW_H = Y_BED_SIZE - DRAW_MARGIN * 2.0f;
  static constexpr uint8_t PARAGRAPH_MAX_LINES = 32;
  static constexpr uint8_t PARAGRAPH_BUF = 512;
  static constexpr float PLOTTER_CENTER_X = X_BED_SIZE * 0.5f;
  static constexpr float PLOTTER_CENTER_Y = Y_BED_SIZE * 0.5f;
  static constexpr feedRate_t TEXT_TRAVEL_MM_S = MMM_TO_MMS(9000);
  static constexpr feedRate_t TEXT_DRAW_MM_S = MMM_TO_MMS(6000);
  static constexpr feedRate_t TEXT_Z_MM_S = MMM_TO_MMS(900);

  enum TextFont : uint8_t {
    FONT_VECTOR,
    FONT_BLOCK,
    FONT_OUTLINE,
    FONT_ITALIC,
    FONT_COUNT
  };

  struct TextOptions {
    char text[PARAGRAPH_BUF + 1];
    uint8_t font;
    uint8_t align;
    uint8_t rotation;
    uint8_t line_spacing_pct;
    float size_mm;
    float letter_spacing_mm;
    bool mirror;
    bool dry_run;
    bool frame_only;
  };

  struct TextLayout {
    float cell;
    float block_w;
    float block_h;
    uint8_t line_count;
  };

  static const char demo_text_1[] PROGMEM = "HELLO|PLOTTER";
  static const char demo_text_2[] PROGMEM = "MAKE|ART";
  static const char demo_text_3[] PROGMEM = "NEVETS|PEN PLOTTER";
  static const char demo_text_4[] PROGMEM = "CENTERED|WORDS";
  static const char demo_text_5[] PROGMEM = "ANDROID|PLOTTER";
  static const char demo_text_6[] PROGMEM = "Z AXIS|PEN LIFT";

  // 5x7 ASCII font, columns, bit 0 = top row.
  static const uint8_t font5x7[][5] PROGMEM = {
    {0x00,0x00,0x00,0x00,0x00}, // space
    {0x00,0x00,0x5F,0x00,0x00}, // !
    {0x00,0x07,0x00,0x07,0x00}, // "
    {0x14,0x7F,0x14,0x7F,0x14}, // #
    {0x24,0x2A,0x7F,0x2A,0x12}, // $
    {0x23,0x13,0x08,0x64,0x62}, // %
    {0x36,0x49,0x55,0x22,0x50}, // &
    {0x00,0x05,0x03,0x00,0x00}, // '
    {0x00,0x1C,0x22,0x41,0x00}, // (
    {0x00,0x41,0x22,0x1C,0x00}, // )
    {0x14,0x08,0x3E,0x08,0x14}, // *
    {0x08,0x08,0x3E,0x08,0x08}, // +
    {0x00,0x50,0x30,0x00,0x00}, // ,
    {0x08,0x08,0x08,0x08,0x08}, // -
    {0x00,0x60,0x60,0x00,0x00}, // .
    {0x20,0x10,0x08,0x04,0x02}, // /
    {0x3E,0x51,0x49,0x45,0x3E}, // 0
    {0x00,0x42,0x7F,0x40,0x00}, // 1
    {0x42,0x61,0x51,0x49,0x46}, // 2
    {0x21,0x41,0x45,0x4B,0x31}, // 3
    {0x18,0x14,0x12,0x7F,0x10}, // 4
    {0x27,0x45,0x45,0x45,0x39}, // 5
    {0x3C,0x4A,0x49,0x49,0x30}, // 6
    {0x01,0x71,0x09,0x05,0x03}, // 7
    {0x36,0x49,0x49,0x49,0x36}, // 8
    {0x06,0x49,0x49,0x29,0x1E}, // 9
    {0x00,0x36,0x36,0x00,0x00}, // :
    {0x00,0x56,0x36,0x00,0x00}, // ;
    {0x08,0x14,0x22,0x41,0x00}, // <
    {0x14,0x14,0x14,0x14,0x14}, // =
    {0x00,0x41,0x22,0x14,0x08}, // >
    {0x02,0x01,0x51,0x09,0x06}, // ?
    {0x32,0x49,0x79,0x41,0x3E}, // @
    {0x7E,0x11,0x11,0x11,0x7E}, // A
    {0x7F,0x49,0x49,0x49,0x36}, // B
    {0x3E,0x41,0x41,0x41,0x22}, // C
    {0x7F,0x41,0x41,0x22,0x1C}, // D
    {0x7F,0x49,0x49,0x49,0x41}, // E
    {0x7F,0x09,0x09,0x09,0x01}, // F
    {0x3E,0x41,0x49,0x49,0x7A}, // G
    {0x7F,0x08,0x08,0x08,0x7F}, // H
    {0x00,0x41,0x7F,0x41,0x00}, // I
    {0x20,0x40,0x41,0x3F,0x01}, // J
    {0x7F,0x08,0x14,0x22,0x41}, // K
    {0x7F,0x40,0x40,0x40,0x40}, // L
    {0x7F,0x02,0x0C,0x02,0x7F}, // M
    {0x7F,0x04,0x08,0x10,0x7F}, // N
    {0x3E,0x41,0x41,0x41,0x3E}, // O
    {0x7F,0x09,0x09,0x09,0x06}, // P
    {0x3E,0x41,0x51,0x21,0x5E}, // Q
    {0x7F,0x09,0x19,0x29,0x46}, // R
    {0x46,0x49,0x49,0x49,0x31}, // S
    {0x01,0x01,0x7F,0x01,0x01}, // T
    {0x3F,0x40,0x40,0x40,0x3F}, // U
    {0x1F,0x20,0x40,0x20,0x1F}, // V
    {0x3F,0x40,0x38,0x40,0x3F}, // W
    {0x63,0x14,0x08,0x14,0x63}, // X
    {0x07,0x08,0x70,0x08,0x07}, // Y
    {0x61,0x51,0x49,0x45,0x43}  // Z
  };

  static uint8_t glyph_column(char c, const uint8_t col) {
    if (WITHIN(c, 'a', 'z')) c -= 32;
    if (!WITHIN(c, ' ', 'Z')) c = '?';
    return pgm_read_byte(&font5x7[c - ' '][col]);
  }

  static float char_advance(const char c, const float cell, const float spacing) {
    return (c == ' ' ? cell * 3.0f : cell * 6.0f) + spacing;
  }

  static float line_height(const float cell, const uint8_t line_spacing_pct) {
    return cell * 7.0f * (float(line_spacing_pct) * 0.01f);
  }

  static float measure_line(const char *p, const float cell, const float spacing) {
    float w = 0.0f;
    for (; *p && *p != '\n'; ++p) w += char_advance(*p, cell, spacing);
    return w;
  }

  static uint8_t count_lines(const char *p) {
    uint8_t lines = 1;
    for (; *p; ++p) if (*p == '\n') ++lines;
    return _MIN(lines, uint8_t(PARAGRAPH_MAX_LINES));
  }

  static void measure_block(const TextOptions &opt, const float cell, TextLayout &layout) {
    layout.cell = cell;
    layout.block_w = 0.0f;
    layout.line_count = count_lines(opt.text);
    const char *line = opt.text;
    for (uint8_t i = 0; i < layout.line_count; ++i) {
      layout.block_w = _MAX(layout.block_w, measure_line(line, cell, opt.letter_spacing_mm));
      while (*line && *line != '\n') ++line;
      if (*line == '\n') ++line;
    }
    layout.block_h = cell * 7.0f + (layout.line_count - 1) * line_height(cell, opt.line_spacing_pct);
  }

  static void fit_layout(TextOptions &opt, TextLayout &layout) {
    float cell = constrain(opt.size_mm, 8.0f, 44.0f) / 7.0f;
    measure_block(opt, cell, layout);
    const bool swapped = (opt.rotation & 1);
    const float fit_w = swapped ? layout.block_h : layout.block_w;
    const float fit_h = swapped ? layout.block_w : layout.block_h;
    float scale = 1.0f;
    if (fit_w > MAX_DRAW_W) scale = _MIN(scale, MAX_DRAW_W / fit_w);
    if (fit_h > MAX_DRAW_H) scale = _MIN(scale, MAX_DRAW_H / fit_h);
    if (scale < 1.0f) {
      cell *= scale;
      opt.letter_spacing_mm *= scale;
      measure_block(opt, cell, layout);
      SERIAL_ECHOLNPGM("Text auto-shrunk to fit 200x200");
    }
  }

  static void transform_point(const TextOptions &opt, const TextLayout &layout, const float lx, const float ly, float &x, float &y) {
    float rx = lx - layout.block_w * 0.5f;
    float ry = layout.block_h * 0.5f - ly;
    if (opt.mirror) rx = -rx;

    float tx = rx, ty = ry;
    switch (opt.rotation & 3) {
      case 1: tx = -ry; ty =  rx; break;
      case 2: tx = -rx; ty = -ry; break;
      case 3: tx =  ry; ty = -rx; break;
    }

    x = constrain(PLOTTER_CENTER_X + tx, 0.0f, float(X_BED_SIZE));
    y = constrain(PLOTTER_CENTER_Y + ty, 0.0f, float(Y_BED_SIZE));
  }

  static bool pen_z(const float z) {
    current_position.z = constrain(z, 0.0f, float(Z_MAX_POS));
    line_to_current_position(MMS_SCALED(TEXT_Z_MM_S));
    return penplotter_progress_step();
  }

  static bool move_local(const TextOptions &opt, const TextLayout &layout, const float lx, const float ly, const feedRate_t rate) {
    float x, y;
    transform_point(opt, layout, lx, ly, x, y);
    current_position.x = x;
    current_position.y = y;
    line_to_current_position(MMS_SCALED(rate));
    return penplotter_progress_step();
  }

  static bool stroke(const TextOptions &opt, const TextLayout &layout, const float x1, const float y1, const float x2, const float y2) {
    if (!pen_z(penplotter_settings.pen_up_z)) return false;
    if (!move_local(opt, layout, x1, y1, TEXT_TRAVEL_MM_S)) return false;
    if (!opt.dry_run && !pen_z(penplotter_settings.pen_down_z)) return false;
    if (!move_local(opt, layout, x2, y2, TEXT_DRAW_MM_S)) return false;
    return pen_z(penplotter_settings.pen_up_z);
  }

  static bool stroke_weighted(const TextOptions &opt, const TextLayout &layout, const float x1, const float y1, const float x2, const float y2, const float offset) {
    if (!stroke(opt, layout, x1, y1, x2, y2)) return false;
    if (offset <= 0.0f) return true;
    return stroke(opt, layout, x1, y1 + offset, x2, y2 + offset);
  }

  static bool outline_cell(const TextOptions &opt, const TextLayout &layout, const float x, const float y, const float cell) {
    const float x2 = x + cell, y2 = y + cell;
    if (!pen_z(penplotter_settings.pen_up_z)) return false;
    if (!move_local(opt, layout, x, y, TEXT_TRAVEL_MM_S)) return false;
    if (!opt.dry_run && !pen_z(penplotter_settings.pen_down_z)) return false;
    if (!move_local(opt, layout, x2, y, TEXT_DRAW_MM_S)) return false;
    if (!move_local(opt, layout, x2, y2, TEXT_DRAW_MM_S)) return false;
    if (!move_local(opt, layout, x, y2, TEXT_DRAW_MM_S)) return false;
    if (!move_local(opt, layout, x, y, TEXT_DRAW_MM_S)) return false;
    return pen_z(penplotter_settings.pen_up_z);
  }

  static float italic_shift(const uint8_t font, const uint8_t row, const float cell) {
    return font == FONT_ITALIC ? (6 - row) * cell * 0.24f : 0.0f;
  }

  static bool draw_glyph(const TextOptions &opt, const TextLayout &layout, const char ch, const float x, const float y) {
    if (ch == ' ') return true;

    const float cell = layout.cell;
    if (opt.font == FONT_OUTLINE) {
      for (uint8_t col = 0; col < 5; ++col) {
        const uint8_t bits = glyph_column(ch, col);
        for (uint8_t row = 0; row < 7; ++row)
          if (TEST(bits, row) && !outline_cell(opt, layout, x + col * cell, y + row * cell, cell * 0.82f))
            return false;
      }
      return true;
    }

    const float weight = opt.font == FONT_BLOCK ? cell * 0.16f : 0.0f;
    for (uint8_t row = 0; row < 7; ++row) {
      int8_t run_start = -1;
      for (uint8_t col = 0; col <= 5; ++col) {
        const bool on = col < 5 && TEST(glyph_column(ch, col), row);
        if (on && run_start < 0)
          run_start = col;
        else if (!on && run_start >= 0) {
          const float slant = italic_shift(opt.font, row, cell);
          const float yy = y + row * cell + cell * 0.5f;
          if (!stroke_weighted(opt, layout, x + run_start * cell + slant, yy, x + col * cell + slant, yy, weight))
            return false;
          run_start = -1;
        }
      }
    }
    return true;
  }

  static uint16_t estimate_strokes(const TextOptions &opt) {
    uint16_t strokes = opt.frame_only ? 6 : 10;
    for (const char *p = opt.text; *p; ++p) {
      if (*p == ' ' || *p == '\n') continue;
      strokes += opt.font == FONT_OUTLINE ? 36 : 14;
    }
    return _MAX(uint16_t(20), strokes);
  }

  static bool draw_frame(const TextOptions &opt, const TextLayout &layout) {
    const float pad = layout.cell;
    if (!stroke(opt, layout, -pad, -pad, layout.block_w + pad, -pad)) return false;
    if (!stroke(opt, layout, layout.block_w + pad, -pad, layout.block_w + pad, layout.block_h + pad)) return false;
    if (!stroke(opt, layout, layout.block_w + pad, layout.block_h + pad, -pad, layout.block_h + pad)) return false;
    return stroke(opt, layout, -pad, layout.block_h + pad, -pad, -pad);
  }

  static bool draw_text_block(const TextOptions &opt, const TextLayout &layout) {
    const char *line = opt.text;
    float y = 0.0f;
    for (uint8_t li = 0; li < layout.line_count; ++li) {
      const float line_w = measure_line(line, layout.cell, opt.letter_spacing_mm);
      float x = 0.0f;
      if (opt.align == 1) x = (layout.block_w - line_w) * 0.5f;
      else if (opt.align >= 2) x = layout.block_w - line_w;

      while (*line && *line != '\n') {
        const char ch = *line++;
        if (!draw_glyph(opt, layout, ch, x, y)) return false;
        x += char_advance(ch, layout.cell, opt.letter_spacing_mm);
      }
      if (*line == '\n') ++line;
      y += line_height(layout.cell, opt.line_spacing_pct);
    }
    return true;
  }

  static void copy_demo_text(char *dst, const uint8_t demo) {
    PGM_P src = demo_text_1;
    switch (demo) {
      case 2: src = demo_text_2; break;
      case 3: src = demo_text_3; break;
      case 4: src = demo_text_4; break;
      case 5: src = demo_text_5; break;
      case 6: src = demo_text_6; break;
    }
    strcpy_P(dst, src);
    for (char *p = dst; *p; ++p) if (*p == '|') *p = '\n';
  }

  static bool text_from_parser(char *dst, const size_t size) {
    if (!parser.string_arg || !parser.string_arg[0]) return false;
    uint8_t out = 0, lines = 1;
    for (char *p = parser.string_arg; *p && out < size - 1; ++p) {
      char c = *p;
      if (c == '|' || c == '\r' || c == '\n') {
        if (lines < PARAGRAPH_MAX_LINES && out && dst[out - 1] != '\n') {
          dst[out++] = '\n';
          ++lines;
        }
        continue;
      }
      if (c == '"' || c == '\\') c = '\'';
      if (WITHIN(c, 'a', 'z')) c -= 32;
      if (WITHIN(c, ' ', 'Z')) dst[out++] = c;
    }
    while (out && dst[out - 1] == '\n') --out;
    dst[out] = '\0';
    return out > 0;
  }

  // Read a paragraph from an SD card text file, e.g. M752 N"PLOT.TXT".
  static bool text_from_file(char *dst, const size_t size) {
    if (!parser.seenval('N') || !parser.string_arg[0]) return false;
    card.openFileRead(parser.string_arg);
    if (!card.isFileOpen()) {
      SERIAL_ERROR_MSG("M752: SD file not found: ", parser.string_arg);
      return false;
    }
    uint8_t out = 0, lines = 1;
    while (out < size - 1) {
      const int c = card.get();
      if (c < 0) break;
      const char ch = (char)c;
      if (ch == '\r' || ch == '\n') {
        if (lines < PARAGRAPH_MAX_LINES && out && dst[out - 1] != '\n') {
          dst[out++] = '\n';
          ++lines;
        }
        continue;
      }
      if (ch == '"' || ch == '\\') dst[out++] = '\'';
      else if (WITHIN(ch, 'a', 'z')) dst[out++] = ch - 32;
      else if (WITHIN(ch, ' ', 'Z')) dst[out++] = ch;
    }
    card.closefile();
    while (out && dst[out - 1] == '\n') --out;
    dst[out] = '\0';
    return out > 0;
  }

  // Wrap lines so none exceed the drawable width, breaking at word boundaries.
  static void wrap_paragraph(TextOptions &opt) {
    char buf[PARAGRAPH_BUF + 32];
    const float cell = constrain(opt.size_mm, 8.0f, 44.0f) / 7.0f;
    const bool swapped = (opt.rotation & 1);
    const float max_w = swapped ? MAX_DRAW_H : MAX_DRAW_W;
    uint8_t lines = 1, out = 0, last_space = 0;
    float w = 0.0f;

    for (const char *p = opt.text; *p; ++p) {
      const char c = *p;
      if (c == '\n') {
        buf[out++] = '\n';
        if (++lines > PARAGRAPH_MAX_LINES) break;
        w = 0.0f; last_space = 0;
        continue;
      }
      const float cw = char_advance(c, cell, opt.letter_spacing_mm);
      if (w + cw > max_w) {
        if (last_space) {
          buf[last_space - 1] = '\n';   // last space becomes the line break
          out = last_space;             // drop trailing space
          if (++lines > PARAGRAPH_MAX_LINES) break;
          w = 0.0f; last_space = 0;
        }
        else if (++lines > PARAGRAPH_MAX_LINES)
          break;
        else {
          buf[out++] = '\n';
          w = 0.0f; last_space = 0;
        }
      }
      buf[out++] = c;
      w += cw;
      if (c == ' ') last_space = out;
    }
    buf[out] = '\0';
    strcpy(opt.text, buf);
  }

  static void report_text_usage() {
    SERIAL_ECHOLNPGM("M752 centered text writer:");
    SERIAL_ECHOLNPGM("  M752 F0 S18 A1 W1 L120 R0 M0 \"HELLO|PLOTTER\"");
    SERIAL_ECHOLNPGM("  M752 N\"PLOT.TXT\"  reads a paragraph from an SD card text file");
    SERIAL_ECHOLNPGM("  F0 Vector, F1 Block, F2 Outline, F3 Italic");
    SERIAL_ECHOLNPGM("  A0 left, A1 center, A2 right; W letter spacing; L line spacing percent");
    SERIAL_ECHOLNPGM("  R0..3 rotate quarter turns; M1 mirror; P1 dry run; B1 frame only");
    SERIAL_ECHOLNPGM("  D1..D6 built-in text demos. Text wraps and auto-fits inside 200x200.");
  }
}

void GcodeSuite::M752() {
  TextOptions opt;
  memset(&opt, 0, sizeof(opt));
  opt.font = parser.byteval('F', FONT_VECTOR);
  opt.align = parser.byteval('A', 1);
  opt.rotation = parser.byteval('R', 0) & 3;
  opt.line_spacing_pct = constrain(parser.byteval('L', 120), 80, 200);
  opt.size_mm = parser.floatval('S', 18.0f);
  opt.letter_spacing_mm = constrain(parser.floatval('W', 0.0f), 0.0f, 4.0f);
  opt.mirror = parser.boolval('M');
  opt.dry_run = parser.boolval('P');
  opt.frame_only = parser.boolval('B');

  if (parser.seenval('D')) {
    const uint8_t demo = parser.byteval('D');
    if (!WITHIN(demo, 1, 6)) {
      SERIAL_ERROR_MSG("M752 D", demo, " out of range. Use D1..D6.");
      return;
    }
    copy_demo_text(opt.text, demo);
    if (!parser.seenval('F')) opt.font = (demo - 1) % FONT_COUNT;
    if (!parser.seenval('S')) opt.size_mm = demo == 3 ? 22.0f : 20.0f;
  }
  else if (parser.seenval('N')) {
    if (!text_from_file(opt.text, sizeof(opt.text))) return;
  }
  else if (!text_from_parser(opt.text, sizeof(opt.text))) {
    report_text_usage();
    return;
  }

  wrap_paragraph(opt);

  opt.font = _MIN(opt.font, uint8_t(FONT_COUNT - 1));
  opt.align = _MIN(opt.align, uint8_t(2));
  penplotter_settings_sanitize();

  char up[12], preflight[96];
  dtostrf(penplotter_settings.pen_up_z, 1, 2, up);
  snprintf_P(preflight, sizeof(preflight), PSTR("M17\nM211 S1\nG90\nG1 Z%s F900\nG28 X Y"), up);
  process_subcommands_now(preflight);
  if (!MOTION_CONDITIONS) return;

  TextLayout layout;
  fit_layout(opt, layout);
  penplotter_begin_job(estimate_strokes(opt));

  stepper.enable_all_steppers();
  planner.synchronize();
  bool ok = pen_z(penplotter_settings.pen_up_z);

  if (ok && (opt.frame_only || opt.dry_run)) ok = draw_frame(opt, layout);
  if (ok && !opt.frame_only) ok = draw_text_block(opt, layout);

  pen_z(penplotter_settings.pen_up_z);
  current_position.x = PLOTTER_CENTER_X;
  current_position.y = PLOTTER_CENTER_Y;
  line_to_current_position(MMS_SCALED(TEXT_TRAVEL_MM_S));
  planner.synchronize();
  penplotter_end_job();

  if (ok) {
    SERIAL_ECHO_MSG("Text complete: ", opt.text);
    ui.set_status(opt.dry_run ? F("Text dry-run done") : F("Text done"));
  }
  else {
    ui.set_status(F("Text stopped"));
  }
}

#endif // PENPLOTTER_EMBEDDED_DEMOS
