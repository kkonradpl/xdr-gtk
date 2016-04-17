#ifndef XDR_UI_TUNER_SET_H_
#define XDR_UI_TUNER_SET_H_
#include <glib.h>

#define TUNER_FREQ_MODIFY_DOWN  0
#define TUNER_FREQ_MODIFY_UP    1
#define TUNER_FREQ_MODIFY_RESET 2

void tuner_set_frequency(gint);
void tuner_set_mode(gint);
void tuner_set_bandwidth();
void tuner_set_deemphasis();
void tuner_set_volume();
void tuner_set_squelch();
void tuner_set_antenna();
void tuner_set_agc();
void tuner_set_gain();
void tuner_set_alignment();
void tuner_set_rotator(gpointer);
void tuner_set_forced_mono(gboolean);
void tuner_set_stereo_test();
void tuner_modify_frequency(guint);

#endif
