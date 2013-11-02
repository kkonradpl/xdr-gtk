#include <gtk/gtk.h>
#include "gui.h"
#include "connection.h"

gboolean gui_update_volume(gpointer data)
{
    gdouble val = (*(gint*)data) / 2047.0;

    g_signal_handlers_block_by_func(G_OBJECT(gui.volume), tty_change_volume, NULL);
    gtk_scale_button_set_value(GTK_SCALE_BUTTON(gui.volume), val);
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.volume), tty_change_volume, NULL);

    g_free(data);
    return FALSE;
}

gboolean gui_update_agc(gpointer data)
{
    gint *id = (gint*)data;

    g_signal_handlers_block_by_func(G_OBJECT(gui.c_agc), tty_change_agc, NULL);
    gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_agc), *id);
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.c_agc), tty_change_agc, NULL);

    g_free(id);
    return FALSE;
}

gboolean gui_update_deemphasis(gpointer data)
{
    gint *id = (gint*)data;

    g_signal_handlers_block_by_func(G_OBJECT(gui.c_deemph), tty_change_deemphasis, NULL);
    gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_deemph), *id);
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.c_deemph), tty_change_deemphasis, NULL);

    g_free(id);
    return FALSE;
}

gboolean gui_update_ant(gpointer data)
{
    gint *id = (gint*)data;

    g_signal_handlers_block_by_func(G_OBJECT(gui.c_ant), tty_change_ant, NULL);
    gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_ant), *id);
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.c_ant), tty_change_ant, NULL);

    g_free(id);
    return FALSE;
}

gboolean gui_update_gain(gpointer data)
{
    gchar *rfif = (gchar*)data;

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.x_rf), (rfif[0]=='1'?TRUE:FALSE));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.x_if), (rfif[1]=='1'?TRUE:FALSE));

    g_free(data);
    return FALSE;
}

gboolean gui_update_filter(gpointer data)
{
    gint *id = (gint*)data;
    gint i;

    for(i=0; i<filters_n; i++)
    {
        if(filters[i] == *id)
        {
            g_signal_handlers_block_by_func(G_OBJECT(gui.c_bw), tty_change_bandwidth, NULL);
            gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_bw), i);
            g_signal_handlers_unblock_by_func(G_OBJECT(gui.c_bw), tty_change_bandwidth, NULL);
            break;
        }
    }

    g_free(id);
    return FALSE;
}
