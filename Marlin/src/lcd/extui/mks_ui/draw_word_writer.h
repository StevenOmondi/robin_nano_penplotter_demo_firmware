/**
 * Pen plotter word writer screen.
 */
#pragma once

#ifdef __cplusplus
  extern "C" {
#endif

void lv_draw_word_writer();
void lv_clear_word_writer();
void plotter_text_set(const char *text);
const char *plotter_text_get();

#ifdef __cplusplus
  } /* C-declarations for C++ */
#endif
