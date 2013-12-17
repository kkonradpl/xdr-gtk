#ifndef XDR_GUI_H_
#define XDR_GUI_H_
#include "gui_net.h"
#include "menu.h"

#define FONT_FILE "VeraMono.ttf"

typedef struct
{
    GdkColor background;
    GdkColor grey;
    GdkColor black;
    GdkColor stereo;
} gui_colors;

typedef struct
{
    GtkWidget *window;
    gchar window_title[100];
    gui_colors colors;
    GtkClipboard *clipboard;

    GtkWidget *box;
    GtkWidget *box_header;
    GtkWidget *box_gui;
    GtkWidget *box_left;
    GtkWidget *box_left_tune;
    GtkWidget *box_left_signal;
    GtkWidget *box_left_indicators;
    GtkWidget *box_left_settings1;
    GtkWidget *box_left_settings2;
    GtkWidget *box_right;

    GtkWidget *volume;
    GtkAdjustment *volume_adj;
    GtkWidget *event_band, *l_band;
    GtkWidget *event_freq, *l_freq;
    GtkWidget *event_pi, *l_pi;
    GtkWidget *event_ps, *l_ps;

    GtkWidget *b_tune_back;
    GtkWidget *e_freq;
    GtkWidget *b_tune_reset;

    GtkWidget *graph, *p_signal;

    GtkWidget *l_st;
    GtkWidget *l_rds;
    GtkWidget *l_tp;
    GtkWidget *l_ta;
    GtkWidget *l_ms;
    GtkWidget *l_pty;
    GtkWidget *l_sig;

    GtkWidget *menu, *b_menu;
    menu_struct menu_items;
    GtkWidget *c_ant;
    GtkWidget *l_agc, *c_agc;
    GtkWidget *x_rf;

    GtkWidget *l_deemph, *c_deemph;
    GtkWidget *l_bw, *c_bw;
    GtkWidget *x_if;

    GtkWidget *event_rt[2], *l_rt[2];

    guint status_timeout;
    GtkWidget *l_status;

    GtkWidget *l_af;
    GtkListStore *af;
    GtkWidget *af_box;
} gui_struct;

gui_struct gui;

void gui_init();
void gui_quit();
void dialog_error(gchar* msg);
gboolean gui_update_status(gpointer);
gboolean gui_clear(gpointer);
gboolean gui_update_ps(gpointer);
gboolean gui_update_rt(gpointer);
gboolean gui_update_pi(gpointer);
gboolean gui_update_ptytp(gpointer);
gboolean gui_update_tams(gpointer);
gboolean gui_update_af(gpointer);
gboolean gui_af_check(GtkTreeModel*, GtkTreePath*, GtkTreeIter*, gpointer*);
gboolean gui_clear_power_off(gpointer);
void gui_mode_toggle(GtkWidget*, GdkEventButton*, gpointer);
gboolean gui_mode_FM();
gboolean gui_mode_AM();
void gui_fill_bandwidths(GtkWidget*, gboolean);
void tune_gui_back(GtkWidget*, GdkEventButton*, gpointer);
void tune_gui_round(GtkWidget*, GdkEventButton*, gpointer);
void tune_gui_step_click(GtkWidget*, GdkEventButton*, gpointer);
void tune_gui_step_scroll(GtkWidget*, GdkEventScroll*, gpointer);
void tune_gui_af(GtkTreeSelection*, gpointer);
void tty_change_bandwidth();
void tty_change_deemphasis();
void tty_change_volume(GtkScaleButton*, gdouble, gpointer);
void tty_change_ant();
void tty_change_agc();
void tty_change_gain();
gboolean gui_toggle_gain(GtkWidget*, GdkEventButton*, gpointer);
gboolean gui_update_clock(gpointer);
gboolean volume_click(GtkWidget*, GdkEventButton*);
gchar* s_meter(gfloat);
#endif
