#include <math.h>
#include "ui.h"
#include "ui-tuner-set.h"
#include "ui-tuner-update.h"
#include "tuner.h"
#include "conf.h"
#include "stationlist.h"

static void tuner_modify_frequency_full(guint, guint, guint);

void
tuner_set_frequency(gint freq)
{
    static gint freq_waiting = -1;
    static gint64 last_request = 0;
    gint64 now = g_get_real_time();
    gchar buffer[8];
    gint real_freq;
    gint ant;

    ant = ui_antenna_id(freq);
    real_freq = freq + tuner.offset[ant];

    if((now-last_request) < 1000000 &&
       real_freq == freq_waiting &&
       real_freq != tuner.freq)
        return;

    g_snprintf(buffer, sizeof(buffer), "T%d", real_freq);
    tuner_write(tuner.thread, buffer);

    ui_antenna_switch(freq);

    freq_waiting = real_freq;
    last_request = now;
}

void
tuner_set_frequency_prev()
{
    tuner_set_frequency(tuner.prevfreq - tuner.offset[tuner.prevantenna]);
}

void
tuner_set_mode(gint mode)
{
    gchar buffer[3];
    g_snprintf(buffer, sizeof(buffer), "M%d", mode);
    tuner_write(tuner.thread, buffer);
}

void
tuner_set_bandwidth()
{
    gchar buffer[16];
    gint index = gtk_combo_box_get_active(GTK_COMBO_BOX(ui.c_bw));

    g_snprintf(buffer, sizeof(buffer), "F%d", tuner_filter_from_index(index));
    tuner_write(tuner.thread, buffer);

    g_snprintf(buffer, sizeof(buffer), "W%d", tuner_filter_bw_from_index(index));
    tuner_write(tuner.thread, buffer);

    tuner.last_set_bandwidth = g_get_real_time() / 1000;
}

void
tuner_set_deemphasis()
{
    gchar buffer[3];
    conf.deemphasis = gtk_combo_box_get_active(GTK_COMBO_BOX(ui.c_deemph));
    g_snprintf(buffer, sizeof(buffer), "D%d", conf.deemphasis);
    tuner_write(tuner.thread, buffer);
    tuner.last_set_deemph = g_get_real_time() / 1000;
}

void
tuner_set_volume()
{
    gchar buffer[5];
    conf.volume = lround(gtk_scale_button_get_value(GTK_SCALE_BUTTON(ui.volume)));
    g_snprintf(buffer, sizeof(buffer), "Y%d", conf.volume);
    tuner_write(tuner.thread, buffer);
    tuner.last_set_volume = g_get_real_time() / 1000;
}

void
tuner_set_squelch()
{
    gchar buffer[5];
    g_snprintf(buffer, sizeof(buffer), "Q%ld", lround(gtk_scale_button_get_value(GTK_SCALE_BUTTON(ui.squelch))));
    tuner_write(tuner.thread, buffer);
    tuner.last_set_squelch = g_get_real_time() / 1000;
}

void
tuner_set_antenna()
{
    gchar buffer[3];
    gint antenna = gtk_combo_box_get_active(GTK_COMBO_BOX(ui.c_ant));
    g_snprintf(buffer, sizeof(buffer), "Z%d", (antenna >=0 ? antenna : 0));
    tuner_write(tuner.thread, buffer);
    tuner.last_set_ant = g_get_real_time() / 1000;
}

void
tuner_set_agc()
{
    gchar buffer[3];
    conf.agc = gtk_combo_box_get_active(GTK_COMBO_BOX(ui.c_agc));
    g_snprintf(buffer, sizeof(buffer), "A%d", conf.agc);
    tuner_write(tuner.thread, buffer);
    tuner.last_set_agc = g_get_real_time() / 1000;
}

void
tuner_set_gain()
{
    gchar buffer[4];
    conf.rfgain = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui.x_rf));
    conf.ifgain = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui.x_if));
    g_snprintf(buffer, sizeof(buffer), "G%02d", conf.rfgain * 10 + conf.ifgain);
    tuner_write(tuner.thread, buffer);
    tuner.last_set_gain = g_get_real_time() / 1000;
}

void
tuner_set_alignment()
{
    gchar buffer[5];
    g_snprintf(buffer, sizeof(buffer), "V%ld", lround(gtk_adjustment_get_value(GTK_ADJUSTMENT(ui.adj_align))));
    tuner_write(tuner.thread, buffer);
    tuner.last_set_daa = g_get_real_time() / 1000;
}

void
tuner_set_rotator(gpointer user_data)
{
    gboolean cw = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui.b_cw));
    gboolean ccw = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ui.b_ccw));
    gchar buffer[3];
    gint state = GPOINTER_TO_INT(user_data);

    if(!cw && !ccw)
    {
        state = 0;
    }
    else if(state == 1)
    {
        if(ccw)
        {
            g_signal_handlers_block_by_func(G_OBJECT(ui.b_ccw), GINT_TO_POINTER(tuner_set_rotator), GINT_TO_POINTER(2));
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui.b_ccw), FALSE);
            g_signal_handlers_unblock_by_func(G_OBJECT(ui.b_ccw), GINT_TO_POINTER(tuner_set_rotator), GINT_TO_POINTER(2));
        }
    }
    else if(state == 2)
    {
        if(cw)
        {
            g_signal_handlers_block_by_func(G_OBJECT(ui.b_cw), GINT_TO_POINTER(tuner_set_rotator), GINT_TO_POINTER(1));
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui.b_cw), FALSE);
            g_signal_handlers_unblock_by_func(G_OBJECT(ui.b_cw), GINT_TO_POINTER(tuner_set_rotator), GINT_TO_POINTER(1));
        }
    }

    g_snprintf(buffer, sizeof(buffer), "C%d", state);
    tuner_write(tuner.thread, buffer);
    tuner.last_set_rotator = g_get_real_time() / 1000;
}

void
tuner_set_forced_mono(gboolean value)
{
    if(value)
        tuner_write(tuner.thread, "B1");
    else
        tuner_write(tuner.thread, "B0");
}

void
tuner_set_stereo_test()
{
    tuner_write(tuner.thread, "N");
    tuner.last_set_pilot = g_get_real_time() / 1000;
}

void
tuner_set_sampling_interval(gint interval, gboolean mode)
{
    gchar buffer[10];
    g_snprintf(buffer, sizeof(buffer), "I%d,%d", interval, mode);
    tuner_write(tuner.thread, buffer);
}

void
tuner_modify_frequency(guint mode)
{
    if(tuner_get_freq() <= 300)
        tuner_modify_frequency_full(99, 9, mode);
    else if(tuner_get_freq() <= 1900)
        tuner_modify_frequency_full((conf.mw_10k_steps?100:99), (conf.mw_10k_steps?10:9), mode);
    else if(tuner_get_freq() <= 30000)
        tuner_modify_frequency_full(1900, 10, mode);
    else if(tuner_get_freq() >= 65750 && tuner_get_freq() <= 74000)
        tuner_modify_frequency_full(65750, 30, mode);
    else
        tuner_modify_frequency_full(0, 100, mode);
}

static void
tuner_modify_frequency_full(guint base_freq,
                            guint step,
                            guint mode)
{
    gint m;
    if(((tuner_get_freq()-base_freq) % step) == 0)
    {
        if(mode == TUNER_FREQ_MODIFY_UP)
            tuner_set_frequency(tuner_get_freq()+step);
        else if(mode == TUNER_FREQ_MODIFY_DOWN)
            tuner_set_frequency(tuner_get_freq()-step);
        else if(mode == TUNER_FREQ_MODIFY_RESET)
            tuner_set_frequency(tuner_get_freq());
    }
    else
    {
        m = (tuner_get_freq()-base_freq) % step;
        if(mode == TUNER_FREQ_MODIFY_UP ||
           (mode == TUNER_FREQ_MODIFY_RESET && m >= (step/2)))
        {
            tuner_set_frequency(base_freq+((tuner_get_freq()-base_freq)/step*step)+step);
        }
        else if(mode == TUNER_FREQ_MODIFY_DOWN ||
                (mode == TUNER_FREQ_MODIFY_RESET && m < (step/2)))
        {
            tuner_set_frequency(base_freq+((tuner_get_freq()-base_freq)/step*step));
        }
    }
}

