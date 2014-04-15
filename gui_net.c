#include <gtk/gtk.h>
#include "gui.h"
#include "connection.h"

gboolean gui_update_volume(gpointer data)
{
    g_signal_handlers_block_by_func(G_OBJECT(gui.volume), GINT_TO_POINTER(tty_change_volume), NULL);
    gtk_scale_button_set_value(GTK_SCALE_BUTTON(gui.volume), GPOINTER_TO_INT(data) / 2047.0);
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.volume), GINT_TO_POINTER(tty_change_volume), NULL);
    return FALSE;
}

gboolean gui_update_agc(gpointer data)
{
    g_signal_handlers_block_by_func(G_OBJECT(gui.c_agc), GINT_TO_POINTER(tty_change_agc), NULL);
    gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_agc), GPOINTER_TO_INT(data));
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.c_agc), GINT_TO_POINTER(tty_change_agc), NULL);
    return FALSE;
}

gboolean gui_update_deemphasis(gpointer data)
{
    g_signal_handlers_block_by_func(G_OBJECT(gui.c_deemph), GINT_TO_POINTER(tty_change_deemphasis), NULL);
    gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_deemph), GPOINTER_TO_INT(data));
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.c_deemph), GINT_TO_POINTER(tty_change_deemphasis), NULL);
    return FALSE;
}

gboolean gui_update_ant(gpointer data)
{
    g_signal_handlers_block_by_func(G_OBJECT(gui.c_ant), GINT_TO_POINTER(tty_change_ant), NULL);
    gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_ant), GPOINTER_TO_INT(data));
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.c_ant), GINT_TO_POINTER(tty_change_ant), NULL);
    return FALSE;
}

gboolean gui_update_gain(gpointer data)
{
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.x_rf), ((GPOINTER_TO_INT(data) == 10 || GPOINTER_TO_INT(data) == 11) ? TRUE : FALSE));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.x_if), ((GPOINTER_TO_INT(data) == 01 || GPOINTER_TO_INT(data) == 11) ? TRUE : FALSE));
    return FALSE;
}

gboolean gui_update_filter(gpointer data)
{
    gint i;

    for(i=0; i<filters_n; i++)
    {
        if(filters[i] == GPOINTER_TO_INT(data))
        {
            g_signal_handlers_block_by_func(G_OBJECT(gui.c_bw), GINT_TO_POINTER(tty_change_bandwidth), NULL);
            gtk_combo_box_set_active(GTK_COMBO_BOX(gui.c_bw), i);
            g_signal_handlers_unblock_by_func(G_OBJECT(gui.c_bw), GINT_TO_POINTER(tty_change_bandwidth), NULL);
            break;
        }
    }

    return FALSE;
}

gboolean gui_auth(gpointer data)
{
    switch(GPOINTER_TO_INT(data))
    {
        case 0:
            dialog_error("Wrong password!");
            break;

        case 1:
            g_source_remove(gui.status_timeout);
            gtk_label_set_text(GTK_LABEL(gui.l_status), "Logged in as a guest!");
            gui.status_timeout = g_timeout_add(1000, (GSourceFunc)gui_update_clock, (gpointer)gui.l_status);
            break;
    }

    return FALSE;
}
