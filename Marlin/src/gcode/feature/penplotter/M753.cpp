/**
 * M753 - Centered generated art for the Neptune 2S / Robin Nano pen plotter.
 *
 * Usage:
 *   M753 A0 S120 D3 P0
 *
 * A0 square, A1 circle, A2 spiral, A3 star, A4 grid,
 * A5 hatch, A6 waves, A7 mandala, A8 spirograph.
 * S<size-mm>, D<density 1..10>, P1 dry-run with pen lifted.
 */

#include "../../../inc/MarlinConfig.h"

#if ENABLED(PENPLOTTER_EMBEDDED_DEMOS)

#include <math.h>

#include "../../gcode.h"
#include "../../../feature/penplotter/penplotter_settings.h"
#include "../../../lcd/marlinui.h"
#include "../../../module/motion.h"
#include "../../../module/planner.h"
#include "../../../module/stepper.h"

namespace {
  static constexpr float PLOTTER_CENTER_X = X_BED_SIZE * 0.5f;
  static constexpr float PLOTTER_CENTER_Y = Y_BED_SIZE * 0.5f;
  static constexpr float DRAW_MARGIN = 10.0f;
  static constexpr float PLOTTER_PI = 3.14159265f;
  static constexpr float PLOTTER_TAU = PLOTTER_PI * 2.0f;
  static constexpr feedRate_t ART_TRAVEL_MM_S = MMM_TO_MMS(9000);
  static constexpr feedRate_t ART_DRAW_MM_S = MMM_TO_MMS(6000);
  static constexpr feedRate_t ART_Z_MM_S = MMM_TO_MMS(900);

  struct ArtOptions {
    uint8_t art;
    uint8_t density;
    float size_mm;
    bool dry_run;
    bool frame;
  };

  static float cx(const float x) { return constrain(x, 0.0f, float(X_BED_SIZE)); }
  static float cy(const float y) { return constrain(y, 0.0f, float(Y_BED_SIZE)); }

  static bool pen_z(const float z) {
    current_position.z = constrain(z, 0.0f, float(Z_MAX_POS));
    line_to_current_position(MMS_SCALED(ART_Z_MM_S));
    return penplotter_progress_step();
  }

  static bool travel_xy(const float x, const float y) {
    if (!pen_z(penplotter_settings.pen_up_z)) return false;
    current_position.x = cx(x);
    current_position.y = cy(y);
    line_to_current_position(MMS_SCALED(ART_TRAVEL_MM_S));
    return penplotter_progress_step();
  }

  static bool draw_xy(const float x, const float y) {
    current_position.x = cx(x);
    current_position.y = cy(y);
    line_to_current_position(MMS_SCALED(ART_DRAW_MM_S));
    return penplotter_progress_step();
  }

  static bool begin_path(const ArtOptions &opt, const float x, const float y) {
    if (!travel_xy(x, y)) return false;
    if (!opt.dry_run && !pen_z(penplotter_settings.pen_down_z)) return false;
    return true;
  }

  static bool end_path() {
    return pen_z(penplotter_settings.pen_up_z);
  }

  static bool stroke(const ArtOptions &opt, const float x1, const float y1, const float x2, const float y2) {
    if (!begin_path(opt, x1, y1)) return false;
    if (!draw_xy(x2, y2)) return false;
    return end_path();
  }

  static bool draw_square(const ArtOptions &opt, const float size) {
    const float h = size * 0.5f;
    if (!begin_path(opt, PLOTTER_CENTER_X - h, PLOTTER_CENTER_Y - h)) return false;
    if (!draw_xy(PLOTTER_CENTER_X + h, PLOTTER_CENTER_Y - h)) return false;
    if (!draw_xy(PLOTTER_CENTER_X + h, PLOTTER_CENTER_Y + h)) return false;
    if (!draw_xy(PLOTTER_CENTER_X - h, PLOTTER_CENTER_Y + h)) return false;
    if (!draw_xy(PLOTTER_CENTER_X - h, PLOTTER_CENTER_Y - h)) return false;
    return end_path();
  }

  static bool draw_circle(const ArtOptions &opt, const float radius, const uint16_t segments) {
    if (!begin_path(opt, PLOTTER_CENTER_X + radius, PLOTTER_CENTER_Y)) return false;
    for (uint16_t i = 1; i <= segments; ++i) {
      const float a = PLOTTER_TAU * i / segments;
      if (!draw_xy(PLOTTER_CENTER_X + cos(a) * radius, PLOTTER_CENTER_Y + sin(a) * radius)) return false;
    }
    return end_path();
  }

  static bool draw_spiral(const ArtOptions &opt, const float radius) {
    const uint16_t segments = 60 + opt.density * 36;
    if (!begin_path(opt, PLOTTER_CENTER_X, PLOTTER_CENTER_Y)) return false;
    for (uint16_t i = 1; i <= segments; ++i) {
      const float t = float(i) / segments;
      const float a = PLOTTER_TAU * (2.0f + opt.density * 0.85f) * t;
      const float r = radius * t;
      if (!draw_xy(PLOTTER_CENTER_X + cos(a) * r, PLOTTER_CENTER_Y + sin(a) * r)) return false;
    }
    return end_path();
  }

  static bool draw_star(const ArtOptions &opt, const float radius) {
    const uint8_t points = 5 + (opt.density > 5 ? 1 : 0);
    const uint8_t vertices = points * 2;
    if (!begin_path(opt, PLOTTER_CENTER_X, PLOTTER_CENTER_Y + radius)) return false;
    for (uint8_t i = 1; i <= vertices; ++i) {
      const float r = (i & 1) ? radius * 0.42f : radius;
      const float a = PLOTTER_PI * 0.5f + PLOTTER_TAU * i / vertices;
      if (!draw_xy(PLOTTER_CENTER_X + cos(a) * r, PLOTTER_CENTER_Y + sin(a) * r)) return false;
    }
    return end_path();
  }

  static bool draw_grid(const ArtOptions &opt, const float size) {
    const float h = size * 0.5f;
    const uint8_t lines = constrain(opt.density + 3, 4, 13);
    for (uint8_t i = 0; i < lines; ++i) {
      const float p = -h + size * i / (lines - 1);
      if (!stroke(opt, PLOTTER_CENTER_X + p, PLOTTER_CENTER_Y - h, PLOTTER_CENTER_X + p, PLOTTER_CENTER_Y + h)) return false;
      if (!stroke(opt, PLOTTER_CENTER_X - h, PLOTTER_CENTER_Y + p, PLOTTER_CENTER_X + h, PLOTTER_CENTER_Y + p)) return false;
    }
    return true;
  }

  static bool draw_hatch(const ArtOptions &opt, const float size) {
    const float h = size * 0.5f;
    const uint8_t lines = constrain(opt.density * 3, 3, 30);
    for (uint8_t i = 0; i < lines; ++i) {
      const float t = -h + size * i / (lines - 1);
      if (!stroke(opt, PLOTTER_CENTER_X - h, PLOTTER_CENTER_Y + t, PLOTTER_CENTER_X + t, PLOTTER_CENTER_Y - h)) return false;
      if (!stroke(opt, PLOTTER_CENTER_X + h, PLOTTER_CENTER_Y - t, PLOTTER_CENTER_X - t, PLOTTER_CENTER_Y + h)) return false;
    }
    return true;
  }

  static bool draw_waves(const ArtOptions &opt, const float size) {
    const float h = size * 0.5f;
    const uint8_t rows = constrain(opt.density + 2, 3, 12);
    const uint8_t segments = 24 + opt.density * 4;
    for (uint8_t row = 0; row < rows; ++row) {
      const float y0 = PLOTTER_CENTER_Y - h + size * row / (rows - 1);
      if (!begin_path(opt, PLOTTER_CENTER_X - h, y0)) return false;
      for (uint8_t i = 1; i <= segments; ++i) {
        const float t = float(i) / segments;
        const float x = PLOTTER_CENTER_X - h + size * t;
        const float y = y0 + sin(t * PLOTTER_TAU * (1.0f + opt.density * 0.25f)) * size * 0.035f;
        if (!draw_xy(x, y)) return false;
      }
      if (!end_path()) return false;
    }
    return true;
  }

  static bool draw_mandala(const ArtOptions &opt, const float radius) {
    const uint8_t petals = constrain(opt.density + 5, 6, 15);
    const uint8_t segments = 24;
    for (uint8_t p = 0; p < petals; ++p) {
      const float base = PLOTTER_TAU * p / petals;
      const float x0 = PLOTTER_CENTER_X + cos(base) * radius * 0.22f;
      const float y0 = PLOTTER_CENTER_Y + sin(base) * radius * 0.22f;
      if (!begin_path(opt, x0, y0)) return false;
      for (uint8_t i = 1; i <= segments; ++i) {
        const float t = float(i) / segments;
        const float a = base + sin(t * PLOTTER_PI) * 0.62f;
        const float r = radius * (0.22f + sin(t * PLOTTER_PI) * 0.70f);
        if (!draw_xy(PLOTTER_CENTER_X + cos(a) * r, PLOTTER_CENTER_Y + sin(a) * r)) return false;
      }
      if (!end_path()) return false;
    }
    if (!draw_circle(opt, radius * 0.28f, 36)) return false;
    return draw_circle(opt, radius * 0.58f, 54);
  }

  static bool draw_spirograph(const ArtOptions &opt, const float radius) {
    const uint16_t segments = 120 + opt.density * 36;
    const float big_r = radius * 0.58f;
    const float small_r = radius * (0.18f + opt.density * 0.01f);
    const float pen_r = radius * 0.62f;
    const float start_x = PLOTTER_CENTER_X + (big_r - small_r) + pen_r;
    if (!begin_path(opt, start_x, PLOTTER_CENTER_Y)) return false;
    for (uint16_t i = 1; i <= segments; ++i) {
      const float t = PLOTTER_TAU * 4.0f * i / segments;
      const float ratio = (big_r - small_r) / small_r;
      const float x = PLOTTER_CENTER_X + (big_r - small_r) * cos(t) + pen_r * cos(ratio * t);
      const float y = PLOTTER_CENTER_Y + (big_r - small_r) * sin(t) - pen_r * sin(ratio * t);
      if (!draw_xy(x, y)) return false;
    }
    return end_path();
  }

  static uint16_t estimate_steps(const ArtOptions &opt) {
    switch (opt.art) {
      case 0: return 24;
      case 1: return 48 + opt.density * 12;
      case 2: return 80 + opt.density * 40;
      case 3: return 36;
      case 4: return 40 + opt.density * 12;
      case 5: return 50 + opt.density * 18;
      case 6: return 70 + opt.density * 28;
      case 7: return 120 + opt.density * 40;
      default: return 160 + opt.density * 40;
    }
  }

  static void report_art_usage() {
    SERIAL_ECHOLNPGM("M753 centered art generator:");
    SERIAL_ECHOLNPGM("  M753 A0 S120 D3 P0");
    SERIAL_ECHOLNPGM("  A0 square, A1 circle, A2 spiral, A3 star, A4 grid");
    SERIAL_ECHOLNPGM("  A5 hatch, A6 waves, A7 mandala, A8 spirograph");
    SERIAL_ECHOLNPGM("  S<size-mm> auto-limited to 180, D density 1..10, P1 dry-run, B1 bounding frame");
  }
}

void GcodeSuite::M753() {
  if (!parser.seenval('A') && !parser.seenval('S') && !parser.seenval('D') && !parser.seen('P') && !parser.seen('B')) {
    report_art_usage();
    return;
  }

  ArtOptions opt;
  opt.art = _MIN(parser.byteval('A', 0), uint8_t(8));
  opt.density = constrain(parser.byteval('D', 3), 1, 10);
  opt.size_mm = constrain(parser.floatval('S', 120.0f), 20.0f, _MIN(float(X_BED_SIZE), float(Y_BED_SIZE)) - DRAW_MARGIN * 2.0f);
  opt.dry_run = parser.boolval('P');
  opt.frame = parser.boolval('B');

  penplotter_settings_sanitize();
  char up[12], preflight[96];
  dtostrf(penplotter_settings.pen_up_z, 1, 2, up);
  snprintf_P(preflight, sizeof(preflight), PSTR("M17\nM211 S1\nG90\nG1 Z%s F900\nG28 X Y"), up);
  process_subcommands_now(preflight);
  if (!MOTION_CONDITIONS) return;

  penplotter_begin_job(estimate_steps(opt));
  stepper.enable_all_steppers();
  planner.synchronize();

  bool ok = pen_z(penplotter_settings.pen_up_z);
  if (ok && opt.frame) ok = draw_square(opt, opt.size_mm);

  const float radius = opt.size_mm * 0.5f;
  if (ok) {
    switch (opt.art) {
      case 0: ok = draw_square(opt, opt.size_mm); break;
      case 1: ok = draw_circle(opt, radius, 48 + opt.density * 12); break;
      case 2: ok = draw_spiral(opt, radius); break;
      case 3: ok = draw_star(opt, radius); break;
      case 4: ok = draw_grid(opt, opt.size_mm); break;
      case 5: ok = draw_hatch(opt, opt.size_mm); break;
      case 6: ok = draw_waves(opt, opt.size_mm); break;
      case 7: ok = draw_mandala(opt, radius); break;
      default: ok = draw_spirograph(opt, radius); break;
    }
  }

  pen_z(penplotter_settings.pen_up_z);
  current_position.x = PLOTTER_CENTER_X;
  current_position.y = PLOTTER_CENTER_Y;
  line_to_current_position(MMS_SCALED(ART_TRAVEL_MM_S));
  planner.synchronize();
  penplotter_end_job();

  if (ok) {
    SERIAL_ECHOLNPGM("Art complete A", opt.art);
    ui.set_status(opt.dry_run ? F("Art dry-run done") : F("Art done"));
  }
  else {
    ui.set_status(F("Art stopped"));
  }
}

#endif // PENPLOTTER_EMBEDDED_DEMOS
