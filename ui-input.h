#ifndef XDR_UI_INPUT_H_
#define XDR_UI_INPUT_H_

gboolean keyboard_press(GtkWidget*, GdkEventKey*, gpointer);
gboolean keyboard_release(GtkWidget*, GdkEventKey*, gpointer);
gboolean mouse_scroll(GtkWidget*, GdkEventScroll*, gpointer);
gboolean mouse_window(GtkWidget*, GdkEventButton*, GtkWindow*);
gboolean mouse_freq(GtkWidget*, GdkEvent*, gpointer);
gboolean mouse_pi(GtkWidget*, GdkEvent*, gpointer);
gboolean mouse_ps(GtkWidget*, GdkEventButton*, gpointer);
gboolean mouse_rt(GtkWidget*, GdkEvent*, gpointer);

#endif
