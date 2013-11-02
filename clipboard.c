#include <gtk/gtk.h>
#include <string.h>
#include "gui.h"
#include "settings.h"
#include "clipboard.h"
#include "connection.h"

gboolean clipboard_full(GtkWidget *widget, GdkEvent *event, gpointer nothing)
{
    gchar buff[30];
    gchar* pi_text = get_pi();
    gchar* rds_ps;

    if(ps_available)
    {
        if(conf.replace_spaces)
        {
            rds_ps = replace_spaces(ps_data);
            g_snprintf(buff, sizeof(buff), "%.3f %s %s", (freq/1000.0), pi_text, rds_ps);
            g_free(rds_ps);
        }
        else
        {
            g_snprintf(buff, sizeof(buff), "%.3f %s %s", (freq/1000.0), pi_text, ps_data);
        }
    }
    else if(pi >= 0)
    {
        g_snprintf(buff, sizeof(buff), "%.3f %s", (freq/1000.0), pi_text);
    }
    else
    {
        g_snprintf(buff, sizeof(buff), "%.3f", (freq/1000.0));
    }

    gtk_clipboard_set_text(gui.clipboard, buff, -1);
    g_free(pi_text);
    return FALSE;
}

gboolean clipboard_pi(GtkWidget *widget, GdkEvent *event, gpointer nothing)
{
    gchar* pi_text;
    if(pi >= 0)
    {
        pi_text = get_pi();
        gtk_clipboard_set_text(gui.clipboard, pi_text, -1);
        g_free(pi_text);
    }
    return FALSE;
}

gboolean clipboard_str(GtkWidget *widget, GdkEvent *event, gpointer data)
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

gchar* replace_spaces(gchar* str)
{
    gchar* new_str = g_strdup(str);
    size_t i;
    for(i=0; i<strlen(new_str); i++)
    {
        if(new_str[i] == ' ')
        {
            new_str[i] = '_';
        }
    }
    return new_str;
}

gchar* get_pi()
{
    gchar* pi_text = g_strdup(gtk_label_get_text(GTK_LABEL(gui.l_pi)));
    if(pi_text[4] == ' ')  // trim text
    {
        pi_text[4] = 0;
    }
    return pi_text;
}
