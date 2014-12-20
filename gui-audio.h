#ifndef XDR_GUI_AUDIO_H_
#define XDR_GUI_AUDIO_H_
#include "gui.h"

GtkWidget* volume_init();
gboolean volume_tooltip(GtkWidget *button, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer user_data);
gboolean volume_click(GtkWidget *widget, GdkEventButton *event);
void volume_text(GtkScaleButton *widget, gdouble value, gpointer label);

GtkWidget* squelch_init();
gboolean squelch_tooltip(GtkWidget *button, gint x, gint y, gboolean keyboard_mode, GtkTooltip *tooltip, gpointer user_data);
gboolean squelch_click(GtkWidget *widget, GdkEventButton *event);
void squelch_text(GtkScaleButton *widget, gdouble sq, gpointer label);
void squelch_icon(GtkScaleButton *button, gdouble value, gpointer user_data);

#endif
