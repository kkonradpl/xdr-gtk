#ifndef XDR_SCHEDULER_H_
#define XDR_SCHEDULER_H_

void scheduler_toggle();
void scheduler_start();
void scheduler_stop();
gboolean scheduler_switch(gpointer);
void scheduler_toggle_button(gboolean);
#endif


