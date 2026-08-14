/**
 * Pen plotter utility controls for the ELEGOO Neptune 2S / Robin Nano build.
 *
 * The pen lift is the Z axis. There is no servo pen control in this firmware.
 */

#include "../../../inc/MarlinConfig.h"

#if ENABLED(PENPLOTTER_EMBEDDED_DEMOS)

#include "../../gcode.h"
#include "../../queue.h"
#include "../../../feature/penplotter/penplotter_settings.h"
#include "../../../lcd/marlinui.h"
#include "../../../module/motion.h"
#include "../../../module/settings.h"

namespace {
  static constexpr int16_t PLOTTER_SPEED_MIN = 10;
  static constexpr int16_t PLOTTER_SPEED_MAX = 300;
  static constexpr float PLOTTER_CENTER_X = 100.0f;
  static constexpr float PLOTTER_CENTER_Y = 100.0f;

  static void z_to_string(const float z, char * const out) {
    dtostrf(constrain(z, 0.0f, float(Z_MAX_POS)), 1, 2, out);
  }

  static void run_preflight(const char * const status) {
    char up[12], cmd[112];
    z_to_string(penplotter_settings.pen_up_z, up);
    snprintf_P(cmd, sizeof(cmd), PSTR("M17\nM211 S1\nG90\nG1 Z%s F900\nG28 X Y\nM117 %s"), up, status);
    gcode.process_subcommands_now(cmd);
  }

  static void run_preflight_then_z(const float z, const char * const status) {
    char up[12], down[12], cmd[128];
    z_to_string(penplotter_settings.pen_up_z, up);
    z_to_string(z, down);
    snprintf_P(cmd, sizeof(cmd), PSTR("M17\nM211 S1\nG90\nG1 Z%s F900\nG28 X Y\nG1 Z%s F900\nM117 %s"), up, down, status);
    gcode.process_subcommands_now(cmd);
  }

  static void run_center_park() {
    char up[12], cmd[128];
    z_to_string(penplotter_settings.pen_up_z, up);
    snprintf_P(cmd, sizeof(cmd), PSTR("M17\nM211 S1\nG90\nG1 Z%s F900\nG28 X Y\nG1 X100 Y100 F9000\nM117 Park center"), up);
    gcode.process_subcommands_now(cmd);
  }

  static void run_origin_here() {
    char up[12], cmd[128];
    z_to_string(penplotter_settings.pen_up_z, up);
    snprintf_P(cmd, sizeof(cmd), PSTR("M17\nM211 S1\nG90\nG1 Z%s F900\nG28 X Y\nG92 X0 Y0 Z0\nM117 Origin set"), up);
    gcode.process_subcommands_now(cmd);
  }

  static void run_test_tap() {
    char up[12], down[12], cmd[160];
    z_to_string(penplotter_settings.pen_up_z, up);
    z_to_string(penplotter_settings.pen_down_z, down);
    snprintf_P(cmd, sizeof(cmd), PSTR("M17\nM211 S1\nG90\nG1 Z%s F900\nG28 X Y\nG1 X100 Y100 F9000\nG1 Z%s F600\nG4 P250\nG1 Z%s F900\nM117 Pen tap"), up, down, up);
    gcode.process_subcommands_now(cmd);
  }

  static void set_plotter_speed(const int16_t percent) {
    feedrate_percentage = constrain(percent, PLOTTER_SPEED_MIN, PLOTTER_SPEED_MAX);
    SERIAL_ECHO_MSG("Plotter speed ", feedrate_percentage, "%");
  }

  static void report_plotter_controls() {
    SERIAL_ECHOLNPGM("M751 plotter controls:");
    SERIAL_ECHOLNPGM("  Motion actions lift Z and home X/Y first.");
    SERIAL_ECHOLNPGM("  A setup/home XY, U pen up, D pen down, Z<mm> pen height");
    SERIAL_ECHOLNPGM("  I<mm> set pen-up Z, K<mm> set pen-down Z, C test tap, G save, X reset");
    SERIAL_ECHOLNPGM("  S<pct> live speed, B 100%, F 150%, H 200% (no homing, no move)");
    SERIAL_ECHOLNPGM("  O origin, P park center, R dry frame, T trace frame, Q square");
    SERIAL_ECHOLNPGM("  L list demos, W words demo, M mandala demo, V favorite demo");
    SERIAL_ECHOLNPGM("  Text: M752 F0..3 S<size> A0..2 W<space> L<line> R0..3 M0..1 \"WORDS\"");
    SERIAL_ECHOLNPGM("  Art: M753 A0..8 S<size> D<density> P<dry>");
    SERIAL_ECHOPAIR_F("  Pen up Z ", penplotter_settings.pen_up_z, 2);
    SERIAL_ECHOLNPAIR_F(" down Z ", penplotter_settings.pen_down_z, 2);
  }
}

/**
 * M751: Pen plotter controls.
 *
 *  A        Setup: motors on, absolute mode, Z lift, home X/Y
 *  U/D      Pen up/down using the Z axis
 *  Z<mm>    Move pen to a specific Z height
 *  I/K      Set pen-up / pen-down Z calibration
 *  C/G/X    Test tap / save EEPROM / reset plotter calibration
 *  S<pct>   Set live feed-rate percentage while drawing
 *  B/F/H    Balanced/Fast/High live speed presets
 *  O/P      Set origin here / park at bed center
 *  R/T/Q    Dry centered frame / trace centered frame / centered square
 *  L/W/M/V  List demos / words demo / mandala demo / favorite demo
 */
void GcodeSuite::M751() {
  penplotter_settings_sanitize();

  if (parser.seenval('S')) {
    set_plotter_speed(parser.value_int());
    return;
  }

  if (parser.seen('B')) { set_plotter_speed(100); return; }
  if (parser.seen('F')) { set_plotter_speed(150); return; }
  if (parser.seen('H')) { set_plotter_speed(200); return; }

  if (parser.seenval('I')) {
    penplotter_settings.pen_up_z = parser.value_float();
    penplotter_settings_sanitize();
    SERIAL_ECHOLNPAIR_F("Pen up Z: ", penplotter_settings.pen_up_z, 2);
    ui.set_status(F("Pen up saved in RAM"));
    return;
  }

  if (parser.seenval('K')) {
    penplotter_settings.pen_down_z = parser.value_float();
    penplotter_settings_sanitize();
    SERIAL_ECHOLNPAIR_F("Pen down Z: ", penplotter_settings.pen_down_z, 2);
    ui.set_status(F("Pen down saved in RAM"));
    return;
  }

  if (parser.seenval('N')) {
    penplotter_settings.favorite_demo = constrain(parser.byteval('N'), 1, 51);
    SERIAL_ECHOLNPGM("Favorite demo D", penplotter_settings.favorite_demo);
    ui.set_status(F("Favorite demo set"));
    return;
  }

  if (parser.seenval('Z')) {
    run_preflight_then_z(parser.value_float(), "Pen Z");
    return;
  }

  if (parser.seen('G')) {
    TERN_(EEPROM_SETTINGS, (void)settings.save());
    ui.set_status(F("Plotter settings saved"));
    return;
  }

  if (parser.seen('X')) {
    penplotter_settings_reset();
    SERIAL_ECHOLNPGM("Plotter calibration reset");
    ui.set_status(F("Plotter settings reset"));
    return;
  }

  if (parser.seen('C')) { run_test_tap(); return; }
  if (parser.seen('A')) { set_plotter_speed(100); run_preflight("Plotter ready"); return; }
  if (parser.seen('U')) { run_preflight_then_z(penplotter_settings.pen_up_z, "Pen up"); return; }
  if (parser.seen('D')) { run_preflight_then_z(penplotter_settings.pen_down_z, "Pen down"); return; }
  if (parser.seen('O')) { run_origin_here(); return; }
  if (parser.seen('P')) { run_center_park(); return; }
  if (parser.seen('R')) { queue.enqueue_one(F("M753 A0 S180 P1")); return; }
  if (parser.seen('T')) { queue.enqueue_one(F("M753 A0 S180")); return; }
  if (parser.seen('Q')) { queue.enqueue_one(F("M753 A0 S50")); return; }
  if (parser.seen('L')) { queue.inject(F("M750")); return; }
  if (parser.seen('W')) { queue.enqueue_one(F("M752 D1")); return; }
  if (parser.seen('M')) { queue.enqueue_one(F("M753 A7 S160 D4")); return; }
  if (parser.seen('V')) {
    char cmd[16];
    snprintf_P(cmd, sizeof(cmd), PSTR("M750 D%u"), penplotter_settings.favorite_demo);
    queue.enqueue_one(cmd);
    return;
  }

  report_plotter_controls();
}

#endif // PENPLOTTER_EMBEDDED_DEMOS
