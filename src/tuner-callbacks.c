#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "tuner-callbacks.h"
#include "ui-tuner-update.h"
#include "tuner.h"
#include "conf.h"

#include "rdsspy.h"

#define DEFAULT_SAMPLING_INTERVAL 66

gboolean
tuner_ready(gpointer data)
{
    tuner.ready = TRUE;
    tuner.guest = GPOINTER_TO_INT(data);
    return FALSE;
}

gboolean
tuner_unauthorized(gpointer data)
{
    ui_unauthorized();
    return FALSE;
}

gboolean
tuner_disconnect(gpointer data)
{
    tuner_clear_all(); /* tuner.thread = NULL */
    g_free(data); /* free tuner_thread_t */

    rdsspy_reset();
    return FALSE;
}

gboolean
tuner_freq(gpointer data)
{
    gint freq = GPOINTER_TO_INT(data);
    if(freq != tuner.freq ||
       (tuner.prevantenna != tuner.antenna &&
        tuner.offset[tuner.prevantenna] != tuner.offset[tuner.antenna]))
    {
        tuner.prevfreq = tuner.freq;
        tuner.prevantenna = tuner.antenna;
        tuner.freq = freq;
    }


    tuner_clear_signal();
    tuner_clear_rds();
    ui_update_freq();

    tuner.signal     = NAN;
    tuner.signal_max = NAN;
    tuner.signal_sum = 0.0;
    tuner.signal_samples = 0;
    tuner.ready_tuned = TRUE;

    rdsspy_reset();
    return FALSE;
}

gboolean
tuner_daa(gpointer data)
{
    tuner.daa = GPOINTER_TO_INT(data);
    return FALSE;
}

gboolean
tuner_signal(gpointer data)
{
    tuner_signal_t *signal = (tuner_signal_t*)data;
    if(tuner.ready_tuned)
    {
        if(tuner.rds_timeout)
            tuner.rds_timeout--;

        tuner.signal = signal->value;
        if(isnan(tuner.signal_max) || tuner.signal > tuner.signal_max)
            tuner.signal_max = tuner.signal;
        tuner.signal_sum += tuner.signal;
        tuner.signal_samples++;
        ui_update_signal();

        tuner.stereo = signal->stereo & SIGNAL_STEREO;
        tuner.forced_mono = signal->stereo & SIGNAL_FORCED_MONO;
        ui_update_stereo_flag();
        ui_update_rds_flag();
    }
    g_free(data);
    return FALSE;
}

gboolean
tuner_cci(gpointer data)
{
    tuner.cci = GPOINTER_TO_INT(data);
    ui_update_cci();
    return FALSE;
}

gboolean
tuner_aci(gpointer data)
{
    tuner.aci = GPOINTER_TO_INT(data);
    ui_update_aci();
    return FALSE;
}

gboolean
tuner_pi(gpointer data)
{
    gint pi = GPOINTER_TO_INT(data) & 0xFFFF;
    gint err_level = (GPOINTER_TO_INT(data) & 0x30000) >> 16;
    gint interval = (tuner.sampling_interval ? tuner.sampling_interval : DEFAULT_SAMPLING_INTERVAL);

    if(err_level > tuner.rds_pi_err_level &&
       tuner.rds_pi != pi)
        return FALSE;

    /* RDS stream: 1187.5 bps
     * One group: 104 bits (each has PI code) */
    tuner.rds_timeout = ceil(1000 * 104 / 1187.5 / interval) + 1;
    tuner.rds_reset_timer = g_get_real_time();

    tuner.rds_pi = pi;
    if(err_level < tuner.rds_pi_err_level)
        tuner.rds_pi_err_level = err_level;

    ui_update_pi();
    return FALSE;
}

gboolean
tuner_rds_legacy(gpointer msg_ptr)
{
    gchar *msg = (gchar*)msg_ptr;
    guint data[4], errors;

    for (guint i = 1; i < 4; i++)
    {
        gchar hex[5];
        strncpy(hex, msg+(i-1)*4, 4);
        hex[4] = 0;
        sscanf(hex, "%x", &data[i]);
    }

    sscanf(msg+12, "%x", &errors);
    g_free(msg);

    guint errors_corrected = 0;

    if (!tuner.rds_timeout)
    {
        errors_corrected |= (0x03 << 6);
    }

    errors_corrected |= (errors & 0x03) << 4;
    errors_corrected |= (errors & 0x0C);
    errors_corrected |= (errors & 0x30) >> 4;

    char *new_format;
    new_format = g_strdup_printf("%04X%04X%04X%04X%02X",
                                 tuner.rds_pi,
                                 data[1],
                                 data[2],
                                 data[3],
                                 errors_corrected);

    tuner_rds_new(new_format);
    return FALSE;
}

gboolean
tuner_rds_new(gpointer msg_ptr)
{
    gchar *msg = (gchar*)msg_ptr;
    guint data[4], errors, i;

    if(tuner.rds_pi < 0)
    {
        g_free(msg);
        return FALSE;
    }

    for (i = 0; i < 4; i++)
    {
        gchar hex[5];
        strncpy(hex, msg+i*4, 4);
        hex[4] = 0;
        sscanf(hex, "%x", &data[i]);
    }

    sscanf(msg+16, "%x", &errors);

    rdsparser_parse_string(tuner.rds, msg);
    rdsspy_send(data, errors);

    g_free(msg);
    return FALSE;
}

gboolean
tuner_scan(gpointer data)
{
    tuner_scan_t *scan = (tuner_scan_t*)data;
    gint i;
    gint offset = tuner_get_offset();

    if(offset)
        for(i=0; i<scan->len; i++)
            scan->signals[i].freq -= offset;

    ui_update_scan(scan);
    return FALSE;
}

gboolean
tuner_pilot(gpointer data)
{
    ui_update_pilot(GPOINTER_TO_INT(data));
    return FALSE;
}

gboolean
tuner_volume(gpointer data)
{
    tuner.volume = GPOINTER_TO_INT(data);
    return FALSE;
}

gboolean
tuner_agc(gpointer data)
{
    tuner.agc = GPOINTER_TO_INT(data);
    return FALSE;
}

gboolean
tuner_deemphasis(gpointer data)
{
    tuner.deemphasis = GPOINTER_TO_INT(data);
    return FALSE;
}

gboolean
tuner_antenna(gpointer data)
{
    tuner.antenna = GPOINTER_TO_INT(data);
    if(conf.ant_clear_rds)
    {
        tuner_clear_rds();
        rdsspy_reset();
    }
    tuner_clear_signal();
    ui_update_freq();
    return FALSE;
}

gboolean
tuner_event(gpointer data)
{
    ui_action();
    return FALSE;
}

gboolean
tuner_gain(gpointer data)
{
    gint gain = GPOINTER_TO_INT(data);
    tuner.rfgain = (gain == 10 || gain == 11);
    tuner.ifgain = (gain ==  1 || gain == 11);
    tuner_clear_signal();
    return FALSE;
}

gboolean
tuner_mode(gpointer data)
{
    tuner.mode = GPOINTER_TO_INT(data);
    tuner_clear_signal();
    tuner_clear_rds();
    ui_update_mode();
    tuner.filter = -1;
    ui_update_filter();
    return FALSE;
}

gboolean
tuner_filter(gpointer data)
{
    tuner.filter = GPOINTER_TO_INT(data);
    ui_update_filter();
    return FALSE;
}

gboolean
tuner_squelch(gpointer data)
{
    tuner.squelch = GPOINTER_TO_INT(data);
    return FALSE;
}

gboolean
tuner_rotator(gpointer data)
{
    gint rotator = GPOINTER_TO_INT(data);
    tuner.rotator = abs(rotator);
    tuner.rotator_waiting = (rotator < 0);
    ui_update_rotator();
    return FALSE;
}

gboolean
tuner_sampling_interval(gpointer data)
{
    tuner.sampling_interval = GPOINTER_TO_INT(data);
    return FALSE;
}

gboolean
tuner_online(gpointer data)
{
    tuner.online = GPOINTER_TO_INT(data);
    if(!tuner.ready && tuner.online > 1)
        tuner.send_settings = FALSE;
    return FALSE;
}

gboolean
tuner_online_guests(gpointer data)
{
    tuner.online_guests = GPOINTER_TO_INT(data);
    return FALSE;
}
