#ifndef XDR_GUI_TUNER_H_
#define XDR_GUI_TUNER_H_
#include <gtk/gtk.h>

void tuner_set_frequency(gint freq);
#define tuner_reset_frequency(f) tuner_set_frequency(((f)/100)*100 + (((f)%100<50)?0:100))
void tuner_set_mode();
void tuner_set_volume();
void tuner_set_squelch();
void tuner_set_bandwidth();
void tuner_set_deemphasis();
void tuner_set_antenna();
void tuner_set_agc();
void tuner_set_gain();
void tuner_set_alignment();
void tuner_set_rotator(gpointer n);
void tuner_st_test();

#endif
