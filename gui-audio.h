#ifndef XDR_GUI_AUDIO_H_
#define XDR_GUI_AUDIO_H_
#include "gui.h"

GtkWidget* volume_init();
gboolean volume_tooltip(GtkWidget *button, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer user_data);
gboolean volume_click(GtkWidget *widget, GdkEventButton *event);

GtkWidget* squelch_init();
gboolean squelch_tooltip(GtkWidget *button, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer user_data);
gboolean squelch_click(GtkWidget *widget, GdkEventButton *event);

void scale_text(GtkScaleButton *widget, gdouble sq, gpointer label);

#endif
