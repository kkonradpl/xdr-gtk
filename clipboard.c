#include <gtk/gtk.h>
#include <string.h>
#include "gui.h"
#include "gui-update.h"
#include "settings.h"
#include "clipboard.h"
#include "tuner.h"
#include "log.h"

gboolean clipboard_full(GtkWidget *widget, GdkEvent *event, gpointer nothing)
{
    const gchar* freq_text = gtk_label_get_text(GTK_LABEL(gui.l_freq));
    const gchar* pi_text = gtk_label_get_text(GTK_LABEL(gui.l_pi));
    gchar buff[30];
    gchar* ps;

    if(tuner.ps_avail)
    {
        if(conf.replace_spaces)
        {
            ps = replace_spaces(tuner.ps);
            g_snprintf(buff, sizeof(buff), "%s %s %s", freq_text, pi_text, ps);
            g_free(ps);
        }
        else
        {
            g_snprintf(buff, sizeof(buff), "%s %s %s", freq_text, pi_text, tuner.ps);
        }
    }
    else if(tuner.pi >= 0)
    {
        g_snprintf(buff, sizeof(buff), "%s %s", freq_text, pi_text);
    }
    else
    {
        g_snprintf(buff, sizeof(buff), "%s", freq_text);
    }

    gtk_clipboard_set_text(gui.clipboard, buff, -1);
    return FALSE;
}

gboolean clipboard_pi(GtkWidget *widget, GdkEvent *event, gpointer nothing)
{
    if(tuner.pi >= 0)
    {
        gtk_clipboard_set_text(gui.clipboard, gtk_label_get_text(GTK_LABEL(gui.l_pi)), -1);
    }
    return FALSE;
}

gboolean clipboard_ps(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    gchar* ps;
    if(event->type == GDK_BUTTON_PRESS && event->button == 3) // right click
    {
        conf.rds_ps_progressive = !conf.rds_ps_progressive;
        settings_write();
        if(tuner.ps_avail)
        {
            gui_update_ps(NULL);
        }
        return FALSE;
    }
    if(!tuner.ps_avail)
    {
        return FALSE;
    }
    if(conf.replace_spaces)
    {
        ps = replace_spaces(tuner.ps);
        gtk_clipboard_set_text(gui.clipboard, ps, -1);
        g_free(ps);
    }
    else
    {
        gtk_clipboard_set_text(gui.clipboard, tuner.ps, -1);
    }
    return FALSE;
}

gboolean clipboard_rt(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    gchar* str;
    if(conf.replace_spaces)
    {
        str = replace_spaces((gchar*)data);
        gtk_clipboard_set_text(gui.clipboard, str, -1);
        g_free(str);
    }
    else
    {
        gtk_clipboard_set_text(gui.clipboard, (gchar*)data, -1);
    }

    return FALSE;
}
