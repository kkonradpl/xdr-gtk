#ifndef XDR_TUNER_H_
#define XDR_TUNER_H_
#include <glib.h>
#include <librdsparser.h>
#include "conf.h"
#include "tuner-filters.h"

#define TUNER_THREAD_SERIAL 0
#define TUNER_THREAD_SOCKET 1

#define MODE_FM 0
#define MODE_AM 1

#define SIGNAL_MONO             0
#define SIGNAL_STEREO           1
#define SIGNAL_FORCED_MONO  (1<<1)

#define TUNER_FREQ_MIN 100
#define TUNER_FREQ_MAX 200000

typedef struct signal_data
{
    gfloat   level;
    gboolean rds;
    gboolean stereo;
} signal_data_t;

typedef struct tuner_signal
{
    gfloat value;
    gboolean stereo;
} tuner_signal_t;

typedef struct tuner
{
    gint mode;
    gint freq;
    gint prevfreq;
    gint prevantenna;

    gint     sampling_interval;
    gfloat   signal;
    gfloat   signal_max;
    gdouble  signal_sum;
    guint    signal_samples;
    gboolean stereo;

    gint     cci;
    gint     aci;

    rdsparser_t *rds;
    gint         rds_timeout;
    gint64       rds_reset_timer;
    gint         rds_pi;
    gint         rds_pi_err_level;

    gint daa;
    gint volume;
    gint agc;
    gint deemphasis;
    gint antenna;
    gint rfgain;
    gint ifgain;
    gint filter;
    gint squelch;
    gint rotator;
    gboolean rotator_waiting;
    gboolean forced_mono;

    gboolean ready;
    gboolean ready_tuned;
    gboolean guest;
    gint online;
    gint online_guests;
    gboolean send_settings;
    gpointer thread;

    gint64 last_set_filter;
    gint64 last_set_deemph;
    gint64 last_set_agc;
    gint64 last_set_ant;
    gint64 last_set_volume;
    gint64 last_set_squelch;
    gint64 last_set_gain;
    gint64 last_set_daa;
    gint64 last_set_rotator;
    gint64 last_set_pilot;

    gint offset[ANT_COUNT];
} tuner_t;

extern tuner_t tuner;

gpointer tuner_thread_new(gint, gintptr);
gboolean tuner_write_socket(gintptr, gchar*, int);
void tuner_thread_cancel(gpointer);
void tuner_write(gpointer, gchar*);

void tuner_rds_init();
void tuner_rds_configure();
void tuner_clear_all();
void tuner_clear_signal();
void tuner_clear_rds();
gint tuner_get_freq();
gint tuner_get_offset();
void tuner_set_offset(gint, gint);

#endif
