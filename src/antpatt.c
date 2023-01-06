#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include "ui.h"
#include "tuner.h"

struct xdrgtk_antpatt
{
    GIOChannel *channel;
    gboolean active;
};

static struct xdrgtk_antpatt antpatt =
{
    .channel = NULL,
    .active = FALSE
};

static void antpatt_callback(GPid, gint, gpointer);
static void antpatt_write(GIOChannel*, gboolean, const gchar*, ...);
static void antpatt_start(void);
static void antpatt_stop(void);

static void antpatt_ui_set_running(gboolean);
static void antpatt_ui_set_active(gboolean);

                      
void
antpatt_toggle(void)
{
    gchar *command[] = { "antpatt", (conf.dark_theme ? "-id" : "-i"), NULL };
    GSpawnFlags flags = G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD;
    GPid pid;
    gint stream;

    if (antpatt.channel)
    {
        antpatt_ui_set_running(TRUE);
        if (!antpatt.active)
        {
            antpatt_start();
        }
        else
        {
            antpatt_stop();
        }
        return;
    }

    if (!g_spawn_async_with_pipes(NULL, command, NULL, flags, 0, NULL, &pid, &stream, NULL, NULL, NULL))
    {
        antpatt_ui_set_running(FALSE);
        ui_dialog(ui.window,
                  GTK_MESSAGE_ERROR,
                  "Antenna pattern",
                  "Failed to run antpatt.\nMake sure it is installed or available the same directory as xdr-gtk.");
        return;
    }

#ifdef G_OS_WIN32
    antpatt.channel = g_io_channel_win32_new_fd(stream);
#else
    antpatt.channel = g_io_channel_unix_new(stream);
#endif
    if (antpatt.channel)
    {
        antpatt.active = FALSE;
        g_child_watch_add(pid, (GChildWatchFunc)antpatt_callback, NULL);
        g_io_channel_set_close_on_unref(antpatt.channel, TRUE);
        antpatt_ui_set_running(TRUE);
    }
    else
    {
        g_spawn_close_pid(pid);
    }
}

void
antpatt_push(gfloat level)
{
    if (antpatt.channel && antpatt.active)
        antpatt_write(antpatt.channel, TRUE, "PUSH %.2f\n", level);
}

static void
antpatt_callback(GPid     pid,
                 gint     status,
                 gpointer user_data)
{
    if (antpatt.channel)
    {
        g_io_channel_unref(antpatt.channel);
        antpatt.channel = NULL;
    }

    g_spawn_close_pid(pid);
    
    antpatt_ui_set_active(FALSE);
    antpatt_ui_set_running(FALSE);
}

static void
antpatt_write(GIOChannel  *channel,
              gboolean     flush,
              const gchar *format,
              ...)
{
    va_list args;
    gchar *msg;
    gsize written;

    va_start(args, format);
    msg = g_strdup_vprintf(format, args);
    va_end(args);
    
    g_io_channel_write_chars(channel, msg, strlen(msg), &written, NULL);
    
    if (flush)
        g_io_channel_flush(channel, NULL);
    
    g_free(msg);
}

static void
antpatt_start(void)
{
    if (antpatt.channel)
    {
        antpatt_write(antpatt.channel, FALSE, "START\n");
        antpatt_write(antpatt.channel, TRUE, "FREQ %d\n", tuner_get_freq());
        antpatt_ui_set_active(TRUE);
        antpatt.active = TRUE;
    }
}

static void
antpatt_stop(void)
{
    if (antpatt.channel)
    {
        antpatt_write(antpatt.channel, TRUE, "STOP\n");
        antpatt_ui_set_active(FALSE);
        antpatt.active = FALSE;
    }
}

static void
antpatt_ui_set_running(gboolean is_active)
{
    g_signal_handlers_block_by_func(G_OBJECT(ui.b_pattern), GINT_TO_POINTER(antpatt_toggle), NULL);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui.b_pattern), is_active);
    g_signal_handlers_unblock_by_func(G_OBJECT(ui.b_pattern), GINT_TO_POINTER(antpatt_toggle), NULL);
}

static void
antpatt_ui_set_active(gboolean state)
{
    if (state)
        gtk_style_context_add_class(gtk_widget_get_style_context(ui.b_pattern), "xdr-action");
    else
        gtk_style_context_remove_class(gtk_widget_get_style_context(ui.b_pattern), "xdr-action");
}
