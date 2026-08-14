/**
 * Shared pen plotter settings and job state.
 */

#include "../../inc/MarlinConfig.h"

#if ENABLED(PENPLOTTER_EMBEDDED_DEMOS)

#include "penplotter_settings.h"
#include "../../MarlinCore.h"
#include "../../module/motion.h"
#include "../../module/planner.h"

PenPlotterSettings penplotter_settings = { 1.20f, 0.00f, 49 };

static volatile bool plotter_busy = false,
                     plotter_paused = false,
                     plotter_stopped = false;

static uint16_t plotter_total_steps = 0,
                plotter_done_steps = 0;

void penplotter_settings_reset() {
  penplotter_settings.pen_up_z = 1.20f;
  penplotter_settings.pen_down_z = 0.00f;
  penplotter_settings.favorite_demo = 49;
}

void penplotter_settings_sanitize() {
  penplotter_settings.pen_down_z = constrain(penplotter_settings.pen_down_z, 0.0f, float(Z_MAX_POS));
  penplotter_settings.pen_up_z = constrain(penplotter_settings.pen_up_z, 0.0f, float(Z_MAX_POS));
  if (penplotter_settings.pen_up_z < penplotter_settings.pen_down_z + 0.10f)
    penplotter_settings.pen_up_z = constrain(penplotter_settings.pen_down_z + 1.20f, 0.0f, float(Z_MAX_POS));
  if (!WITHIN(penplotter_settings.favorite_demo, 1, 51))
    penplotter_settings.favorite_demo = 49;
}

void penplotter_begin_job(const uint16_t total_steps) {
  penplotter_settings_sanitize();
  plotter_total_steps = _MAX(uint16_t(1), total_steps);
  plotter_done_steps = 0;
  plotter_stopped = false;
  plotter_paused = false;
  plotter_busy = true;
}

void penplotter_end_job() {
  plotter_done_steps = plotter_total_steps;
  plotter_busy = false;
  plotter_paused = false;
}

bool penplotter_progress_step(const uint16_t step_count/*=1*/) {
  if (plotter_stopped) return false;
  if (plotter_busy) {
    const uint16_t next = plotter_done_steps + step_count;
    plotter_done_steps = _MIN(next, plotter_total_steps);
  }
  while (plotter_paused && !plotter_stopped) {
    planner.synchronize();
    idle(true);
  }
  idle(true);
  return !plotter_stopped;
}

uint8_t penplotter_progress_percent() {
  if (!plotter_total_steps) return 0;
  return uint8_t(_MIN(100UL, (uint32_t(plotter_done_steps) * 100UL) / plotter_total_steps));
}

bool penplotter_is_busy() { return plotter_busy; }
bool penplotter_is_paused() { return plotter_paused; }
bool penplotter_is_stopped() { return plotter_stopped; }

void penplotter_request_pause(const bool pause) {
  if (!plotter_busy && pause) return;
  plotter_paused = pause;
}

void penplotter_request_stop() {
  plotter_stopped = true;
  plotter_paused = false;
  quickstop_stepper();
}

void penplotter_clear_stop() {
  plotter_stopped = false;
}

#endif // PENPLOTTER_EMBEDDED_DEMOS
