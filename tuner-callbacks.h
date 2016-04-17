#ifndef XDR_TUNER_CALLBACKS_H_
#define XDR_TUNER_CALLBACKS_H_
#include <glib.h>

gboolean tuner_ready(gpointer);
gboolean tuner_unauthorized(gpointer);
gboolean tuner_disconnect(gpointer);
gboolean tuner_freq(gpointer);
gboolean tuner_daa(gpointer);
gboolean tuner_signal(gpointer);
gboolean tuner_cci(gpointer);
gboolean tuner_aci(gpointer);
gboolean tuner_pi(gpointer);
gboolean tuner_rds(gpointer);
gboolean tuner_scan(gpointer);
gboolean tuner_pilot(gpointer);
gboolean tuner_volume(gpointer);
gboolean tuner_agc(gpointer);
gboolean tuner_deemphasis(gpointer);
gboolean tuner_antenna(gpointer);
gboolean tuner_event(gpointer);
gboolean tuner_gain(gpointer);
gboolean tuner_mode(gpointer);
gboolean tuner_filter(gpointer);
gboolean tuner_squelch(gpointer);
gboolean tuner_rotator(gpointer);
gboolean tuner_online(gpointer);
gboolean tuner_online_guests(gpointer);

#endif

