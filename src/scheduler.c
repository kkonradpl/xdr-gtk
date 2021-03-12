#include <gtk/gtk.h>
#include "scheduler.h"
#include "ui.h"
#include "ui-tuner-set.h"
#include "conf.h"

static guint scheduler_id = 0;
static guint scheduler_next;

static gboolean scheduler_switch(gpointer);
static void scheduler_toggle_button(gboolean);

void
scheduler_toggle()
{
    if(!scheduler_id)
        scheduler_start();
    else
        scheduler_stop();
}

void
scheduler_start()
{
    if(!conf.scheduler_n)
    {
        scheduler_toggle_button(FALSE);
        ui_dialog(ui.window,
                  GTK_MESSAGE_INFO,
                  "Frequency scheduler",
                  "No scheduler frequencies defined.\nAdd some in settings first.");
        return;
    }

    scheduler_next = 0;
    scheduler_switch(NULL);
    scheduler_toggle_button(TRUE);
}

void
scheduler_stop()
{
    if(!scheduler_id)
        return;

    scheduler_toggle_button(FALSE);
    g_source_remove(scheduler_id);
    scheduler_id = 0;
}

static gboolean
scheduler_switch(gpointer data)
{
    tuner_set_frequency(conf.scheduler_freqs[scheduler_next]);
    scheduler_id = g_timeout_add(conf.scheduler_timeouts[scheduler_next]*1000, scheduler_switch, NULL);
    scheduler_next = (scheduler_next + 1)%conf.scheduler_n;
    return FALSE;
}

static void
scheduler_toggle_button(gboolean active)
{
    g_signal_handlers_block_by_func(G_OBJECT(ui.b_scheduler), GINT_TO_POINTER(scheduler_toggle), NULL);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ui.b_scheduler), GPOINTER_TO_INT(active));
    g_signal_handlers_unblock_by_func(G_OBJECT(ui.b_scheduler), GINT_TO_POINTER(scheduler_toggle), NULL);
}
