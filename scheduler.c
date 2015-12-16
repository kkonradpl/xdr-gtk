#include <gtk/gtk.h>
#include "gui.h"
#include "gui-tuner.h"
#include "settings.h"
#include "scheduler.h"

guint scheduler_id = 0;
guint scheduler_next;

void scheduler_toggle()
{
    if(!scheduler_id)
    {
        scheduler_start();
    }
    else
    {
        scheduler_stop();
    }
}

void scheduler_start()
{
    if(!conf.scheduler_n)
    {
        scheduler_toggle_button(FALSE);
        dialog_error("Frequency scheduler", "No scheduler frequencies defined.\nAdd some in settings first.");
        return;
    }
    scheduler_next = 0;
    scheduler_switch(NULL);
    scheduler_toggle_button(TRUE);
}

void scheduler_stop()
{
    if(scheduler_id)
    {
        scheduler_toggle_button(FALSE);
        g_source_remove(scheduler_id);
        scheduler_id = 0;
    }
}

gboolean scheduler_switch(gpointer data)
{
    tuner_set_frequency(conf.scheduler_freqs[scheduler_next]);
    scheduler_id = g_timeout_add(conf.scheduler_timeouts[scheduler_next]*1000, scheduler_switch, NULL);
    scheduler_next = (scheduler_next + 1)%conf.scheduler_n;
    return FALSE;
}

void scheduler_toggle_button(gboolean active)
{
    g_signal_handlers_block_by_func(G_OBJECT(gui.b_scheduler), GINT_TO_POINTER(scheduler_toggle), NULL);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui.b_scheduler), GPOINTER_TO_INT(active));
    g_signal_handlers_unblock_by_func(G_OBJECT(gui.b_scheduler), GINT_TO_POINTER(scheduler_toggle), NULL);
}
