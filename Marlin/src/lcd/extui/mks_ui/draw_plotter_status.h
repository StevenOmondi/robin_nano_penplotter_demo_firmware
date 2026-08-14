/**
 * Shared live status bar for pen plotter screens.
 *
 * Shows pen state (up/down/moving), live X/Y position, and the current
 * plotter job state (idle/busy/paused/stopped) in a slim strip under the
 * header. Refreshed by an LVGL task every 300 ms.
 */

#ifndef DRAW_PLOTTER_STATUS_H
#define DRAW_PLOTTER_STATUS_H

#include <lvgl.h>

void plotter_status_create(lv_obj_t *scr);
void plotter_status_start();
void plotter_status_stop();
void plotter_status_refresh(lv_task_t *task);

#endif // DRAW_PLOTTER_STATUS_H
