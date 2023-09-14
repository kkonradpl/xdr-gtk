#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "ui-tuner-update.h"
#include "tuner.h"
#include "conf.h"

#include "rdsspy.h"

#define DEFAULT_SAMPLING_INTERVAL 66

#define RDS_BLOCK_A 0
#define RDS_BLOCK_B 1
#define RDS_BLOCK_C 2
#define RDS_BLOCK_D 3

static void tuner_rds(guint*, guint);


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
        if(tuner.rds)
            tuner.rds--;

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
    tuner.rds = ceil(1000 * 104 / 1187.5 / interval) + 1;
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
    guint data[4], errors, i;
    guint errors_corrected = 0;

    if(tuner.rds_pi < 0)
    {
        g_free(msg);
        return FALSE;
    }

    data[0] = tuner.rds_pi;

    for (i = 1; i < 4; i++)
    {
        gchar hex[5];
        strncpy(hex, msg+(i-1)*4, 4);
        hex[4] = 0;
        sscanf(hex, "%x", &data[i]);
    }

    sscanf(msg+12, "%x", &errors);

    if (!tuner.rds)
        errors_corrected |= (0x03 << 6);

    errors_corrected |= (errors & 0x03) << 4;
    errors_corrected |= (errors & 0x0C);
    errors_corrected |= (errors & 0x30) >> 4;

    tuner_rds(data, errors_corrected);
    g_free(msg);
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

    tuner_rds(data, errors);
    g_free(msg);
    return FALSE;
}

static void
tuner_rds(guint *data,
          guint  errors)
{
    guchar group = (data[RDS_BLOCK_B] & 0xF000) >> 12;
    gboolean flag = (data[RDS_BLOCK_B] & 0x0800) >> 11;
    guchar err[] =
    {
        (errors & 192) >> 6, /* Block A */
        (errors & 48) >> 4,  /* Block B */
        (errors & 12) >> 2,  /* Block C */
        (errors & 3)         /* Block D */
    };
    guint i;

    rdsspy_send(data, errors);

    // PTY, TP, TA, MS, AF, ECC: error-free blocks
    if(!err[RDS_BLOCK_B])
    {
        gchar pty = (data[RDS_BLOCK_B] & 0x03E0) >> 5;
        gchar tp = (data[RDS_BLOCK_B] & 0x400) >> 10;

        if(tuner.rds_pty != pty)
        {
            tuner.rds_pty = pty;
            ui_update_pty();
        }

        if(tuner.rds_tp != tp)
        {
            tuner.rds_tp = tp;
            ui_update_tp();
        }

        if(group == 0)
        {
            gchar ta = (data[RDS_BLOCK_B] & 0x10) >> 4;
            gchar ms = (data[RDS_BLOCK_B] & 0x8) >> 3;
            if(tuner.rds_ta != ta)
            {
                tuner.rds_ta = ta;
                ui_update_ta();
            }
            if(tuner.rds_ms != ms)
            {
                tuner.rds_ms = ms;
                ui_update_ms();
            }

            // AF
            if(!err[RDS_BLOCK_C] && !flag)
            {
                guchar af[] = { data[RDS_BLOCK_C] >> 8, data[RDS_BLOCK_C] & 0xFF };
                for(i=0; i<2; i++)
                    if(af[i]>0 && af[i]<205)
                        ui_update_af(af[i]);
            }
        }

        // ECC
        if(group == 1 && !flag && !err[RDS_BLOCK_C])
        {
            if(!(data[RDS_BLOCK_C] >> 12))
            {
                guchar ecc = data[RDS_BLOCK_C] & 255;
                if(tuner.rds_ecc != ecc)
                {
                    tuner.rds_ecc = ecc;
                    ui_update_ecc();
                }
            }
        }
    }

    // PS: user-defined error correction
    if(conf.rds_ps_progressive || err[RDS_BLOCK_B] <= conf.rds_ps_info_error)
    {
        if(err[RDS_BLOCK_B] < 3 &&
           err[RDS_BLOCK_D] < 3 &&
           group == 0 &&
           (conf.rds_ps_progressive || (err[RDS_BLOCK_D] <= conf.rds_ps_data_error)))
        {
            gchar pos = data[RDS_BLOCK_B] & 3;
            gchar ps[] = { data[RDS_BLOCK_D] >> 8, data[RDS_BLOCK_D] & 0xFF };
            gboolean changed = FALSE;
            guchar e = 2*err[RDS_BLOCK_B] + 3*err[RDS_BLOCK_D];

            for(i=0; i<2; i++)
            {
                // only ASCII printable characters
                if(ps[i] >= 32 && ps[i] < 127)
                {
                    gint p = pos*2+i;
                    if(!conf.rds_ps_progressive || tuner.rds_ps_err[p] >= e)
                    {
                        if(tuner.rds_ps[p] != ps[i] || tuner.rds_ps_err[p] > e)
                        {
                            tuner.rds_ps[p] = ps[i];
                            tuner.rds_ps_err[p] = e;
                            changed = TRUE;
                        }
                    }
                }
            }

            if(changed)
            {
                tuner.rds_ps_avail = TRUE;
                ui_update_ps(TRUE);
            }
        }
    }

    // RT: user-defined error correction
    if (err[RDS_BLOCK_B] <= conf.rds_rt_info_error)
    {
        if (group == 2)
        {
            gboolean rt_flag = (data[RDS_BLOCK_B] & 16) >> 4;
            gchar rt[] =
            {
                data[RDS_BLOCK_C] >> 8,
                data[RDS_BLOCK_C] & 0xFF,
                data[RDS_BLOCK_D] >> 8,
                data[RDS_BLOCK_D] & 0xFF
            };

            gint length = flag ? 2 : 4;
            gint offset = flag ? 2 : 0;
            gboolean changed = FALSE;
            for (i = offset; i < 4; i++)
            {
                gint pos = (data[RDS_BLOCK_B] & 15) * length + (i - offset);

                if (tuner.rds_rt[rt_flag][pos] == rt[i])
                {
                    /* No change */
                    continue;
                }

                if (rt[i] == 0x0D)
                {
                    /* End of the RadioText message */
                    if ((i <= 1 && (errors&15) == 0) ||
                        (i >= 2 && (errors&51) == 0))
                    {
                        tuner.rds_rt[rt_flag][pos] = 0;
                        changed = TRUE;
                    }
                }
                else if (rt[i] >= 32 &&
                         rt[i] < 127)
                {
                    /* Only ASCII printable characters */
                    if ((i <= 1 && ((errors & 12) >> 2) <= conf.rds_rt_data_error) ||
                        (i >= 2 && ((errors & 48) >> 4) <= conf.rds_rt_data_error))
                    {
                        tuner.rds_rt[rt_flag][pos] = rt[i];
                        changed = TRUE;
                    }
                }
            }

            if (changed)
            {
                tuner.rds_rt_avail[rt_flag] = TRUE;
                ui_update_rt(rt_flag);
            }
        }
    }
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
