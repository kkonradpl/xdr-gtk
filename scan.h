#ifndef XDR_SCAN_H_
#define XDR_SCAN_H_
#include "tuner-scan.h"

void scan_init();
void scan_dialog();

void scan_update(tuner_scan_t*);
void scan_update_value(gint, gfloat);

void scan_try_toggle(gboolean);
void scan_force_redraw();

#endif
