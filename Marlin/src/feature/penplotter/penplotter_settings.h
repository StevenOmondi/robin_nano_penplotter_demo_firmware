/**
 * Shared pen plotter settings and job state.
 *
 * The pen lift is the Z axis. No servo or extruder state is used here.
 */
#pragma once

#include "../../inc/MarlinConfig.h"

struct PenPlotterSettings {
  float pen_up_z;
  float pen_down_z;
  uint8_t favorite_demo;
};

extern PenPlotterSettings penplotter_settings;

void penplotter_settings_reset();
void penplotter_settings_sanitize();

void penplotter_begin_job(const uint16_t total_steps);
void penplotter_end_job();
bool penplotter_progress_step(const uint16_t step_count=1);
uint8_t penplotter_progress_percent();
bool penplotter_is_busy();
bool penplotter_is_paused();
bool penplotter_is_stopped();
void penplotter_request_pause(const bool pause);
void penplotter_request_stop();
void penplotter_clear_stop();
