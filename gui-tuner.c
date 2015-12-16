#include <gtk/gtk.h>
#include <math.h>
#include "gui.h"
#include "gui-tuner.h"
#include "settings.h"
#include "stationlist.h"
#include "tuner.h"

void tuner_set_frequency(gint freq)
{
    gchar buffer[8];
    gui_antenna_switch(freq);
    g_snprintf(buffer, sizeof(buffer), "T%d", freq);
    tuner_write(buffer);
}

void tuner_set_mode()
{
    gchar buffer[3];
    g_snprintf(buffer, sizeof(buffer), "M%d", tuner.mode);
    tuner_write(buffer);
}

void tuner_set_bandwidth()
{
    gchar buffer[4];
    tuner.filter = gtk_combo_box_get_active(GTK_COMBO_BOX(gui.c_bw));
    g_snprintf(buffer, sizeof(buffer), "F%d", filters[tuner.filter]);
    tuner_write(buffer);
    stationlist_bw(tuner.filter);
}

void tuner_set_deemphasis()
{
    gchar buffer[3];
    conf.deemphasis = gtk_combo_box_get_active(GTK_COMBO_BOX(gui.c_deemph));
    g_snprintf(buffer, sizeof(buffer), "D%d", conf.deemphasis);
    tuner_write(buffer);
}

void tuner_set_volume()
{
    gchar buffer[5];
    g_snprintf(buffer, sizeof(buffer), "Y%d", (gint)round(gtk_scale_button_get_value(GTK_SCALE_BUTTON(gui.volume))));
    tuner_write(buffer);
}

void tuner_set_squelch()
{
    gchar buffer[5];
    g_snprintf(buffer, sizeof(buffer), "Q%d", (gint)round(gtk_scale_button_get_value(GTK_SCALE_BUTTON(gui.squelch))));
    tuner_write(buffer);
}

void tuner_set_antenna()
{
    gchar buffer[3];
    g_snprintf(buffer, sizeof(buffer), "Z%d", gtk_combo_box_get_active(GTK_COMBO_BOX(gui.c_ant)));
    tuner_write(buffer);
}

void tuner_set_agc()
{
    gchar buffer[3];
    conf.agc = gtk_combo_box_get_active(GTK_COMBO_BOX(gui.c_agc));
    g_snprintf(buffer, sizeof(buffer), "A%d", conf.agc);
    tuner_write(buffer);
}

void tuner_set_gain()
{
    gchar buffer[4];
    conf.rfgain = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gui.x_rf));
    conf.ifgain = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gui.x_if));
    g_snprintf(buffer, sizeof(buffer), "G%d%d", conf.rfgain, conf.ifgain);
    tuner_write(buffer);
}

void tuner_set_alignment()
{
    gchar buffer[5];
    g_snprintf(buffer, sizeof(buffer), "V%d", (gint)gtk_adjustment_get_value(GTK_ADJUSTMENT(gui.adj_align)));
    tuner_write(buffer);
}

void tuner_set_rotator(gpointer user_data)
{
    gint n = GPOINTER_TO_INT(user_data);
    gboolean cw = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gui.b_cw));
    gboolean ccw = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gui.b_ccw));
    gchar buffer[3];

    if(!cw && !ccw)
    {
        n = 0;
        gtk_widget_modify_bg(gui.b_cw, GTK_STATE_ACTIVE, &gui.colors.prelight);
        gtk_widget_modify_bg(gui.b_cw, GTK_STATE_PRELIGHT, &gui.colors.prelight);
        gtk_widget_modify_bg(gui.b_ccw, GTK_STATE_ACTIVE, &gui.colors.prelight);
        gtk_widget_modify_bg(gui.b_ccw, GTK_STATE_PRELIGHT, &gui.colors.prelight);
    }
    else if(n == 1)
    {
        gtk_widget_modify_bg(gui.b_cw, GTK_STATE_ACTIVE, &gui.colors.action);
        gtk_widget_modify_bg(gui.b_cw, GTK_STATE_PRELIGHT, &gui.colors.action);
        if(ccw)
        {
            g_signal_handlers_block_by_func(G_OBJECT(gui.b_ccw), GINT_TO_POINTER(tuner_set_rotator), GINT_TO_POINTER(2));
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.b_ccw), FALSE);
            g_signal_handlers_unblock_by_func(G_OBJECT(gui.b_ccw), GINT_TO_POINTER(tuner_set_rotator), GINT_TO_POINTER(2));
        }
    }
    else if(n == 2)
    {
        gtk_widget_modify_bg(gui.b_ccw, GTK_STATE_ACTIVE, &gui.colors.action);
        gtk_widget_modify_bg(gui.b_ccw, GTK_STATE_PRELIGHT, &gui.colors.action);
        if(cw)
        {
            g_signal_handlers_block_by_func(G_OBJECT(gui.b_cw), GINT_TO_POINTER(tuner_set_rotator), GINT_TO_POINTER(1));
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.b_cw), FALSE);
            g_signal_handlers_unblock_by_func(G_OBJECT(gui.b_cw), GINT_TO_POINTER(tuner_set_rotator), GINT_TO_POINTER(1));
        }
    }

    g_snprintf(buffer, sizeof(buffer), "C%d", n);
    tuner_write(buffer);
}

void tuner_st_test()
{
    tuner_write("N");
}
