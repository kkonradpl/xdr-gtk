#ifndef XDR_MENU_H_
#define XDR_MENU_H_

typedef struct
{
    GtkWidget *connect;
    GtkWidget *alignment;
    GtkWidget *scan;
    GtkWidget *pattern;
    GtkWidget *alwaysontop;
    GtkWidget *settings;
    GtkWidget *about;
} menu_struct;

GtkWidget* menu_create();
gboolean menu_popup(GtkWidget*, GdkEventButton*);
void window_on_top(GtkCheckMenuItem *item);
void about_dialog();
#endif
