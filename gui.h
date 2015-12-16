#ifndef XDR_GUI_H_
#define XDR_GUI_H_
#include <gtk/gtk.h>

#define AF_LIST_STORE_ID   0
#define AF_LIST_STORE_FREQ 1

typedef struct gui_colors
{
    GdkColor background;
    GdkColor grey;
    GdkColor black;
    GdkColor stereo;
    GdkColor action;
    GdkColor prelight;
} gui_colors_t;

typedef struct gui
{
    GtkWidget *window;
    gchar window_title[100];
    gui_colors_t colors;
    GtkClipboard *clipboard;
    GdkCursor *click_cursor;

    GtkWidget *frame;
    GtkWidget *margin;
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
    GtkWidget *squelch;
    GtkWidget *event_band, *l_band;
    GtkWidget *event_freq, *l_freq;
    GtkWidget *event_pi, *l_pi;
    GtkWidget *event_ps, *l_ps;

    GtkWidget *b_scan;
    GtkWidget *b_tune_back;
    GtkWidget *e_freq;
    GtkWidget *b_tune_reset;

    GtkObject *adj_align;
    GtkWidget *hs_align;

    GtkWidget *graph, *p_signal;

    GtkWidget *event_st, *l_st;
    GtkWidget *l_rds;
    GtkWidget *l_tp;
    GtkWidget *l_ta;
    GtkWidget *l_ms;
    GtkWidget *l_pty;
    GtkWidget *l_sig;

    GtkWidget *b_connect;
    GtkWidget *b_pattern;
    GtkWidget *b_settings;
    GtkWidget *b_scheduler;
    GtkWidget *b_rdsspy;
    GtkWidget *b_ontop, *b_ontop_icon;

    GtkWidget *c_ant;
    GtkListStore *ant;

    GtkWidget *b_cw, *b_cw_label;
    GtkWidget *b_ccw, *b_ccw_label;
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
} gui_t;

gui_t gui;

void gui_init();
void gui_destroy();
gboolean gui_delete_event(GtkWidget*, GdkEvent*, gpointer);
void dialog_error(gchar*, gchar*, ...);
gboolean gui_update_status(gpointer);
void gui_clear();
gboolean gui_clear_rds();
gboolean gui_mode_FM();
gboolean gui_mode_AM();
void gui_fill_bandwidths(GtkWidget*, gboolean);
void tune_gui_back(GtkWidget*, gpointer);
void tune_gui_round(GtkWidget*, gpointer);
void tune_gui_step_click(GtkWidget*, GdkEventButton*, gpointer);
void tune_gui_step_scroll(GtkWidget*, GdkEventScroll*, gpointer);
void tune_gui_af(GtkTreeSelection*, gpointer);
gboolean gui_toggle_gain(GtkWidget*, GdkEventButton*, gpointer);
gboolean gui_update_clock(gpointer);
gchar* s_meter(gfloat);
void window_on_top(GtkToggleButton*);
void connect_button(gboolean);
void gui_antenna_switch(gint);
void gui_toggle_band(GtkWidget *widget, GdkEventButton *event, gpointer step);
void gui_st_click(GtkWidget *widget, GdkEventButton *event, gpointer step);
gboolean gui_cursor(GtkWidget *widget, GdkEvent  *event, gpointer cursor);
void gui_antenna_showhide();
gboolean signal_tooltip(GtkWidget*, gint, gint, gboolean, GtkTooltip*, gpointer);
void gui_toggle_ps_mode();
void gui_screenshot();
void gui_activate();
gboolean gui_window_event(GtkWidget*, gpointer);
void gui_rotator_button_realized(GtkWidget*);
void gui_rotator_button_swap();
void gui_status(gint, gchar*, ...);

#endif

